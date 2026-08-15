# Camera 2D — notes de référence

Ce fichier documente **la caméra réellement implémentée dans `camera_2d.cpp`**.

Notre convention 2D est :

```text
(0, 0) ─────────────► +X
  |
  |
  ▼
 +Y
```

Donc :

- `X` augmente vers la droite ;
- `Y` augmente vers le bas ;
- les positions du programme restent en coordonnées « humaines » ;
- la conversion vers l'espace attendu par Vulkan est faite par la caméra et le vertex shader.

---

## 1. État minimal de la caméra

La caméra stocke seulement trois informations utiles :

```cpp
glm::vec2 viewSize_{};
glm::vec2 center_{};
float zoom_{1.0F};
```

Mathématiquement :

- `viewSize_ = (W, H)` est la taille logique de la vue ;
- `center_ = C = (C_x, C_y)` est le point du monde affiché au centre de l'écran ;
- `zoom_ = Z` est le facteur de zoom.

On impose :

$$
W > 0, \qquad H > 0, \qquad Z > 0
$$

Un zoom de `1` signifie une échelle naturelle. Un zoom de `2` affiche les objets deux fois plus gros. Un zoom de `0.5` les affiche deux fois plus petits.

---

## 2. Construction : garder la caméra dans un état valide

```cpp
Camera2D::Camera2D(glm::vec2 viewSize, float zoom) {
    setViewSize(viewSize);
    setZoom(zoom);
}
```

On passe volontairement par les setters afin de réutiliser les mêmes validations :

```cpp
void Camera2D::setViewSize(glm::vec2 viewSize) {
    if (viewSize.x <= 0.0F || viewSize.y <= 0.0F) {
        throw std::invalid_argument("Camera view size must be greater than 0.");
    }

    viewSize_ = viewSize;
}
```

```cpp
void Camera2D::setZoom(float zoom) {
    if (zoom <= 0.0F) {
        throw std::invalid_argument("Camera zoom must be greater than 0.");
    }

    zoom_ = zoom;
}
```

Les contraintes correspondent directement aux divisions utilisées plus tard :

$$
\frac{1}{W}, \qquad \frac{1}{H}, \qquad \frac{1}{Z}
$$

Une largeur, une hauteur ou un zoom nul rendrait donc la transformation invalide.

---

## 3. Le centre : déplacer la caméra sans déplacer le monde

```cpp
void Camera2D::setCenter(glm::vec2 center) noexcept {
    center_ = center;
}
```

Si un point du monde vaut :

$$
P = (P_x, P_y)
$$

et le centre caméra :

$$
C = (C_x, C_y)
$$

la première opération consiste à exprimer le point **relativement à la caméra** :

$$
P_{view} = P - C
$$

soit :

$$
x_{view} = P_x - C_x
$$

$$
y_{view} = P_y - C_y
$$

Exemple :

$$
P = (1300, 400), \qquad C = (1200, 300)
$$

alors :

$$
P_{view} = (100, 100)
$$

Le point est vu 100 unités à droite et 100 unités sous le centre de la caméra.

C'est le principe du déplacement de caméra : **le monde ne bouge pas, c'est le repère depuis lequel on le regarde qui change**.

---

## 4. Le zoom : combien de monde est visible ?

Dans `viewProjection()`, on commence par calculer la demi-taille visible :

```cpp
const glm::vec2 halfView = viewSize_ / (2.0F * zoom_);
```

En X :

$$
halfWidth = \frac{W}{2Z}
$$

En Y :

$$
halfHeight = \frac{H}{2Z}
$$

La taille totale du monde visible vaut donc :

$$
visibleWidth = \frac{W}{Z}
$$

$$
visibleHeight = \frac{H}{Z}
$$

C'est ce qui donne la sémantique naturelle du zoom.

Pour une vue de 800 pixels de large :

- avec $Z = 1$, on voit 800 unités de monde ;
- avec $Z = 2$, on ne voit plus que 400 unités ;
- avec $Z = 0.5$, on voit 1600 unités.

Donc augmenter `zoom_` signifie bien **zoom in**.

---

## 5. Projection orthographique

La projection est construite ainsi :

```cpp
const glm::mat4 projection = glm::ortho(
    -halfView.x,
    halfView.x,
    -halfView.y,
    halfView.y,
    -1.0F,
    1.0F
);
```

On définit donc un rectangle visible centré autour de l'origine de la caméra :

$$
left = -\frac{W}{2Z}
$$

$$
right = +\frac{W}{2Z}
$$

$$
top = -\frac{H}{2Z}
$$

$$
bottom = +\frac{H}{2Z}
$$

La projection orthographique transforme ce rectangle vers la zone normalisée visible par Vulkan en X/Y :

