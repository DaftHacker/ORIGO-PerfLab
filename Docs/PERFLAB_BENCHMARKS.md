# ORIGO PerfLab — Benchmark Scenarios and Methodology

## Goal

PerfLab is a separate ORIGO game project for creating repeatable workloads. It gives us a consistent way to answer whether an optimization improves the CPU side, GPU side, physics, transient effects, or frame-time stability.

The initial implementation is deliberately simple and uses the engine-owned default cube/default material so asset import or texture decoding is not mixed into steady-state object-count tests.

## Scenarios

### Static grid

Creates ordinary static mesh-renderer GameObjects in a roughly square X/Y grid. The population count is selectable at 1k, 5k, 10k and 25k from the in-game panel; scripted runs can go up to 50k.

This is a broad engine workload, not only a draw-call counter. It includes object storage, world-matrix cache checks, scene/render-object construction, existing render batching, per-instance view/distance tests, submission, and drawing visible cubes. A larger population is spread over a correspondingly larger area, so much of the added population may be outside the camera frustum; that is intentional for evaluating the current per-instance culling path.

### Instanced Static Mesh

Creates one GameObject with an Instanced Static Mesh component and places the requested number of default-cube transforms in that component. The in-game presets are 10k and 25k; scripted runs support up to 50k placements.

This scenario and the GameObject grid use equivalent repeated geometry/material and approximately the same spatial layout, but they differ in scene-object count. Compare `static -Count 10000` with `instances -Count 10000` on the same build and camera. The pair helps separate scene traversal and render-object construction from the cost of visiting and culling instance transforms. Both paths still use ORIGO's current renderer; this is not a GPU-driven culling test.

### Moving grid

Uses the same default cube mesh/material but changes each object's Z and rotation every update. It increases work for gameplay transform writes, world-matrix recomputation, dynamic submission and view-dependent renderer caches.

The current test uses deterministic phases derived from each object's index. The update is repeatable for equivalent start conditions, but exact frame timing still depends on hardware and system load.

### Physics stack

Creates box colliders and dynamic rigidbodies over a large static floor. The interactive presets are 100 and 500 bodies; scripted counts are restricted to 1,000 for safety.

This measures genuine Jolt-backed rigidbody work, not just the cost of drawing many cubes. Bigger populations may cause contacts and physics synchronization to dominate. Keep it separate from static/moving rendering baselines.

### Particles

Uses the Scene runtime transient-particle-burst API. The particle scenario emits repeated bursts so the approximate live particle population remains present throughout a capture. A separate one-shot burst control is useful for measuring burst spikes in editor play.

The requested count is an approximate steady-state target based on the burst interval and randomized lifetime. It is not a guarantee of exact live particle count, and it should not be compared to GPU-simulated particles without matching simulation and rendering semantics.

### Camera sweep

Moves the camera around the center of the benchmark field. This changes the percentage and identity of visible objects, rather than only measuring one fixed view.

Keep sweep disabled for a deterministic baseline. Use it as a separate test for camera motion and changing visibility. The original authored camera is restored when the sweep is turned off.

### Empty

Removes all PerfLab-generated population objects but leaves the authored camera/light. This is a useful low-workload control for measuring the runtime/editor host and profiler overhead.

## Scripted capture

`Tools/Capture-PerfLab.ps1` launches the supplied engine executable with the project descriptor and --play-only --auto-play. That avoids the second editor viewport and most editor tooling while retaining ORIGO's project/game module/runtime path.

The script sets:

- ORIGO_FRAME_PROFILE — path for the engine's text profile and viewport captures.
- ORIGO_FRAME_PROFILE_WARMUP — delay after the scene is ready before sampling.
- ORIGO_FRAME_PROFILE_SECONDS — measurement duration.
- ORIGO_FRAME_PROFILE_EXIT — request automatic exit after the capture.
- ORIGO_PERFLAB_SCENARIO / ORIGO_PERFLAB_COUNT — start-up workload selector.
- ORIGO_PERFLAB_HUD=0 — hide the in-game control panel for clean timing.
- ORIGO_PERFLAB_CAMERA_SWEEP — optionally animate the camera during the run.

The script also writes a sidecar metadata file. Add the engine commit, hardware/GPU driver, build configuration, resolution, VSync and graphics settings to that metadata before comparing results across machines.

Example:

~~~powershell
.\Tools\Capture-PerfLab.ps1 -EngineExe "D:\ProgrammingProjectsRepo\Origo-Engine\out\build\x64-relwithdebinfo\ORIGO_ENGINE.exe" -Scenario static -Count 10000 -WarmupSeconds 15 -CaptureSeconds 30
~~~

Do not use an editor debug build for headline throughput results. Detailed profiling should be used for diagnosis, then repeat the scenario without detailed profiling when possible.

## What to record

At minimum, preserve:

- Engine commit and build configuration.
- CPU, GPU and driver versions.
- Window/viewport resolution, VSync and renderer settings.
- Scenario, population count, camera-sweep state and capture duration.
- Median, p95 and p99 frame time if available, not just average FPS.
- Complete main-thread/editor-loop CPU time.
- Game update, physics, animation and render-object construction time.
- Per-view CPU renderer scopes and GPU timer-query values, kept as different measurements.
- Draw calls, triangles, visible instances, instances submitted and objects culled.
- Startup population-build duration and any later spikes.
- A viewport image from the same scenario.

Existing ORIGO profile fields and the reported counts can vary by engine commit. If the profile does not show GPU timer validity, treat the GPU time as unavailable, not zero. Do not add nested CPU scopes and GPU timer values into a fabricated “total frame time”.

## Suggested initial matrix

| Test | Scenario/count | Camera | Primary question |
|---|---|---|---|
| Low-workload control | Empty / 0 | Fixed | What is the runtime host/profiler baseline? |
| Small object baseline | Static / 1,000 | Fixed | Does the ordinary workload run smoothly? |\n| Instancing comparison | Static and Instances / 10,000 each | Fixed | What cost is removed by representing placements under one ISM component? |
| Main regression target | Static / 5,000 and 10,000 | Fixed | How does CPU traversal scale with population? |
| Large-scene stress | Static / 25,000 | Fixed | Does per-placement culling dominate? |
| Dynamic update | Moving / 1,000 and 5,000 | Fixed | How expensive are per-frame transform changes? |
| Changing visibility | Static / 10,000 | Sweep | How does view motion affect culling and caches? |
| Physics cost | Physics / 100 and 500 | Fixed | When do solver/contact/synchronization costs dominate? |
| Transient effects | Particles / 5,000 and 10,000 | Fixed | How do particle updates, sorting, uploads and rendering scale? |

Run each test more than once on the same warm machine. Compare the distributions, not isolated lucky frames.

## Relationship to chunk culling

This initial game does not implement hierarchical cell rejection. It gives us a baseline for the current engine path before changing it. The static grid is deliberately arranged so the scene population can increase while a relatively small region remains in the camera view. If total instance visitation is expensive, we expect the current flat culling loop to scale with the population even when GPU-visible geometry grows more slowly.

When a spatial hierarchy is prototyped, add an A/B switch and compare:

- Cells visited, outside, inside and intersecting.
- Individual placements tested after cell pruning.
- Hierarchy build/refit time and per-view query time.
- CPU culling time, GPU time, draw count and image equivalence.
- Camera sweep behavior and correctness when objects are edited or moved.

The goal is lower CPU cost without false-negative culling, stale bounds, missing geometry or incorrect shadow casters.
