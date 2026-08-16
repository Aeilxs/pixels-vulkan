Oui. J'ai fait toute l'archive **fichier par fichier**. Et je vais commencer par la conclusion parce qu'elle conditionne tout le reste :

> **Je ne ferais PAS le compute maintenant.**
> Je ferais **répulsion souris → petite passe d'architecture renderer/Buffer → mesure → compute sur la graphics queue**.
> Et surtout pas une dedicated compute queue tout de suite.

Le code est suffisamment propre pour continuer à construire dessus. On n'est pas dans une situation où il faut arrêter les features pendant trois jours pour refactorer 3000 LOC.

## Mon ordre de bataille

1. **Commit immédiatement l'état actuel** `largePoints + circular particles`.
2. **Répulsion souris CPU** avec vraie `velocity`, retour à l'origine, damping et test de rayon.
3. Petite passe de cleanup issue de cet audit.
4. Sortir **la ressource GPU des particules** de `Renderer`, mais pas mettre un pauvre `Buffer` brut dans `App`.
5. Faire évoluer `Buffer` : garder `write()`, ajouter mapping explicite/persistant.
6. Mesurer.
7. Séparer les données qui changent de celles qui ne changent pas **si les mesures le justifient**.
8. Premier compute shader **sur la même graphics queue**.
9. Dedicated compute queue uniquement comme expérience ultérieure.

Ça nous donne à chaque fois un morceau concret à apprendre.

---

# 1. `src/main.cpp` — ✅ très bien

Rien de sérieux.

J'aime bien :

```cpp
int run(int argc, char* argv[])
```

puis :

```cpp
int main(...) {
    try {
        return run(...);
    } catch (...) {
```

Ça garde ton `main` comme frontière d'exception.

Micro-detail : `run()` a actuellement une linkage globale alors qu'il n'intéresse que ce fichier. Tu pourrais :

```cpp
namespace {

int run(...) {
    ...
}

}
```

Mais franchement : **osef-tier**.

### Verdict

**Ne touche pas.**

---

# 2. `app/config.hpp` — 🟡 première vraie dette architecturale

Là il y a un truc que je changerais.

Tu as :

```cpp
namespace ps::app::config
```

qui contient à la fois :

```cpp
initialWindowWidth
initialWindowHeight
applicationName
```

et :

```cpp
requiredVulkanApiVersion
requiredVulkanDeviceExtensions
engineName
engineVersion
```

Et par conséquent :

```cpp
device.cpp
physical_device.cpp
instance.cpp
```

font :

```cpp
#include "app/config.hpp"
```

Donc actuellement :

```text
renderer/vulkan
      ↓
     app
```

C'est à l'envers.

Le renderer bas niveau ne devrait pas dépendre de la couche application.

### Pas besoin d'une ConfigFactoryEnterprise™

À terme seulement :

```text
app/config.hpp
    application/window stuff

renderer/vulkan/config.hpp
    required API
    required Vulkan extensions
    renderer/engine version
```

Et éventuellement `Instance` reçoit le nom/version de l'application.

### Verdict

**À corriger pendant la prochaine petite passe de cleanup.**

Pas urgent fonctionnellement, mais architecturellement c'est une vraie amélioration.

---

# 3. `app/app.hpp` — ✅ très sain

Ta liste de membres raconte très clairement le programme :

```cpp
ParticleSystem
SDL
Window
Camera

Instance
Surface
PhysicalDevice
Device
Swapchain
Renderer
```

Et surtout l'ordre de destruction est naturellement correct à l'envers.

Je **ne créerais pas un `VulkanContext` géant maintenant** juste pour réduire cette liste à :

```cpp
VulkanContext vulkan_;
```

Ça cacherait justement des concepts que tu cherches à apprendre.

Plus tard, peut-être.

### En revanche

On aura probablement bientôt :

```cpp
ParticleSystem particleSystem_;
...
Device device_;
Swapchain swapchain_;

ParticleGpuData particleGpuData_;

Renderer renderer_;
```

Et ça, **ça me semblerait beaucoup plus intéressant** que `Renderer` possédant directement le buffer des particules.

J'y reviens.

### Verdict

**Très bon en l'état.**

---

# 4. `app/app.cpp` — 🟢 bon, bientôt enrichi par l'input

Très lisible.

J'aime beaucoup cette ligne architecturalement :

```cpp
particleSystem_.update(dt);
renderer_.drawFrame(camera_.viewProjection(), particleSystem_.particles());
```

Elle raconte tout.

En revanche elle révèle aussi notre futur problème :

```text
simulation CPU
     ↓
Renderer reçoit les Particle CPU
     ↓
Renderer fait lui-même l'upload GPU
```

`Renderer` sait donc trop de choses sur les particules.

Mais **je ne le changerais pas avant la répulsion**.

Pourquoi ?

Parce qu'on veut d'abord définir complètement ce qu'est notre simulation CPU avant de modifier la représentation GPU.

### Le prochain truc ici

