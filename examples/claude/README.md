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

## sectorly/ — a small vector editor (intrinsic `graphics.h` + `../../ws.h`)

Pictures are built on a 64×64 grid from two primitives, each a directed stroke
whose filled body lies to the right of its direction: an axis-aligned line
with an attached square, and a quarter arc whose body is the pie, the corner
outside it, or the lens between the arc and its mirror. There is a 16-colour
palette (entry 0 is transparent) for fills and outlines.

| File | What it does |
|------|--------------|
| `primitives.cpp` | Static gallery of every primitive kind, with red arrows marking stroke direction; saves `primitives.png` |
| `sectorly.cpp` | The editor (`Scinc sectorly.cpp [file.svg]`). Drag to draw: near horizontal/vertical gives a line, near diagonal gives a sector. Select by click or rectangle; move, Ctrl+drag copy, copy/paste, duplicate, rotate 90° (R/Shift+R), delete, undo/redo; PgUp/PgDn move the selection in draw order (a scattered selection is gathered into a block first); an Edit tool (E) drags primitive ends, re-snapping to a line or sector; mouse-wheel zoom and middle-drag pan (Home fits); double-click a swatch to edit that palette colour with linked RGB/HSL sliders and a 2D square over the two channels the radio-selected one leaves free (saved in the SVG, undoable); resizable window at scale 2 (Scinc cannot resize a render target, so the window re-lays out while it fits its render target and is recreated only when it grows past it); a tool panel for palette and sector mode. Ctrl+S exports SVG with the model kept in an XML comment, and Ctrl+O loads it back. `-selftest` runs a non-interactive check |

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
| `boing.cpp` | Port of xscreensaver's **boing** (jwz's notice kept), the Amiga Boing demo: flat-shaded red/white ball bouncing and spinning in the purple grid room, shadow on the back wall, CRT scanlines, the original's physics (run in sub-steps at its 30 ms tick so speed matches on any display) and projection; verified side by side with the original. Plus what the original apologises for lacking: a synthesized boing on every bounce (pitch-dropping thump + knock), panned to the side it hits. `O` smooth, `L` lighting, `N` scanlines, `P` spin, `A` sound, `-`/`+` speed, drag rotates, `H` home, space pause, `F` fullscreen, `S` save |
| `lavalamp.cpp` | A lava lamp after xscreensaver's **lavalite** (jwz's notice kept): the 4 lamp styles (classic, giant, cone, rocket with fins), blob physics (groups of 4 metaballs on parabolic arcs with a leader, static pool and top blob), colours and lights follow the original. Instead of its CPU marching cubes, the glass is drawn with an inline fragment shader that ray-casts the same metaball field per pixel (surface at 0.3, glass fade-out, smooth normals) from 18 metaballs passed as uniforms, at the vsync limit; the same pass adds the specular glass reflection from the original's TODO list. Verified side by side with the original. `1`-`4` style, `B` launch a blob, `-`/`+` blob frequency, `G` glass reflection, drag orbit, wheel zoom, space pause, `H` home, `F` fullscreen, `S` save |
| `raytrace_gl.cpp` + `fullscreen.vert` + `raytrace.frag` | GLSL port of `graphic/raytrace.cpp`: the same metal still life ray traced per pixel in a fragment shader on a fullscreen quad, in real time. Scinc owns the scene and camera and passes them as uniforms; the camera orbits and the cube pirouettes on its vertex. Mouse drag orbit, wheel zoom, space pause, `H` home (the exact `raytrace.cpp` view), `1`-`4` AA level, `F` fullscreen, `S` save `raytrace_gl.bmp`, `R` reload shaders (edit the `.frag` while it runs). `Scinc raytrace_gl.cpp [w h]` |
| `ocean.cpp` + `ocean.frag` | Open sea under a procedural sky, ray marched per pixel in a fragment shader, from dawn through a moonlit night. Scinc builds 12 Gerstner waves from the wind (Pierson-Moskowitz peak wavelength, choppiness and foam rising with wind speed, phases advanced by `w = sqrt(g k)` so wind changes never make the sea jump), moves the sun and moon, and integrates sunlight through the same Rayleigh + Mie atmosphere as the shader for the sun colour, cloud lighting, sky ambient and exposure. The shader marches the wave height field, inverts the Gerstner shift with Newton steps, and shades the water with Fresnel sky reflection, sun or moon glitter from a Cox-Munk slope distribution (detail too fine for the pixel is filtered out and turned into glitter roughness, so the far sea does not alias), light through the crests and Jacobian foam; sky with drifting clouds, sun disc, moon and stars. Water reflections come from a 256x96 sky panorama rendered each frame by a first pass and copied to a texture. Drag look, wheel eye height, `[`/`]` time of day, `T` time lapse, `-`/`+` wind 2..20 m/s, `,`/`.` turn the wind, `C` cloud cover, `B` boat mode (the camera rides the swell, computed in Scinc), space pause, `H` home, `F` fullscreen, `S` save, `R` reload the shader. 960x540 at the vsync limit on Iris Xe; 1080p about 36 fps there, 84 fps on the RTX 2050 (`nvrun`). `Scinc ocean.cpp [w h]` |
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
- `float += int` fails (`y += 36` gives "Unknown or unsupported operator +=";
  write `y += 36.0`); int `+=`, including on struct members, is fine. No pointer arithmetic (`int* + int`;
  index a base pointer with an offset instead), and a `?:` that picks between
  two `char*` values crashes at runtime; use an `if`. A `?:` with a function
  call in either branch silently ends the program. Found in `sectorly.cpp`.

