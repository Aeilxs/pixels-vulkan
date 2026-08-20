# Pixel Storm — Preliminary benchmark

> First exploratory benchmark before the text/debug overlay is implemented.  
> This is **not** the final optimization baseline; it is mainly a sanity check for the frame instrumentation and a template for the later benchmark campaign.

## Test environment

| Parameter           | Value                           |
|---------------------|---------------------------------|
| Machine / GPU       | Apple M4                        |
| Build configuration | Release                         |
| Compiler            | AppleClang 21.0.0.21000099      |
| Commit              | `2b725486` (`dirty`)            |
| Logical window size | 1200 × 800                      |
| Swapchain extent    | 2400 × 1600                     |
| Swapchain images    | 3                               |
| Present mode        | `VK_PRESENT_MODE_IMMEDIATE_KHR` |
| Validation layers   | Disabled                        |
| Gap                 | 1                               |
| Uncapped            | Yes                             |

`--uncapped` was added because the normal `VK_PRESENT_MODE_FIFO_KHR` path capped the application at the display refresh rate (~60 FPS / ~16.67 ms). For throughput measurements, `VK_PRESENT_MODE_IMMEDIATE_KHR` is used when explicitly requested so the benchmark can exceed the display refresh rate.

## Workloads

The benchmark images are deterministic, fully opaque RGBA PNGs generated specifically for this test. With `gap = 1`, each image pixel produces one particle.

| Target |  Image size | Actual particle count |
|-------:|------------:|----------------------:|
|  ~500k |   707 × 707 |               499,849 |
|    ~1M | 1024 × 1024 |             1,048,576 |
|    ~2M | 1448 × 1448 |             2,096,704 |
|    ~4M | 2048 × 2048 |             4,194,304 |

## Measurement method

Each displayed metric is already aggregated over an approximately 500 ms statistics window. For this summary:

- the first metrics sample of each run is ignored as a small warm-up;
- the table reports the **median** of the remaining samples;
- one run was performed per workload;
- the mouse was moved manually during the run, so the simulation workload is representative but not perfectly deterministic.

The measured timings are:

- **Frame wall** — elapsed host wall-clock time for one application frame;
- **Simulation** — elapsed time around `ParticleSystem::update()`;
- **Upload** — elapsed host time spent updating the particle buffer;
- **Fence** — time spent blocked in `vkWaitForFences()`.

`Fence` is **not GPU execution time**. Actual GPU time will require Vulkan timestamp queries later.

## Results

| Particles | Median FPS |   Frame wall | Simulation |  Upload |    Fence |
|----------:|-----------:|-------------:|-----------:|--------:|---------:|
|   499,849 |  **120.6** |  **8.29 ms** |    1.64 ms | 1.35 ms |  4.99 ms |
| 1,048,576 |   **95.1** | **10.52 ms** |    2.51 ms | 1.89 ms |  5.83 ms |
| 2,096,704 |   **51.9** | **19.26 ms** |    4.14 ms | 3.59 ms | 11.29 ms |
| 4,194,304 |   **49.4** | **20.23 ms** |    6.27 ms | 3.85 ms |  9.89 ms |

## Observations

The uncapped present mode is necessary for the lighter workloads: the ~500k workload reaches roughly **120 FPS**, whereas the FIFO path previously sat at ~60 FPS and therefore mostly measured display pacing rather than application throughput.

Simulation cost grows clearly with the number of particles, from about **1.64 ms at 500k** to **6.27 ms at 4M**. This is expected from the current CPU-side simulation, which iterates over all particles every frame.

The particle upload is also a visible cost. The current implementation updates the full particle structure every frame through the host-visible buffer path. This is intentionally naive and gives us a useful reference point for the later persistent-mapping and CPU→GPU traffic work.

The fence timing should not be interpreted as GPU rendering time. CPU work and GPU work overlap, so increasing CPU-side work can leave the GPU more time to progress before the CPU reaches the next fence. This is visible between the ~2M and ~4M runs: simulation becomes more expensive while the measured fence wait decreases slightly.

The ~500k and ~1M workloads show noticeably more variance than the heavier workloads. The ~4M run is comparatively stable around 49–50 FPS. Since this is only a preliminary run with manual mouse movement and a single pass per workload, these numbers are sufficient for instrumentation validation but should not be treated as a rigorous final baseline.

## Final baseline checklist

When the text/debug overlay is implemented, repeat the campaign with:

- the same generated benchmark images;
- `gap = 1`;
- Release build;
- validation layers disabled;
- `--uncapped`;
- the same logical window size and swapchain extent;
- the same machine/GPU;
- a clean, committed revision;
- the overlay enabled;
- several runs per workload;
- a short warm-up excluded from the results.

The final baseline should then be reused unchanged for before/after optimization comparisons.