Pour la souris, App est exactement le bon endroit pour :

```text
SDL screen coords
        ↓
Camera2D::screenToWorld()
        ↓
ParticleSystem reçoit WORLD coords
```

Le `ParticleSystem` ne doit jamais connaître SDL ou des pixels écran.

Parfait.

### Petit futur piège : `dt`

Avec la future physique :

```cpp
velocity += force * dt;
position += velocity * dt;
```

si tu mets le programme en pause/debug pendant 4 secondes, ton prochain :

```cpp
dt = 4.0F;
```

peut expédier tes particules sur Jupiter.

On pourra au minimum clamp :

```cpp
dt = std::min(dt, 0.05F);
```

avant d'aborder éventuellement le fixed timestep un jour.

### Verdict

**Très bien. Pas de refactor avant mouse repulsion.**

---

# 5. `cli.hpp/.cpp` — ✅ fini

Franchement on a assez parlé de cette pauvre CLI. 😂

Ça fait le taf.

Deux détails maximum :

```cpp
image_path
```

est snake_case alors que le reste du projet est plutôt camelCase :

```cpp
imagePath
```

Et :

```cpp
if (!std::filesystem::exists(...)) {
    throw std::runtime_error(...)
}
```

fait qu'une image inexistante n'affiche pas le `usage`, contrairement à `cli::Error`.

Mais je trouve ça défendable :

* mauvaise syntaxe → usage
* fichier inexistant → runtime error

### Verdict

**NE PLUS TOUCHER À LA CLI BORDEL.**

---

# 6. `gfx/particles/particle.hpp` — 🟡 futur point architectural important

Actuellement :

```cpp
struct Particle {
    glm::vec2 position;
    glm::vec2 origin;
    glm::vec2 velocity;
    glm::vec4 color;
};
```

Conceptuellement, deux catégories vivent ensemble :

```text
SIMULATION
position
origin
velocity

RENDERING/APPEARANCE
color
```

Ça ne me choque absolument pas pour maintenant.

Mais c'est précisément **pourquoi tu uploads 40 B par particule chaque frame**.

Et c'est aussi pourquoi ta boucle CPU traîne les cache lines contenant `color` alors qu'elle s'en fout.

Donc je mets un gros post-it ici :

> **Ne modifions pas ça maintenant, mais c'est probablement la première structure qui changera lorsque nous optimiserons.**

Il y a même un truc particulièrement intéressant : ta couleur vient d'une image RGBA8.

On transforme actuellement :

```text
RGBA8
4 bytes
```

en :

```cpp
glm::vec4
```

soit :

```text
16 bytes
```

pour... que Vulkan la retransforme en valeurs utilisées dans le shader.

Or Vulkan sait très bien lire :

```cpp
VK_FORMAT_R8G8B8A8_UNORM
```

et fournir directement au shader un `vec4` normalisé.

Donc un jour :

```text
16 B color / particle
          ↓
4 B color / particle
```

sans sorcellerie.

Très gfx-dev comme optimisation. 😏

### Verdict

**Garder maintenant. Gros candidat d'optimisation plus tard.**

---

# 7. `gfx/particles/system.hpp/.cpp` — 🟡 bon prototype, quelques trucs à nettoyer

`fromImage()` est propre.

Le :

```cpp
particles.reserve(rows * cols);
```

est bien.

Le calcul :

```cpp
const std::size_t index =
    static_cast<std::size_t>(y) * image.width + x;
```

est également propre.

### Le premier truc que je virerais

```cpp
std::cout << "Particle count: ..."
```

dans `ParticleSystem`.

Un objet métier/simulation ne devrait pas décider d'imprimer sur stdout.

Ce serait mieux dans App :

```cpp
std::cout << particleSystem_.particles().size();
```

ou plus tard dans un logger.

Pas dramatique, mais vraie séparation de responsabilité.

---

### `std::rand()`

Ça :

```cpp
particle.position.x =
    static_cast<float>(std::rand() % imageDimensions_.width);
```

marche.

Mais c'est vraiment le moment où je passerais à :

```cpp
std::mt19937
std::uniform_real_distribution<float>
```

ou `uniform_int_distribution`.

Et accessoirement ton `.cpp` utilise `std::rand()` sans :

```cpp
#include <cstdlib>
```

Il compile probablement via un include transitif.

**À corriger.**

---

### Un edge case

Image intégralement transparente :

```text
0 particles
```

puis :

```cpp
createParticleBuffer(...)
```

calcule :

```cpp
bufferSize = 0;
```

et `Buffer` throw.

Ce n'est pas faux, mais l'erreur :

> Vulkan buffer size must be greater than zero

est moins utile que :

> Source image contains no visible particles.

Je mettrais cette validation dans `ParticleSystem::fromImage()`.

### Verdict

Petit cleanup :

* `<cstdlib>`
* meilleur RNG
* stdout dehors
* reject 0 particles

Puis on ajoute la vraie physique.

---

# 8. `Camera2D` — ✅ garde-la telle quelle

