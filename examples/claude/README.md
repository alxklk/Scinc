# Claude examples

Example programs written in Scinc and verified to run with the `Scinc`
interpreter (`Scinc <file.cpp>` from the file's own directory). Organised into
three sections.

## simple/ — plain console programs (`stdio.h` only)

| File | What it does |
|------|--------------|
| `hello.cpp` | "Hello, World!" |
| `99bottles.cpp` | The "99 Bottles of Beer" song (299 lines) |
| `fizzbuzz.cpp` | FizzBuzz for 1..100 |
| `fibonacci.cpp` | First 20 Fibonacci numbers (iterative) |
| `factorial.cpp` | Recursive factorial, 1!..12! |
| `primes.cpp` | Sieve of Eratosthenes, primes below 100 |

## sound/ — audio (intrinsic `sound.h`)

| File | What it does |
|------|--------------|
| `whitenoise.cpp` | White-noise generator via the `CSound` API; plays ~3 s |
| `fractal_melody.cpp` | 4-track generative synth: a Thue-Morse fractal drives lead (2-op FM), harmony and bass at three time scales, over a drum track; A-minor-pentatonic, ping-pong delay; sound-only, runs until Ctrl-C |
| `fractal_melody_scope.cpp` | The same synth with a window: oscilloscope, 16-step beat grid, a "fractal roll" plotting the lead/bass walk, and per-track VU meters. Tempo & scale are set on the CLI (`Scinc fractal_melody_scope.cpp [bpm] [scale]`) or live (keys `1`-`5` scale, `-`/`+` tempo) |
| `track_editor.cpp` | A 4-track visual melody editor (in the spirit of `examples/sound/melody_matrix.cpp`, but each grid is an independent instrument track). Paint 16-step patterns with the mouse; click a track header to cycle its instrument through 8 synthesized voices — EPiano/Bell (2-op FM), Pluck (Karplus-Strong), Square, Pad (detuned saws), Organ (additive), SubBass, and Drums (row picks kick/snare/hat/clap). `T` random, `C` clear, `-`/`+` tempo |

## graphic/ — animation (intrinsic `graphics.h` + `../../ws.h`)

| File | What it does |
|------|--------------|
| `lissajous.cpp` | Animated 3:2 Lissajous curve; runs until the window is closed |
| `raytrace.cpp` | Ray-traced metal still life: silver sphere, bronze cube (balanced on one vertex, space diagonal vertical) and gold torus on a checkerboard plane (reflectance 0.3), off-axis camera; resolution from the command line (`Scinc raytrace.cpp [w h]`, default 640×480); renders one frame with 2×2 AA, shows it, and saves `raytrace.png` |

## opengl/ — hardware 3D via a native shared-library wrapper

`scgl.h` wraps OpenGL 1.x (fixed function, display lists, lighting, textures,
`gluPerspective`/`gluLookAt` reimplemented without GLU) plus an SDL2 window with keyboard/mouse input. It
follows the `[[scinc::dynlink]]` + `ScincBoundCall` pattern of
`examples/video/opencv_cam_in*` and `examples/dynamic_native/fileio_wrap.cpp`:
one `WRAP(...)` list in `scgl.h` is both the Scinc-side declarations and,
compiled from `scgl_dl.cpp`, the exported native wrappers. Build the library
once with `./build_so.sh` (needs `libsdl2-dev libgl-dev`).

| File | What it does |
|------|--------------|
| `gears.cpp` | The classic glxgears: three lit gears in display lists, resizable window, FPS every 5 s. Arrows / mouse-drag rotate the view, `z`/`Z` roll, Esc/Q quit |
| `gears_glsl.cpp` | The glxgears scene with per-pixel lighting from inline GLSL: warm key, cool fill and rim lights; coloured metal (Schlick Fresnel tints highlights and reflections), reflections of a procedural photo studio, lathe-turned front faces (concentric grooves with an anisotropic Kajiya-Kay highlight, antialiased with `fwidth`), ACES tone mapping + gamma, gradient backdrop, 4x MSAA. Tooth faces carry their own normals since shader varyings are always interpolated. Arrows/drag rotate, `z`/`Z` roll, wheel zoom, `L` key-light orbit, space pause, `H` home, `F` fullscreen, `S` save |
| `tubes.cpp` | "3D pipes": glossy tubes grow cell by cell through a 14^3 grid, mostly straight, turning at random into free cells with ball joints, never crossing; boxed-in pipes get a cap and a new pipe starts in a new colour; a full scene holds for 2.5 s and starts over. Unit cylinder / sphere display lists placed with translate-rotate-scale, newest segments grow smoothly; per-pixel inline-GLSL lighting (key/fill/rim, Fresnel studio reflections, ACES), gradient backdrop, 4x MSAA. Drag orbit, wheel zoom, `[`/`]` pipes 1..8, `-`/`+` speed, `N` new, space pause, `H` home, `F` fullscreen, `S` save |
| `flurry.cpp` | Port of xscreensaver's **Flurry** (Calum Robinson, BSD license kept in the file): a tumbling star emits smoke puffs towards orbiting sparks, puffs feel every spark's gravity, drag and widening, and are drawn as motion-stretched quads with an animated 8x8 speckled-smoke texture atlas, blended additively over the fading previous frame. Scalar code path translated line by line (physics, 13 colour modes, all 7 presets, texture generation); the particle simulation runs in Scinc at ~50 fps for every preset, including 64-stream "insane". Frames persist through a screen texture (`glCopyTexSubImage2D`) since a swapped back buffer isn't guaranteed to survive. Verified side by side against the original binary. The smoke texture generator lives in `flurry_texture.h` (with the BSD notice), shared with `ophiura_flurry.cpp`. Keys `1`-`7` presets (water, fire, psychedelic, rgb, binary, classic, insane), `R` random, `F` fullscreen, `S` save |
| `ophiura_flurry.cpp` | The brittle star drawn in Flurry's style: same arm walk and colours as `geometry/ophiura.cpp`, but every bead is a Flurry smoke puff (animated smoke-atlas quad stretched along its motion since the last frame, additive over the fading previous frame), with a brighter core along the segment bands and smoke shed from the arm tips that drifts, widens and fades. A subtractive `glBlendEquation` step clears the 8-bit fade residue, so trails die out to pure black. Keys `-`/`+` arms, `[`/`]` trail length, `T` tip smoke, `C` clear, space pause, `F` fullscreen, `S` save |
| `ifs.cpp` | Port of xscreensaver's **ifs** (MIT-style notice kept): 3 mutating lenses (rotation/scale/translation) drawn by the chaos game, multi-colour mode with a smooth cyclic colour map, all following the original. Each point is a soft round spot with the `ophiura_2d` Circle profile, rendered as a point sprite shaped in an inline shader (`gl_PointCoord`), so Scinc writes only x, y per spot: ~19.7k spots at ~115 fps. Additive "glow" mode shows the fractal's density; `G` switches to the original's flat look. Click/`N` new lenses, `-`/`+` functions 2..6, `[`/`]` detail, `,`/`.` spot size, `M` multi-colour, `C` colours, `T` trails, space pause, `F` fullscreen, `S` save |
| `raytrace_gl.cpp` + `fullscreen.vert` + `raytrace.frag` | GLSL port of `graphic/raytrace.cpp`: the same metal still life ray traced per pixel in a fragment shader on a fullscreen quad, in real time. Scinc owns the scene and camera and passes them as uniforms; the camera orbits and the cube pirouettes on its vertex. Mouse drag orbit, wheel zoom, space pause, `H` home (the exact `raytrace.cpp` view), `1`-`4` AA level, `F` fullscreen, `S` save `raytrace_gl.bmp`, `R` reload shaders (edit the `.frag` while it runs). `Scinc raytrace_gl.cpp [w h]` |
| `func_graph_3d.cpp` | OpenGL version of `geometry/func_graph_3d.cpp`: an animated surface z = f(x, y) evaluated in Scinc each frame, drawn as a lit height field with a wireframe on top via vertex arrays (one `glDrawElements` for the fill, one for the wire). Keys `1`-`5` pick the function (ripple, egg crate, sinc, two-source interference, saddle), `M` fill/wire mode, `C` slope/height colours, `-`/`+` grid 20..160, mouse drag orbit, wheel zoom, space pause, `H` home, `F` fullscreen |
| `ophiura.cpp` | OpenGL port of `geometry/ophiura.cpp`, the wriggling brittle star. Arm motion and colours are computed exactly as in the original, but each of the ~800 beads is a real lit sphere (display list) resting on a procedurally textured seabed (mipmapped + anisotropic), with soft blob shadows cast away from the light, fog and 4x MSAA. Top view by default, mouse drag orbits into 3D, wheel zoom, `-`/`+` arms 3..10, `B` seabed blue-grey (as the original displays) / sand, space pause, `H` home, `F` fullscreen, `S` save `ophiura.bmp` |
| `ophiura_tube.cpp` | The same brittle star with smooth tube arms: each arm's bead positions become the centreline of one tube mesh (rings of 16 vertices, radius and colour per ring from the original's thickness and band formulas, rounded end caps), rebuilt by Scinc every frame and drawn with a single `glDrawElements`. Same controls as `ophiura.cpp`; runs at the vsync limit even with 10 arms (~22k vertices per frame) |
| `ophiura_2d.cpp` | Faithful 2D port of `geometry/ophiura.cpp`: every one of the original's soft round spots (`g.Circle`: body, shadow, highlight; ~2400 per frame) is one quad whose round soft edge is computed per pixel by a small GLSL shader (embedded in the source as string literals), using the native `Circle(x, y, R, W, A, col)` profile measured from the interpreter (ring radius R, half-width W, linear fade over A). Same arm walk, colours, opacities and draw order; matches the original frame for frame (RMSE 2.9% at t = 10) but stays smooth at any size. By default the arms are walked in 2x finer steps (~4.9k spots), which smooths the beaded outline; shadow/highlight opacity is rescaled to `1-(1-a)^(1/k)` so the overlaps add up to the original's; `[`/`]` set density 1, 2 or 4 (1 = the original exactly). `Scinc ophiura_2d.cpp [t]` freezes at time t; wheel zoom, `-`/`+` arms, `B` background, space pause, `H` home, `F` fullscreen, `S` save |