## tui/ — a Turbo Vision style text UI (ANSI terminal)

A desktop in the terminal, with overlapping windows, pull-down menus, a status
line, modal dialogs and keyboard and mouse control. Scinc draws it all with ANSI
escape sequences. A small native library, `term_dl.so`, adds what Scinc lacks for
terminals: raw mode, reading a key with a timeout, the terminal size, the time,
directory listing and saving files safely, and for t2fc below copying, moving and
deleting files step by step and running programs.

Run `./build_so.sh` once, then `Scinc tvdemo.cpp [file]` from this folder.

| File | What it does |
|------|--------------|
| `tv.h` | The framework: screen buffer that sends only changed cells, key and xterm mouse decoding, windows (move, resize, zoom, cascade, tile), menus, and button, check box, radio button, input line and list box controls |
| `tvedit.h` | The text editor: C/C++ syntax colours, selection by keyboard and mouse, undo and redo, clipboard, find and replace, go to line, auto-indent, UTF-8. It keeps line ends and permissions, saves through a temporary file, warns about unsaved changes and about files changed on disk, and opens as read-only any file it could not save unchanged |
| `tvdemo.cpp` | File browser and editor windows, calculator, calendar, ASCII table, sliding puzzle and a dialog using every control |
| `term.h`, `term_dl.cpp` | The native terminal library. It restores the terminal on exit, on a crash and on most signals |

`xfce4-terminal` and others may catch F1, F10 or Alt keys for their own menus. The
mouse and Ctrl+Q always work.

## t2fc/ — a two-panel file manager on the tui desktop

Panel windows in the manner of Midnight Commander, built from `../tui`: its
desktop, editor, hex viewer, theme editor and dialogs, and `term_dl.so` for the
file system. Build `../tui/build_so.sh` and this folder's `build_so.sh` (the
picture decoder `img_dl.so`) once (and `../opengl/build_so.sh` for the OpenGL
versions), then from this folder run `Scinc t2fc.cpp [left [right]]` in a
terminal. `t2fc_gfx.cpp`, `t2fc_raster.cpp`, `t2fc_small.cpp`, `t2fc_gl.cpp` and
`t2fc_gl_small.cpp` show the same thing in a graphic or OpenGL window, as tvdemo's
files of those names do. F1 shows every key.

| Keys | What they do |
|------|--------------|
| Tab, Enter, Backspace, letters | Other panel, enter a directory, the parent, jump to a name |
| Insert, right click, `+` `-` `*` | Mark, mark by pattern, unmark by pattern, invert |
| F3, F4, Shift+F4 | View (pictures, text read-only, binary in hex), edit, edit a new file |
| Alt+Q | Quick view in the other panel: the picture, text or hex dump of the file under the cursor |
| F5, F6, Shift+F6, F7, F8 | Copy, move, rename, make a directory, delete; with progress, Cancel, and questions before replacing |
| F2, Alt+Enter, Enter on an executable | Run a program (see below) |
| Ctrl+N | Another panel window (up to six), each with its own directories and modes |
| Alt+1..9, Alt+N, Alt+0 | Go to a window by number, from the Windows menu, or from the window list. Up to 12 windows are open besides dialogs, numbered 1..9, a, b, c; click a number on the menu bar to bring that window to the front |
| Ctrl+U, Alt+I, Ctrl+R, Alt+. | Swap panels, other panel here, reread, hidden files |

Clicking a column title sorts by it; the Left and Right menus choose full or
brief listings, quick view and the sort order, and Command > Compare directories
marks the files that differ.

**Pictures.** F3 on a picture opens a picture window, and quick view shows the
picture under the other panel's cursor. They are drawn as tvdemo's Mandelbrot is,
two pixels a character cell with half blocks, in 24-bit colour where the terminal
has it, in every backend. `img_dl.so` (`img_dl.cpp`) decodes them on threads of
its own, so the panels never wait for a big file: PNG, JPEG (turned upright by its
EXIF orientation), GIF and WebP (both animated), BMP, TGA, PSD, HDR and PNM, with
[stb_image](https://github.com/nothings/stb) (public domain, `stb_image.h` here)
and libwebp when it is installed. Shrinking averages all the pixels a cell
covers; clear parts show a checkerboard. In the window, `+` `-` or the wheel
zoom, `0` fits, `1` shows pixels 1:1, arrows or dragging move, Space and
Backspace go to the next and previous picture of the panel, and `r` turns it.

**Running programs.** In a terminal, t2fc gives the terminal to the program, as MC
does: the desktop steps aside, the program runs with the real screen and keyboard
(Ctrl+C reaches only the program), and a key brings the desktop back. A graphic
window has no terminal of its own, so there the program runs in a new terminal
window that stays open until Enter: `$TERMINAL`, then `x-terminal-emulator`,
`gnome-terminal`, `konsole`, `xfce4-terminal` and others are tried, or Options >
Preferences names one (as `xterm -e`). Command > Open with the desktop hands the
file to `xdg-open`. File names are quoted for the shell, and the panels are read
again afterwards.

Settings, including the directories of the panel window in front at exit, are kept
in `t2fc.cfg`; colour themes are shared with tvdemo (`../tui/tvthemes.cfg`).