C'est probablement l'une des abstractions les plus réussies du repo.

Elle ne connaît :

* ni Vulkan,
* ni SDL,
* ni ParticleSystem.

Elle connaît :

```text
world
view
zoom
projection
screen → world
```

Exactement son job.

Et maintenant :

```cpp
screenToWorld()
```

va enfin servir pour autre chose qu'exister dans le README. 😎

### Une seule vraie dépendance future

Au resize de fenêtre il faudra :

```cpp
camera_.setViewSize(newLogicalSize);
```

Sinon `screenToWorld()` devient incohérent après resize.

### Verdict

**Pas toucher.**

---

# 9. `image/image.*` — ✅ très bon petit module

Le `stb_image` reste parfaitement enfermé dans le `.cpp`.

Très bien :

```cpp
StbiPixels
```

avec custom deleter.

Très bien :

```cpp
static_assert(sizeof(Pixel) == 4);
```

La copie :

```cpp
memcpy(image.pixels.data(), pixels.get(), ...)
```

est simple.

Je pourrais pinailler et ajouter :

```cpp
static_assert(std::is_trivially_copyable_v<Pixel>);
```

mais ton struct de quatre `uint8_t` est évidemment trivially copyable.

### Portabilité Windows

Un vrai edge case futur : `path.string()` + API `char*` de stb peut être pénible avec des chemins Windows contenant des caractères Unicode.

**Pas notre problème aujourd'hui.**

### Verdict

**Ne touche pas.**

---

# 10. `SdlContext` — ✅

RAS.

Petit wrapper RAII parfaitement légitime.

---

# 11. `Window` — 🟡 une contradiction fonctionnelle

Tu crées :

```cpp
SDL_WINDOW_RESIZABLE
```

mais le renderer dit :

```text
swapchain recreation is not implemented yet
```

Donc actuellement :

> « La fenêtre est resizable ! »

puis :

> « Oh putain il a resize, THROW. »

😂

Deux solutions honnêtes :

**maintenant :**

retirer temporairement :

```cpp
SDL_WINDOW_RESIZABLE
```

ou :

**bientôt :**

implémenter correctement :

```text
window resize
→ update camera view size
→ recreate swapchain
→ éventuellement recreate pipeline si format change
→ recreate sync resources liés au nombre d'images
```

Je prendrais plutôt la deuxième quand on aura envie d'apprendre le lifecycle swapchain.

Pas maintenant.

### Verdict

**Dette connue.**

---

# 12. `Buffer` — 🟡/🟢 très bonne première abstraction, prochaine cible logique

C'est là que ton intuition était bonne.

Actuellement ton `Buffer` est **simple et sain**.

Je ne supprimerais surtout pas :

```cpp
void write(const void* data, VkDeviceSize size);
```

C'est une bonne API.

Même après avoir `map()` :

```cpp
buffer.write(...)
```

reste parfait pour :

* upload initial,
* petits buffers,
* données occasionnelles,
* tests,
* code où la performance du mapping n'a aucune importance.

### Donc :

> **on garde `write()`.**

Ce qu'on ajoute, c'est un niveau en dessous.

Probablement :

```cpp
void* map();
void unmap() noexcept;
```

avec un membre :

```cpp
void* mappedMemory_{nullptr};
```

Et le destructor sait gérer un mapping encore ouvert.

Ensuite une ressource qui veut du persistent mapping fait :

```cpp
void* data = buffer.map();
```

une fois, puis écrit dedans chaque frame.

---

### Une incohérence que je corrigerais

`Buffer` stocke :

```cpp
const Device* device_{};
```

alors que pratiquement tous tes autres wrappers stockent :

```cpp
VkDevice device_;
```

Par exemple :

```cpp
CommandPool
CommandBuffer
Swapchain
GraphicsPipeline
```

Je préfère leur approche.

Le buffer a besoin du **handle Vulkan** pour se détruire, pas de l'objet C++ `Device`.

Donc :

```cpp
VkDevice device_{VK_NULL_HANDLE};
```

me semble plus cohérent.

Tu supprimes un niveau de lifetime C++ artificiel.

Évidemment le logical device Vulkan doit toujours vivre plus longtemps que le Buffer.

---

### Autre subtilité intéressante

Tu stockes :

```cpp
requiredMemoryProperties_
```

C'est ce que **tu as demandé**, pas nécessairement toutes les propriétés de la memory réellement sélectionnée.

Exemple théorique :

```text
tu demandes HOST_VISIBLE

memory choisie :
HOST_VISIBLE | HOST_COHERENT
```

Ton buffer est réellement coherent.

Mais :

```cpp
(requiredMemoryProperties_ & HOST_COHERENT) == 0
```

donc `write()` dirait :

> nope

alors que la mémoire le permet.

Pour l'instant on demande explicitement :

```cpp
HOST_VISIBLE | HOST_COHERENT
```

donc aucun bug actuel.

Mais si on veut rendre `Buffer` vraiment général, il faudra connaître les **actual memory flags** du memory type choisi.

