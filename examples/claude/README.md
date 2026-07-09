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

## graphic/ — animation (intrinsic `graphics.h` + `../../ws.h`)

| File | What it does |
|------|--------------|
| `lissajous.cpp` | Animated 3:2 Lissajous curve; runs until the window is closed |
| `raytrace.cpp` | Ray-traced metal still life: silver sphere, bronze cube (balanced on one vertex, space diagonal vertical) and gold torus on a checkerboard plane (reflectance 0.3), off-axis camera; resolution from the command line (`Scinc raytrace.cpp [w h]`, default 640×480); renders one frame with 2×2 AA, shows it, and saves `raytrace.png` |

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