$$
[-1, +1]
$$

Le facteur d'échelle horizontal est :

$$
\frac{2}{right-left}
$$

Or :

$$
right-left
=
\frac{W}{2Z} - \left(-\frac{W}{2Z}\right)
=
\frac{W}{Z}
$$

Donc :

$$
\frac{2}{right-left}
=
\frac{2Z}{W}
$$

Même raisonnement verticalement :

$$
\frac{2Z}{H}
$$

On retrouve donc directement :

$$
x_{ndc} = x_{view}\frac{2Z}{W}
$$

$$
y_{ndc} = y_{view}\frac{2Z}{H}
$$

---

## 6. La matrice de vue

Le code construit ensuite la matrice de vue :

```cpp
const glm::mat4 view = glm::translate(
    glm::mat4{1.0F},
    glm::vec3{-center_, 0.0F}
);
```

Pourquoi `-center_` ?

Parce qu'on veut faire :

$$
P_{view} = P - C
$$

Une translation matricielle de $-C$ produit exactement ce résultat.

Si :

$$
C = (1200, 300)
$$

la matrice applique une translation de :

$$
(-1200, -300)
$$

Ainsi le point monde situé exactement au centre de la caméra devient :

$$
(1200,300) + (-1200,-300) = (0,0)
$$

---

## 7. `viewProjection()` : la transformation complète

L'implémentation complète est :

```cpp
glm::mat4 Camera2D::viewProjection() const noexcept {
    const glm::vec2 halfView = viewSize_ / (2.0F * zoom_);

    const glm::mat4 projection = glm::ortho(
        -halfView.x,
        halfView.x,
        -halfView.y,
        halfView.y,
        -1.0F,
        1.0F
    );

    const glm::mat4 view = glm::translate(
        glm::mat4{1.0F},
        glm::vec3{-center_, 0.0F}
    );

    return projection * view;
}
```

La formule générale est :

$$
P_{clip} = Projection \times View \times P_{world}
$$

On lit les opérations de droite à gauche :

```text
World position
    ↓ View
position relative à la caméra
    ↓ Projection
clip space / NDC
```

Pour notre caméra 2D, cela revient à :

$$
\boxed{
P_{ndc}
=
(P-C)
\cdot
\left(
\frac{2Z}{W},
\frac{2Z}{H}
\right)
}
$$

C'est la formule centrale de la caméra.

---

## 8. Pourquoi utiliser une matrice si la formule est si simple ?

On pourrait écrire directement dans le shader :

$$
(P-C) \cdot \frac{2Z}{ViewSize}
$$

Mais les matrices permettent de composer naturellement les transformations graphiques.

Aujourd'hui on a :

$$
Projection \times View
$$

Plus tard, un objet peut avoir sa propre transformation `Model` :

$$
Projection \times View \times Model
$$

Le principe reste le même.

Le vertex shader peut alors rester simple :

```glsl
gl_Position =
    viewProjection *
    vec4(inPosition, 0.0, 1.0);
```

Le code CPU travaille en coordonnées monde ; le shader applique la transformation vers l'espace attendu par Vulkan.

---

## 9. `screenToWorld()` : faire le chemin inverse

L'implémentation est volontairement courte :

```cpp
glm::vec2 Camera2D::screenToWorld(glm::vec2 screenPosition) const noexcept {
    return center_ + (screenPosition - viewSize_ * 0.5F) / zoom_;
}
```

Une position écran utilise le coin supérieur gauche comme origine.

Le centre de l'écran vaut :

$$
\frac{ViewSize}{2}
$$

On commence donc par recentrer la position :

$$
ScreenRelative
=
Screen - \frac{ViewSize}{2}
$$

Puis on annule le zoom :

$$
WorldRelative
=
\frac{ScreenRelative}{Z}
$$

Enfin, on remet le centre caméra :

$$
\boxed{
World
=
C
+
\frac{
Screen - ViewSize/2
}{Z}
}
$$

### Exemple

Vue :

$$
800 \times 600
$$

Centre caméra :

$$
C=(100,50)
$$

Zoom :

$$
Z=2
$$

Souris :

$$
Screen=(500,300)
$$

Le centre écran est :

$$
(400,300)
$$

La souris est donc 100 pixels à droite du centre :

$$
(500,300)-(400,300)=(100,0)
$$

Avec un zoom de 2, cela représente seulement 50 unités monde :

$$
\frac{(100,0)}{2}=(50,0)
$$

Donc :

$$
World=(100,50)+(50,0)=(150,50)
$$

---

## 10. Pourquoi il n'y a pas d'inversion de Y dans `screenToWorld()`

Notre convention monde est :