---

### `write()` avec offset

Très probablement utile bientôt :

```cpp
void write(
    const void* data,
    VkDeviceSize size,
    VkDeviceSize offset = 0
);
```

avec :

```cpp
offset + size <= size_
```

Mais YAGNI : quand on en a besoin.

### Verdict

**Très bonne abstraction actuelle. La prochaine évolution est additive, pas une réécriture.**

---

# 13. `CommandPool` — ✅

Exactement le bon niveau d'abstraction.

Pas sexy, mais propre.

---

# 14. `CommandBuffer` — ✅

Même avis.

Il possède clairement :

```text
VkCommandBuffer
```

et connaît le pool nécessaire pour le libérer.

Simple.

On pourra un jour se demander si `vkFreeCommandBuffers()` explicitement est utile puisque détruire le pool les libère, mais explicite ici = très bien.

### Verdict

**Ne touche pas.**

---

# 15. `FrameSynchronization` — ✅ particulièrement bien

Je trouve ce code plus réfléchi que la moyenne des premiers renderers Vulkan.

Tu as :

```text
1 frame in flight

1 imageAvailable
1 fence

renderFinished[swapchainImage]
```

La partie :

```cpp
renderFinished_.push_back(createSemaphore(device_));
```

par image swapchain évite les réutilisations foireuses d'un semaphore encore impliqué dans la présentation.

Pour notre architecture actuelle : propre.

Évidemment si on passe un jour à 2–3 frames in flight, toute cette classe changera.

**Ce n'est pas une raison pour anticiper maintenant.**

### Verdict

**Keep.**

---

# 16. `GraphicsPipeline` — 🟠 ici je veux un changement

C'est probablement **le fichier dont le nom ment le plus actuellement**.

La classe s'appelle :

```cpp
GraphicsPipeline
```

Ça semble générique.

Mais elle fait :

```cpp
particle.vert.spv
particle.frag.spv
```

puis :

```cpp
sizeof(Particle)
offsetof(Particle, position)
offsetof(Particle, color)
```

puis :

```cpp
VK_PRIMITIVE_TOPOLOGY_POINT_LIST
```

Donc ce n'est absolument pas :

> GraphicsPipeline

C'est :

> **ParticlePipeline**

Et ce n'est pas grave !

Au contraire.

Je préférerais largement une abstraction **spécifique honnête** :

```cpp
ParticlePipeline
```

à une abstraction soi-disant générale qui sait secrètement comment fonctionne `Particle`.

### Et surtout je ne ferais PAS maintenant :

```cpp
GraphicsPipelineBuilder
    .withShader(...)
    .withVertexDescription(...)
    .withBlend(...)
    .withTopology(...)
```

PTSD enterprise immédiat. 😂

On n'a qu'un pipeline.

Quand on aura :

```text
ParticlePipeline
SpritePipeline
UiPipeline
TextPipeline
```

on regardera ce qui est réellement commun et on extraira.

### Autre point

Le pipeline connaît directement la représentation CPU :

```cpp
Particle
```

À terme il devrait connaître une représentation **GPU**.

Parce que la prochaine fois qu'on fait compute, je ne veux surtout pas être obligé de dire :

```text
CPU Particle == SSBO layout == Vertex format
```

C'est une fausse économie.

D'ailleurs point très important :

## ton `Particle` actuel n'est PAS une struct que je réutiliserais aveuglément dans un SSBO

Côté GLSL `std430`, les alignements d'une structure avec notamment un `vec4` peuvent introduire du padding différent de ton C++ `glm`.

Donc lorsque le compute arrivera, on définira intentionnellement le layout GPU.

Encore une excellente raison de séparer :

```text
Particle CPU state
```

de :

```text
Particle GPU representation
```

### Verdict

**Rename `GraphicsPipeline` → `ParticlePipeline` bientôt.**

Pas de généralisation prématurée.

---

# 17. `Renderer` — 🟠 le gros sujet

Voilà le morceau qui mérite vraiment notre attention.

Actuellement `Renderer` possède :

```cpp
GraphicsPipeline graphicsPipeline_;
CommandPool commandPool_;
CommandBuffer commandBuffer_;
FrameSynchronization synchronization_;

Buffer particleBuffer_;
std::uint32_t particleCount_;
```

Les quatre premiers sont clairement :

> **resources needed to render frames**

Les deux derniers sont :

> **content being rendered**

Et là je suis d'accord avec ton intuition :

```cpp
Buffer particleBuffer_;
```

**n'a probablement pas vocation à rester dans `Renderer`.**

Mais attention.

Je ne veux pas faire :

```cpp
App {
    Buffer particleBuffer_;
}
```

et considérer l'architecture résolue.

Parce que `App` devrait alors savoir :

```text
usage flags
memory properties
particle count
upload byte size
```

C'est juste déplacer la merde.

---

## Ce que je vois émerger

Un petit objet sémantique du genre :

