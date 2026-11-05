# MetroCE tutorial

Two parts: **playing** (how the city works) and **the code** (how the program works).

---

## Part 1: Playing

### Your first city (5 minutes)

1. **Road.** Tool 1 is selected at start. Hold `2nd` and press two arrows together (for example Right+Down) to paint a straight road along the grid. Or press `Stat`, move away, press `2nd`: an L-shaped road appears. `Stat` again flips the L.
2. **Zones.** Press `2` (residential), then `Stat`, move to make a box next to the road, `2nd`. Repeat with `3` (commercial) and `4` (industrial). A zone only grows if it touches a road **side to side**. Corners do not count.
3. **Utilities.** Press `5`, `6`, `7` and place one power plant, one water tower, one sewage plant anywhere. They serve the whole city; there are no wires or pipes.
4. **Run.** The city starts running at speed 1. `Graph` is faster, `Trace` pauses.
5. **Read it.** Top bar: `P` power, `W` water, `S` sewage (percent covered), `H` happiness. `Enter` shows demand. Bottom bar shows the tile under the cursor; red **NO ROAD** means a zone cannot grow.
6. **Fix mistakes.** `Del` undoes your last stroke, `Del` again redoes it. `Alpha` bulldozes.

### The rules (taken from `city.c`)

Each simulation tick, every zone with a building adds to the city's load, then growth is decided.

| Thing | Rule |
|---|---|
| Home population | Level 1/2/3 holds 10 / 31 / 66 people |
| Jobs | Commercial 7 per level, industrial 10 per level. Half the population works |
| Utility capacity | Each plant adds 180, multiplied by service funding % |
| Power load | Home = level, commercial = level + 1, industrial = 2 × level |
| Water load | Level, for all three zone types |
| Sewage load | Home and commercial = level, industrial = 2 × level |
| New building | Needs a road next door, and power, water, sewage all at 60% or more. Chance per tick is demand ÷ 5 percent |
| Upgrade | Level below 3, older than 5 ticks, demand above 45. Chance is demand ÷ 18 percent (+2 if happiness is above 75) |
| Downgrade | Power or water below 35%, or no road: 18% chance per tick to lose a level |
| Residential demand | 35 + happiness ÷ 3 − res tax + 14 if there are spare jobs |
| Commercial demand | 25 + population ÷ 35 − com tax, +8 if power is 100%, otherwise −20 |
| Industrial demand | 48 + population ÷ 80 − ind tax, same power rule |
| Happiness (per home) | Base 70. Park +9, school +7, police +5, fire +4, pollution −16, no road −30. Minus 2 per tax point above 9, plus (funding − 100) ÷ 12, minus unemployment ÷ 5, minus shortage penalties. Averaged over homes, 0 to 100 |
| Service reach | Park 7, school 9, police 10, fire 10 tiles (grid distance, not straight line) |
| Pollution | Power plant radius 5, industrial radius 3 + level |
| Ranks | Village 100, town 500, city 2000, capital 6000, metropolis 15000 |

Things that do **not** do anything yet: road congestion is shown but affects nothing, and money is unlimited so taxes only move demand and happiness.

Practical consequences: one plant of each type carries a lot of city early on, so spend effort on **parks, schools and keeping industry away from homes** (pollution is −16 per home in range). Raise service funding for more capacity and happier homes.

---

## Part 2: The code

```
src/main.c      frame loop, menus, tool/area/undo commands, sim timing
src/input.c/.h  keyboard snapshot, edge detection, arrow repeat + acceleration
src/area.c/.h   cells of an L-shaped path or a filled box
src/city.c/.h   map, simulation, build/bulldoze, undo, save/load
src/render.c/.h drawing: tiles, overlays, HUD, menus, help pages
src/gfx/        generated sprites (tools/make_assets.py -> convimg)
tests/          host-side tests (tests/run.sh)
```

### One frame (`main.c`)

```
now = clock()
input_scan()                     snapshot keys, remember last frame's
tool keys (0-9 . + -)            change selected tool
2nd / Alpha pressed              city_undo_begin()   (lazy, see below)
Stat                             start area, or flip the L
tap actions                      build/bulldoze at the cursor
input_arrows() -> move_cursor()  move and paint
Del, Y=, Window, Zoom, ...       single-press commands (edge detected)
if sim timer due: city_simulate()
render_frame()                   draws and swaps buffers
```

There is no fixed `delay()` any more. `gfx_SwapDraw()` waits for the screen, so the frame rate is whatever the drawing costs, and **nothing depends on it**: key repeat and simulation use `clock()` (32768 ticks per second, `MS(n)` converts milliseconds).

### Isometric drawing (`render.c`)