```text
+X → droite
+Y → bas
```

Les coordonnées écran utilisées par SDL suivent également cette convention.

Avec un viewport Vulkan de hauteur positive, NDC `y = -1` correspond au haut et NDC `y = +1` au bas du viewport.

Notre projection peut donc conserver directement :

```text
world Y down
    ↓
screen Y down
```

On n'a pas besoin d'ajouter un `-y` artificiel dans la caméra.

C'est volontaire : une convention unique évite de devoir se demander plus tard où l'axe Y a été retourné.

---

## 11. `fit()` : faire tenir un rectangle dans la vue

L'implémentation est :

```cpp
void Camera2D::fit(glm::vec2 contentCenter, glm::vec2 contentSize, float fill) {
    if (contentSize.x <= 0.0F || contentSize.y <= 0.0F) {
        throw std::invalid_argument("Content size must be greater than 0.");
    }
    if (fill <= 0.0F || fill > 1.0F) {
        throw std::invalid_argument("Fill factor must be in the range (0, 1].");
    }

    const float horizontalZoom = viewSize_.x / contentSize.x;
    const float verticalZoom = viewSize_.y / contentSize.y;

    setCenter(contentCenter);
    setZoom(std::min(horizontalZoom, verticalZoom) * fill);
}
```

Pour faire tenir un contenu de largeur $C_W$ dans une vue de largeur $V_W$, le zoom maximal est :

$$
Z_x = \frac{V_W}{C_W}
$$

Verticalement :

$$
Z_y = \frac{V_H}{C_H}
$$

Il faut respecter **les deux contraintes**. On prend donc :

$$
Z = \min(Z_x, Z_y)
$$

Sinon une dimension rentrerait dans l'écran tandis que l'autre serait coupée.

Le paramètre `fill` ajoute une marge :

$$
Z_{final}
=
\min(Z_x,Z_y) \times fill
$$

Avec :

$$
0 < fill \le 1
$$

Par exemple `fill = 0.9` utilise 90 % de l'espace disponible.

Le `contentCenter` devient simplement le centre de la caméra.

---

## 12. Taille logique de la caméra et taille physique Vulkan

`viewSize_` représente une **taille logique** utile aux coordonnées du programme et aux entrées utilisateur.

Sur un écran HiDPI, on peut avoir par exemple :

```text
logical window size:   1280 × 720
framebuffer physique:  2560 × 1440
```

La caméra raisonne avec la première taille.

Vulkan utilise la seconde pour le swapchain, le viewport et le scissor.

On évite donc de faire dépendre `Camera2D` de `VkExtent2D`.

La séparation est :

```text
Camera2D
    → maths en coordonnées logiques

Renderer Vulkan
    → framebuffer physique
```

---

## 13. Où la matrice part dans Vulkan ?

La caméra ne connaît pas Vulkan. Elle produit seulement :

```cpp
glm::mat4 matrix = camera.viewProjection();
```

Le renderer peut ensuite envoyer cette matrice au vertex shader, par exemple avec une push constant :

```cpp
vkCmdPushConstants(
    commandBuffer,
    pipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT,
    0,
    sizeof(glm::mat4),
    &matrix
);
```

Une `mat4` contient 16 `float`, donc :

$$
16 \times 4 = 64\text{ octets}
$$

Changer le zoom ou le centre caméra ne demande donc pas de recalculer toutes les positions CPU ni de reconstruire un vertex buffer.

On met à jour la matrice, puis le GPU applique la nouvelle transformation à chaque vertex.

---

## 14. Résumé rapide

État caméra :

$$
C = center, \qquad Z = zoom, \qquad ViewSize=(W,H)
$$

Position relative à la caméra :

$$
P_{view}=P-C
$$

Taille visible :

$$
VisibleSize = \frac{ViewSize}{Z}
$$

World vers NDC :

$$
\boxed{
P_{ndc}
=
(P-C)
\cdot
\left(
\frac{2Z}{W},
\frac{2Z}{H}
\right)
}
$$

Matrices :

$$
View = Translate(-C)
$$

$$
ViewProjection = Projection \times View
$$

Écran vers monde :

$$
\boxed{
World
=
C
+
\frac{
Screen-ViewSize/2
}{Z}
}
$$

`fit()` :

$$
\boxed{
Z
=
\min
\left(
\frac{V_W}{C_W},
\frac{V_H}{C_H}
\right)
\times fill
}
$$

Pipeline mental :

```text
World coordinates
      ↓
View: translation de -camera.center
      ↓
Projection orthographique + zoom
      ↓
Vertex shader: gl_Position
      ↓
Clip / NDC
      ↓
VkViewport
      ↓
Framebuffer
```