```cpp
class ParticleBuffer {
public:
    ParticleBuffer(
        const PhysicalDevice&,
        const Device&,
        std::span<const Particle>
    );

    void upload(std::span<const Particle> particles);

    VkBuffer nativeHandle() const noexcept;
    std::uint32_t particleCount() const noexcept;

private:
    Buffer buffer_;
    std::uint32_t particleCount_{};
};
```

Nom à débattre ; ce n'est qu'une idée.

App pourrait posséder :

```cpp
ParticleSystem particleSystem_;

...

ParticleBuffer particleBuffer_;
Renderer renderer_;
```

et la frame ferait explicitement :

```cpp
particleSystem_.update(dt);

particleBuffer_.upload(
    particleSystem_.particles()
);

renderer_.drawFrame(
    camera_.viewProjection(),
    particleBuffer_
);
```

Ça raconte exactement :

```text
CPU simulation
     ↓
GPU resource update
     ↓
render
```

🔥

Et surtout `ParticleBuffer` pourra évoluer en interne.

Aujourd'hui :

```text
1 Buffer de Particle
```

demain :

```text
position buffer
color buffer
```

plus tard :

```text
storage buffer
descriptor set
compute state
```

sans faire grossir `Renderer` en sapin de Noël.

---

## Mais ne le faisons pas avant la souris

Je veux que le comportement CPU soit notre **référence correcte**.

Ensuite on remanie la plomberie.

---

### Autre remarque Renderer

`recordCommandBuffer()` fait maintenant ~120 lignes.

Ça ne me gêne pas encore.

Je préfère largement voir explicitement :

```cpp
vkCmdPipelineBarrier2
vkCmdBeginRendering
vkCmdBind...
vkCmdDraw
vkCmdEndRendering
```

que créer maintenant :

```cpp
RenderingCommandRecorderFactory
```

Non.

Un jour les transitions image répétées pourront devenir des helpers.

Pas maintenant.

### Verdict

**L'ownership du particle buffer doit sortir à moyen terme. Le reste du Renderer reste.**

---

# 18. `Instance` — 🟡

Implementation propre.

Apple correctement isolé :

```cpp
#ifdef __APPLE__
```

### Mais deux remarques

La dépendance :

```cpp
#include "app/config.hpp"
```

dont je parlais.

Et plus important pour un projet d'apprentissage Vulkan :

> **On n'a pas de validation layers/debug messenger dans ce repo.**

Ça, j'aimerais qu'on le fasse à un moment.

Parce que quand on commencera :

```text
descriptors
storage buffer
compute barriers
```

faire Vulkan sans validation est un peu :

> conduire de nuit sans phares parce qu'on maîtrise la route.

😂

Ce sera du boilerplate, certes, mais **très rentable pédagogiquement**.

Pas aujourd'hui si tu veux jouer avec les particules.

### Verdict

Propre ; debug validation à inscrire sur la todo.

---

# 19. `Surface` — ✅

Parfait.

Petit wrapper RAII, rien d'autre.

---

# 20. `PhysicalDevice` — 🟢 très correct

Après notre correction `largePoints`, le chemin est propre :

```text
query API
queues
extensions
swapchain
features
```

Puis :

```cpp
suitable()
```

Très lisible.

### Compute futur

Ici viendra notre changement.

Actuellement les queue families connaissent :

```cpp
graphics
present
```

Pour le compute, **je ne chercherais pas immédiatement une dedicated compute queue**.

Je vérifierais d'abord que notre graphics queue possède aussi :

```cpp
VK_QUEUE_COMPUTE_BIT
```

et on ferait :

```text
compute dispatch
barrier
graphics draw
```

sur **la même queue**.

Pourquoi ?

Parce qu'on apprend alors :

* compute pipeline,
* storage buffers,
* descriptors,
* dispatch,
* workgroups,
* memory barrier,

sans ajouter simultanément :

* cross-queue synchronization,
* queue-family ownership transfers,
* semaphores entre queues.

Après que ça marche :

> maintenant est-ce qu'une dedicated compute queue apporte quelque chose ?

Et là on expérimente.

Ça c'est exactement le genre de progression que je veux pour ce projet.

### Verdict

**Bon. Compute support plus tard, sans massacre.**

---

# 21. `Device` — ✅ maintenant correct

Le nouveau chaînage :

```text
VkDeviceCreateInfo
→ VkPhysicalDeviceFeatures2
→ VkPhysicalDeviceVulkan13Features
```

est propre.

Et le commentaire :

```cpp
// Features are specified in the pNext chain
```

fait partie des rares commentaires que je garderais sans hésiter.

Pas besoin de toucher.

---

# 22. `Swapchain` — 🟢 bon mais grosse future feature

Il fait beaucoup de lignes parce que... **Vulkan**. 😂

Mais les fonctions locales :

```cpp
querySwapchainSupport
chooseSurfaceFormat
choosePresentMode
chooseExtent
chooseCompositeAlpha
createImageView
```

segmentent bien.

Ça reste lisible.

Quelques commentaires genre :