A tile at map `(x, y)` goes to screen
`sx = cam_x + (x - y) * 8`, `sy = cam_y + (x + y) * 4`
(tiles are 16×8 pixels). The camera is set so the cursor is always at the screen centre, so the map scrolls instead of the cursor moving.

Tiles are drawn in order of `x + y` (far to near, the *painter's algorithm*), so a tall building is correctly covered by the tile in front of it. Anything off screen is skipped before drawing.

### Why the arrows work the way they do (`input.c`, `main.c`)

On an isometric map the grid axes run diagonally on screen. Arrows could mean either "screen direction" or "grid axis", and each is bad at something:

- Screen-aligned arrows feel right for looking around, but a screen-horizontal step crosses two grid cells diagonally, so painting with it would make roads that only touch at corners (not connected).
- Grid-axis arrows paint clean roads but feel rotated 45 degrees (the old behaviour: Up moved up-right).

MetroCE uses both: **single arrow = screen direction, two arrows = grid axis** (the arrow pad is a disc, so pressing two neighbours with one thumb is natural). `map_dx / map_dy` in `main.c` is the lookup table. While painting, a screen-direction step is split into its two grid steps, so every cell painted is edge-connected to the last (`move_cursor`).

Timing, in `input_arrows()`:

1. **First press waits one scan.** Otherwise pressing Right+Down slightly apart reads as Right, then Right+Down, and you get a stray step. A tap released before the scan still counts as a tap.
2. **Hold:** after `ARROW_REPEAT_DELAY` it repeats every `ARROW_REPEAT_RATE`.
3. **Acceleration:** after 1 s it takes 2 tiles per repeat, after 2 s it takes 3. Not while painting, so strokes stay precise.
4. **Rolling to another direction** steps immediately and uses a shorter delay.

`input_pressed()` is edge detection: a bit is "pressed" if it is set now and was clear at the previous scan. That is what makes `Zoom` save once instead of once per frame.

### Undo (`city.c`)

`undo_tiles` is a copy of the map, `undo_mask` is one bit per tile. `city_undo_begin()` only sets a flag. The copy is made **just before the first real change** of the stroke, and `build`/`bulldoze` mark every tile they change. `city_undo_toggle()` swaps back only the marked tiles, so:

- a click that changes nothing does not destroy your previous undo,
- growth that happened elsewhere meanwhile is not rolled back,
- undo and redo are the same swap.

It is one level deep. The buffers live outside the `City` struct, so old saves still load.

### Tuning

| What | Where |
|---|---|
| Key repeat delay, rate, acceleration | top of `input.h` |
| Simulation speed (ms per tick at speeds 1, 2, 3) | `sim_ms` in `main.c` |
| Message and exit-confirm durations | `MESSAGE_MS`, `EXIT_CONFIRM_MS` in `main.c` |
| Balance (growth, demand, happiness) | `city_simulate()` in `city.c` |
| Which key does what | the `input_pressed(...)` calls in `main()` |

### Design notes: what was researched, and what was not

I looked for existing TI-84 Plus CE games to copy controls from. Honest result:

- **No open-source isometric city builder for the CE turned up.** The only CE city sim I found was a TI-BASIC game (Cemetech forum, 2016) that draws coloured letters; its controls are not described. Other "Sim City" results were for the older monochrome calculators. So the area tool, L-roads and undo are **my design**, not copied from a CE game.
- **Key handling advice that did turn up, and was used:**
  - The OS key routine (`os_GetCSC`) has its own delayed repeat, which is why BASIC-style movement feels stiff. The toolchain docs describe the `keypadc` library as direct input and the OS one as suited to non-game programs. MetroCE already used `keypadc`; it just never added any timing of its own.
  - A timer-based debounce example (TIny-Hacker's gist): act on the first press, wait about a quarter of a second, then repeat quickly. That is the same shape as `ARROW_REPEAT_DELAY` / `ARROW_REPEAT_RATE`.
  - A CE solitaire game (`fortune`) detects new presses by comparing `kb_Data` with the previous scan. That is `input_pressed()`.
  - A CE Sudoku thread notes `kb_Scan()` must be called again before re-reading `kb_Data`. That is why the old "wait until released" loops were needed, and why they blocked everything else.

### Known limits

- **Not run on a calculator or emulator.** It compiles with no warnings (CEdev v15.0), and the logic is unit-tested on a PC, but I had no ROM to run it. The repeat timings, simulation speeds and exact text positions are untested guesses. Layout text uses measured string widths where it matters, but check every screen once.
- Pressing Up/Down/Left/Right together with `2nd` and `Alpha` relies on the calculator keypad not "ghosting" key combinations. By the key matrix layout it should be fine, but that is also untested.
- Undo is one level. Congestion does nothing. There is no money system.