Shaders: `glwLoadProgram(vsFile, fsFile)` / `glwCreateProgram(vsSrc, fsSrc)`
compile and link a GLSL program and return its id, or 0 after printing the
compiler log; then `glUseProgram`, `glGetUniformLocation`, `glUniform1i/1f..4f`,
`glUniform1fv/3fv/4fv`, `glUniformMatrix4fv`. The GL 2.0 entry points are
resolved at runtime (`SDL_GL_GetProcAddress`), so no extra libraries are
needed. Run on the discrete GPU with `nvrun Scinc raytrace_gl.cpp`
(`__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia`).

Small shaders can live in the Scinc source as string literals: adjacent
literals concatenate and `\n` works, so `char* fs = "#version 120\n" "...\n";`
then `glwCreateProgram(vs, fs)` (see `ophiura_2d.cpp`); bigger ones such as
`raytrace.frag` stay in files for `glwLoadProgram` and live reloading.

Vertex arrays: `glwVertexArray(size, v)`, `glwNormalArray(n)`,
`glwColorArray(size, c)`, `glwTexCoordArray(size, t)` point GL straight at
Scinc `float[]` data (read in place as `GL_DOUBLE`), then `glDrawArrays` /
`glDrawElements(mode, count, int* indices)` and `glwDisableArrays()`. Set the
arrays right before drawing each frame. A wrapped call costs ~0.1 us, but a
mesh of 26k vertices drawn with `glVertex*` still means ~100k calls per frame;
with arrays `func_graph_3d` went from 14 to 48 FPS at 160x160.