```cpp
// Vulkan doesn't like to allocate memory for you...
```

sont maintenant probablement trop pédagogiques pour le niveau auquel on arrive.

On peut les raccourcir pendant le cleanup.

### Vraie dette

Toujours la recreation.

C'est probablement l'une des prochaines grosses choses de “renderer généraliste” à apprendre :

```text
resize
minimize
VK_ERROR_OUT_OF_DATE_KHR
VK_SUBOPTIMAL_KHR
swapchain replacement
dependent resource recreation
```

Mais ça peut attendre.

### Verdict

**Pas besoin de refactor. Implement lifecycle plus tard.**

---

# 23. `QueueFamilyIndices` — ✅

Très simple :

```cpp
graphics
present
```

Puis éventuellement :

```cpp
compute
```

plus tard.

Ne surtout pas l'ajouter avant qu'on fasse effectivement du compute.

---

# 24. shaders — ✅ STOP 😂

Ils marchent.

Tu as gagné le droit de ne plus regarder du GLSL aujourd'hui.

```glsl
gl_PointSize = 5.0;
```

et ton petit cercle fonctionnent.

Très bien.

**ON QUITTE LE DONJON GLSL.**

---

# 25. `CMakeLists.txt` — 🟢 propre

Franchement pas grand-chose à dire.

Le custom command shader est clair.

Le target est explicite.

Les dépendances également.

Pour Windows, la vraie question sera surtout :

```text
comment SDL3/glm/Vulkan SDK sont installés/trouvés
```

pas ton CMake lui-même.

### Deux futures améliorations raisonnables

Warnings :

```text
-Wall -Wextra -Wpedantic
/W4
```

et éventuellement utiliser ce que `FindVulkan` expose déjà pour glslang plutôt qu'un `find_program` séparé.

Mais pas prioritaire.

---

# 26. Documentation/comments — 🟡 maintenant on peut réduire

Ton code est **surdocumenté par endroits**.

Pas catastrophiquement, mais on a parfois :

```cpp
/// Returns foo.
/// @return Foo.
```

sur :

```cpp
VkQueue graphicsQueue() const noexcept;
```

Ça n'apporte pas grand-chose.

Et :

```cpp
[[nodiscard("The Vulkan graphics queue handle must be used")]]
```

est franchement plus bruyant que :

```cpp
[[nodiscard]]
```

Je ferais éventuellement une passe :

> commentaires sur **pourquoi**, moins de commentaires sur **ce que la signature dit déjà**.

Par contre je conserverais des commentaires comme :

```cpp
// Reset fence only after command recording...
```

ou :

```cpp
// Features are specified in pNext chain
```

parce qu'ils racontent une contrainte non évidente.

---

# Maintenant : tes différentes propositions

### « Compute queue »

**Non, pas encore.**

Et quand on la fera :

```text
compute shader
sur graphics queue d'abord
```

avant dedicated compute queue.

---

### « Répulsion souris »

**OUI. Mon choix n°1.**

Parce que ça va compléter notre modèle CPU :

```text
origin force
velocity
damping
mouse repulsion
radius
```

et nous donner la référence qu'on portera ensuite GPU.

Et surtout ça va enfin faire :

```cpp
camera_.screenToWorld(...)
```

dans une vraie feature.

---

### « Passe cleanup »

Une **petite**, oui, après la souris.

Pas un grand refactor.

Les éléments déjà identifiés :

```text
app/config dependency
ParticleSystem stdout
std::rand
missing <cstdlib>
GraphicsPipeline -> ParticlePipeline
comments trop bavards
empty-particle error
```

Ça tient en petits commits.

---

### « Améliorer l'abstraction Renderer »

**Oui, après mouse.**

Et je pense maintenant assez clairement que :

```cpp
Renderer owns particleBuffer
```

est une architecture temporaire.

Je veux séparer :

```text
frame rendering resources
```

de :

```text
GPU content resources
```

sans cacher Vulkan.

---

### « Buffer map/unmap »

**Oui. Mais on garde `write()`.**

Conceptuellement :

```text
Buffer
├── write()          ← API simple
├── map()            ← API bas niveau
└── unmap()
```

Puis ton futur `ParticleBuffer` choisit persistent mapping.

C'est précisément le genre d'abstraction qui **ajoute une capacité sans casser le happy path**.

---

### « Arrêter d'envoyer les couleurs »

Oui, mais **pas tout de suite**.

Ton intuition :

> « ne passer RGB que si RGB change »

est bonne.

Mais avec ton layout actuel :

```cpp
Particle {
    position
    origin
    velocity
    color
}
```

tu ne peux pas arrêter d'uploader color indépendamment : tout est interleaved dans le même `memcpy`.

Pour y arriver proprement il faudra modifier la représentation.

Et ton ancien :

> grayscale / white

ne pose aucun problème conceptuel.

La couleur devient simplement :

```text
dirty = false normalement

user change color mode
→ dirty = true
→ upload color buffer
→ dirty = false
```

Ou plus tard on fait certains effets couleur dans le shader.

Mais **pas maintenant**.

---

# Le truc qui me plaît beaucoup dans la direction actuelle

On commence réellement à voir trois couches émerger naturellement :

```text
               APPLICATION
                   App
                    │
       ┌────────────┴────────────┐
       ▼                         ▼
  SIMULATION                   CAMERA
 ParticleSystem               Camera2D
       │
       │ CPU state
       ▼
   GPU RESOURCE          ← prochaine abstraction
 ParticleBuffer/...
       │
       ▼
       RENDERER
 frame lifecycle / commands
       │
       ▼
     VULKAN
 Buffer / Device / Swapchain / Pipeline / ...
```

Et ça, **je n'aurais surtout pas voulu le dessiner au début du projet et coder 25 interfaces pour correspondre au diagramme**.

Maintenant ça émerge parce qu'on a **réellement rencontré le besoin**.

C'est beaucoup plus sain.

---

## Mon évaluation globale de cette archive

Je mettrais quelque chose comme **16/20 pour un lab Vulkan arrivé à ce stade**.

Pas parce que tout est parfait, mais parce que les défauts actuels sont principalement des **défauts d'évolution normaux** :

* Renderer devenu trop spécifique parce qu'on lui a ajouté des particules ;
* Buffer devenu un peu limité parce qu'on commence à pousser les uploads ;
* CPU/GPU representation confondues parce qu'au départ on avait juste besoin de voir des points ;
* swapchain resize absent parce qu'on n'en avait pas encore besoin ;
* compute absent parce qu'on apprend dans l'ordre.

Je ne vois pas un gros choix architectural que je voudrais jeter.

Et ça, c'est une excellente situation.

**Donc prochain commit après avoir figé l'état actuel : mouse fucking repulsion.** Puis on reviendra sur `Buffer`/`Renderer`, et là je pense qu'on va faire notre première vraie évolution d'architecture gfx intéressante.


## TL;DR

### Todolist

Ouais, ta liste mélange actuellement **features**, **cleanup**, **évolutions d'architecture** et **gros nouveaux chapitres Vulkan** au même niveau. Du coup on a l'impression que `stdout dehors` et `swapchain lifecycle` sont deux tâches comparables alors que pas du tout. 😭

Je la structurerais comme ça, avec un ordre qui garde le projet fun **et** fait émerger l'architecture au moment où elle devient nécessaire :

- [x] **Milestone 1 — Premier vrai particle renderer**

  - [x] Chargement image → particules
  - [x] Camera 2D / fit / pan / zoom
  - [x] `POINT_LIST`
  - [x] Couleur par particule
  - [x] Animation CPU + upload GPU
  - [x] `largePoints`
  - [x] Particules circulaires avec `gl_PointCoord`
  - [x] ~1.9 M particules fluides en implémentation naïve

- [ ] **Milestone 2 — Pixel Storm interactif côté CPU**
  - [x] Remplacer le retour simpliste par une vraie simulation
    - [x] force de retour vers `origin`
    - [x] `velocity`
    - [x] damping / friction
    - [x] `dt` clamp raisonnable
  - [ ] Répulsion souris
    - [ ] récupérer position souris SDL
    - [ ] `Camera2D::screenToWorld()`
    - [ ] rayon d'influence
    - [ ] test distance² sans `sqrt` si inutile
    - [ ] force de répulsion
    - [ ] activation au clic / état input
  - [ ] Garder `randomize()` comme action de test
  - [ ] Vérifier comportement à 500k / 1M / 2M particules
- [ ] **Milestone 3 — Petit cleanup avant de toucher l'architecture**
  - [ ] `app/config.hpp`
    - [ ] sortir les constantes Vulkan de la couche `app`
    - [ ] supprimer la dépendance `renderer/vulkan → app`
  - [ ] `ParticleSystem`
    - [ ] stdout dehors
    - [ ] erreur explicite si image → 0 particule
    - [ ] remplacer `std::rand()`
    - [ ] includes explicites
  - [ ] commentaires
    - [ ] supprimer Doxygen qui répète les signatures
    - [ ] conserver les commentaires expliquant des contraintes Vulkan
  - [ ] renommer `GraphicsPipeline` → `ParticlePipeline`
  - [ ] **ne rien généraliser davantage à ce stade**
- [ ] **Milestone 4 — Clarifier CPU state vs GPU resources**
  - [ ] Décider explicitement ce qui appartient à `ParticleSystem`
    - [ ] état simulation CPU
    - [ ] `position`
    - [ ] `origin`
    - [ ] `velocity`
    - [ ] apparence logique
  - [ ] Introduire une représentation GPU des particules
    - [ ] ne plus considérer implicitement `Particle` comme format GPU universel
    - [ ] préparer des layouts adaptés au renderer
  - [ ] Sortir l'ownership du particle buffer de `Renderer`
  - [ ] Introduire une petite ressource sémantique type `ParticleBuffer` / `ParticleGpuData`
    - [ ] création des buffers
    - [ ] upload
    - [ ] count
    - [ ] handles Vulkan nécessaires au draw
  - [ ] Faire en sorte que `Renderer` consomme une ressource GPU déjà prête au lieu de recevoir les `Particle` CPU
  - [ ] Garder `App` comme orchestrateur :
    - [ ] simulation
    - [ ] update ressource GPU
    - [ ] render