Notes: Scinc `float` is a native `double`, so array arguments (`glLightfv`,
`glMaterialfv`, `glLoadMatrixf`...) take a Scinc `float[]` and are converted
in the wrapper. Functions that would return pointers or use out-params are
renamed with a `glw` prefix (`glwGetString(name, buf, len)`,
`glwGenTexture()`, `glwTexImage2D(w, h, int* argb)`).

All examples compile and run. The `simple/` ones just print; `sound/` plays
audio; `graphic/` opens a window (needs a display).

## Scinc quirks these examples work around

Found empirically while writing and running these:

- Only `int`, `float`, `double`, `char` types exist — no `long`, `short`,
  or `unsigned`. (`factorial.cpp` stops at 12! to stay within 32-bit `int`.)
- Array sizes must be literal/macro constants, not `const int` — hence
  `#define N 100` in `primes.cpp`.
- No comma-separated declarations with initializers (`int a = 0, b = 1;`
  fails) — declare each variable on its own line.
- A `char` array element read directly in a boolean context
  (`if (sieve[i])`) is currently always truthy, so `primes.cpp` uses an
  `int` sieve. Scalar `char`, `int` arrays, and explicit `!= 0` are fine.
- No `rand()`/`srand()` (roll your own PRNG — `whitenoise.cpp` uses a small
  LCG) and no `usleep()` (pace loops with the `Time()` intrinsic instead).
- No `floor()`/`fabs()` either — `raytrace.cpp` defines its own. `sqrt`, `pow`,
  `sin`, `cos`, `tan`, `fflush`, and `malloc` are available, and structs are
  passed/returned by value (used for the vector math in `raytrace.cpp`).
- `sound.h` and `graphics.h` are intrinsics (not real files); windowing
  helpers like `Present()` come from the real `ws.h` at the examples root,
  so a program two levels deep includes it as `../../ws.h`.