- [ ] **Milestone 5 — Faire évoluer `Buffer` proprement**
  - [ ] Garder `Buffer::write()`
  - [ ] Ajouter `map()`
  - [ ] Ajouter `unmap()`
  - [ ] gérer proprement l'état mapped/unmapped
  - [ ] envisager persistent mapping pour les buffers mis à jour chaque frame
  - [ ] stocker le `VkDevice` nécessaire plutôt qu'une dépendance C++ inutile vers `Device`
  - [ ] distinguer propriétés mémoire demandées / réellement obtenues
  - [ ] éventuellement ajouter `offset` à `write()` lorsqu'un use-case apparaît
  - [ ] **pas encore** de gros allocator Vulkan maison

- [ ] **Milestone 6 — Arrêter de transférer des données inutiles**
  - [ ] Instrumenter avant de modifier
    - [ ] temps `ParticleSystem::update()`
    - [ ] temps upload / `Buffer::write()`
    - [ ] frame time total
  - [ ] Séparer données dynamiques et statiques si les mesures le justifient
    - [ ] positions dynamiques
    - [ ] couleurs statiques / rarement modifiées
  - [ ] Ne réuploader les couleurs que lorsqu'elles changent
  - [ ] prévoir un dirty flag pour les changements de palette / grayscale / white
  - [ ] étudier `VK_FORMAT_R8G8B8A8_UNORM` au lieu d'un `glm::vec4` de 16 B
  - [ ] mesurer à nouveau
  - [ ] établir un vrai benchmark 500k / 1M / 2M / 4M / 6M / etc.

- [ ] **Milestone 7 — Robustesse Vulkan / renderer plus général**
  - [ ] Validation layers
  - [ ] Debug messenger
  - [ ] Gestion resize SDL
  - [ ] `Camera2D::setViewSize()` au resize
  - [ ] Swapchain recreation
    - [ ] `VK_ERROR_OUT_OF_DATE_KHR`
    - [ ] `VK_SUBOPTIMAL_KHR`
    - [ ] minimize / restore
    - [ ] recréation des ressources dépendantes
  - [ ] seulement lorsque plusieurs pipelines existent :
    - [ ] identifier ce qui est vraiment commun
    - [ ] extraire les abstractions génériques utiles
  - [ ] ne pas fabriquer un `GraphicsPipelineBuilderFactoryManager™` avant d'avoir plusieurs vrais besoins

- [ ] **Milestone 8 — Premier compute shader**
  - [ ] Détecter le support compute de la graphics queue existante
  - [ ] Introduire les storage buffers
  - [ ] Descriptor set/layout minimal
  - [ ] Compute pipeline
  - [ ] Un work item par particule
  - [ ] `vkCmdDispatch`
  - [ ] simulation position / velocity / retour origin sur GPU
  - [ ] répulsion souris sur GPU
  - [ ] barrière compute → graphics
  - [ ] réutiliser directement les données produites par compute pour le draw
  - [ ] supprimer l'upload CPU des positions chaque frame
  - [ ] benchmark CPU vs GPU

- [ ] **Milestone 9 — Compute avancé, seulement pour apprendre / mesurer**
  - [ ] Étudier une dedicated compute queue
  - [ ] comparer avec compute sur graphics queue
  - [ ] apprendre cross-queue synchronization
  - [ ] queue-family ownership si nécessaire
  - [ ] vérifier si ça apporte réellement quelque chose
  - [ ] expérimenter avec partition spatiale seulement si le test souris sur toutes les particules devient un vrai bottleneck

- [ ] **Milestone 10 — Transformer le lab en petite base gfx 2D propre**
  - [ ] Particle pipeline
  - [ ] Sprite/quad pipeline
  - [ ] textures / UV
  - [ ] transformations 2D
  - [ ] batching adapté aux vrais besoins
  - [ ] draw/resource representation commune uniquement là où elle apporte quelque chose
  - [ ] documentation des choix d'architecture
  - [ ] conserver Vulkan visible au lieu de le masquer derrière une pseudo-Raylib maison

Le point important est que **`Particle : CPU/GPU split`, `Buffer map/unmap` et `Renderer ownership` ne sont pas du “petit cleanup”**. Ce sont notre **Milestone architecture gfx**. Je les sortirais totalement de la rubrique cleanup.

Et notre chemin immédiat devient extrêmement simple :

```text
maintenant
   ↓
mouse repulsion + vraie physique CPU
   ↓
mini-cleanup
   ↓
ParticleGpuData / ownership Renderer
   ↓
Buffer mapping
   ↓
mesures + réduction des uploads
   ↓
compute
```
