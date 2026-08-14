# Improvements backlog

> Promoted from session notes on 2026-08-08. Captured-but-unbuilt ideas for the portable
> APIs. Items 1 and 2 are **done** (2026-08-09) — see
> [`logging-api-plan.md`](logging-api-plan.md) for the decision log and verification.
> Companion: [`portable-apis-strategy.md`](portable-apis-strategy.md).

## 1. Logging API → own source subdir — ✅ DONE 2026-08-09

Already API-ized; just relocate to `App/logging/` (or similar), like uart_stream /
automation_console. Mechanical migration.

**As built:** it was not purely mechanical. The macro layer was extracted out of the
app-owned config header into a vendored `log_helpers.h`, `ANSI.h` moved to `App/common/`,
and the HAL dependency was replaced by an application-defined `u32_log_timestamp_ms()`.
`debug_config.h` was renamed `logging_config.h` and its non-logging content moved to
`device_config.h`.

## 2. Logging macros: binary switches → compile-time LOG LEVELS — ✅ DONE 2026-08-09

Today the message classes in `debug_config.h` are on/off booleans. Change to a level
scheme: a global constant desired level — none (0), error, warning, info, debug (4) —
compared in the `LOGxx()` macros against each message's `LOG_xxxx` constant. Keep
EVERYTHING `const`/`#define` so the `if()` in the macro folds at compile time and the
guarded code is dead-code-eliminated when the test is false (holds at any `-O` above
`-O0`).

**As built — the ladder runs the other way.** The paragraph above describes an ascending
*severity* scale, which was tried first and abandoned: with `>=` comparison, a global of
`0` turned out to be the most permissive setting the scale could express, the exact
opposite of what "none" promises. The constants are **verbosities**, so the shipped ladder
ascends terse → chatty and the predicate is `<=`:

```c
#define LOG_LEVEL_QUIET     0   // never emitted — as a class value AND as the global
#define LOG_LEVEL_ALWAYS    1
#define LOG_LEVEL_ERROR     2
#define LOG_LEVEL_WARNING   3
#define LOG_LEVEL_INFO      4
#define LOG_LEVEL_DEBUG     5

#define LOG_EMIT(tag)   ( (tag) != LOG_LEVEL_QUIET && (tag) <= (LOG_LEVEL) )
```

That direction is what makes `0` mean quiet at *both* ends of the comparison, which in
turn preserves the legacy 0/1 tags for free: a project that still writes
`#define LOG_FOO 1` migrates by setting `LOG_LEVEL` to `LOG_LEVEL_DEBUG` and changing
nothing else. The compile-time fold was measured, not assumed — a `QUIET` build drops the
guarded code *and* its format-string literals from `.rodata`. See D1/S3 in
[`logging-api-plan.md`](logging-api-plan.md).

## 3. nvmparams — vendored module + pluggable storage drivers

**NEXT BIG THING, and it gets its OWN SESSION** (user, 2026-08-12). The user is doing
high-level design thinking first and will relay decisions at the start of that session.
**Do not pre-empt those decisions.** This section is the cold-start brief: what is known,
what is measured, and what is explicitly still open.

**This one gets the full decision-log board** (`Docs/planning/nvmparams-plan.md`, not yet
created — building it is the first act of the new session, once the user relays their
high-level shape). The user expects to **interrupt frequently to revise and extend the
wish list**. That is the intended working mode, not churn: park every open point as a row,
keep the board current, and do not close design questions on their behalf.

---

### What the user has already decided (stated 2026-08-12)

1. **Make it a vendored module**, same pattern as `logging` / `uart_stream` /
   `automation_console`. So: `App/nvmparams/`, a `nvmparams_config_template.h`, and a
   README per T4.
2. **Adopter-supplied "device drivers" for any non-volatile backend** — STM internal
   flash, SPI or I2C external flash/EEPROM, a file via C stdio / a user-provided
   filesystem, RTC backup RAM, and **NVM-emulated-in-RAM**. That last one is deliberate:
   it doubles as the **degenerate how-to example** for writing a driver.
3. **`x_nvm_init` takes a struct**, not a parameter list — pool settings *including the
   driver function pointers* arrive in one struct. **TBD:** whether that struct IS the
   pool handle, or a separate public-facing settings struct the pool copies from.
4. **Stretch goal: very basic wear levelling.**
5. **Review the present implementation for correctness and robustness** as part of the
   work — this is not purely a repackaging job.

### The two main decision points, in the user's words

- **How to structure the device drivers.**
- **What parameters the pool init function must be given.**

Everything else is downstream of those two. Two further points, added the same day, are
settled in shape but not in detail and are written up below: **where the enums split**
(and how the reserved ID space is partitioned), and **backward compatibility with
existing projects**, which has a named check target.

---

### Where the enums split, and the reserved-ID question (user, 2026-08-12)

The user is **already clear on what gets split where**. It remains a formal decision point
to be confirmed when the board is built, but the shape is not in doubt:

| Enum | Home | Why |
|---|---|---|
| **Parameter IDs** (`NVM_PARAM_*`) | **adopter-modified header** | Application data. Mostly owned by the application. |
| **Error codes** (`NVM_ERROR_*`) | **module header** | Wholly owned by the nvmparams core. |

**But the parameter-ID space is not purely the application's.** The core will reserve some
IDs for itself:

- an **end-of-list sentinel**;
- probably **write-count IDs**, to hold the per-region counters that wear levelling needs;
- possibly others.

So the real question is not "app or module" but **how the ID space is partitioned** — a
reserved range the core owns, an application range, and whoever writes an adopter header
needing to know where the boundary is and that it may move. Worth deciding *with* the
init-parameter question (main decision point 2), because if IDs are adopter-owned the pool
may need to be TOLD its ID space rather than assume it.

Note the existing contiguity contract this has to survive: SwitchTester computes IDs
arithmetically (`NVM_PARAM_CYCLE_A_REPEAT + (channel * COUNT) + parameter`) and guards the
assumption with `_Static_assert` in `switch_out.c`. Whatever the split, arithmetic ID
computation over an application-defined block must keep working.

### Backward compatibility is a GOAL, and there is a named compatibility check

**The user wants nvmparams droppable into other existing projects**, not only the three in
this effort. Some have not been discussed in these sessions at all.

**The named compatibility target is `ee_fw-ST3074-8-inch-Round-mirror-wifi-bt`** — a
**work project**, and the STM side of the two-MCU smart-mirror product. Relevant facts
already on hand, none of which required opening that repo:

- It is an **STM32G0B0** — same family as this bench's G0B1, so the internal-flash driver
  should transfer with little more than page-geometry differences.
- **NVM is one of its existing concerns**, alongside LEDs, sensors, charger, state machines
  and shadow apply/publish. So it has a working parameter store today whose usage patterns
  are a real test of the new API.
- Being a work project, it carries its own review and release constraints. A refactor that
  forces sweeping call-site churn there is a much harder sell than one that does not.

**Do not dig into that repo during nvmparams planning** (user, explicit). It is named here
so that the design keeps it in view — specifically, so "how much does adopting this break
at the call sites" stays a live question rather than being discovered late.

Practical reading: the **public call API** (`x_nvm_get`, `_set`, `_create`, `_commit`, …)
should stay recognisable even as `x_nvm_pool_init` changes shape. Init is called once per
pool; the accessors are called everywhere. Breaking the former is cheap, the latter is not.

### Current state — measured 2026-08-12, so the new session need not re-derive it

| Fact | Value |
|---|---|
| Location | `App/Src/nvmparams.c` + `App/Inc/nvmparams.h` — **not yet a module directory** |
| Size | 912 lines `.c`, 299 lines `.h` |
| Present in | G0B1_Skeleton, SwitchTester. **Absent from LED_Strip.** |
| Call sites | 9 in Skeleton, 42 in SwitchTester |
| Public API | `x_nvm_pool_init`, `_pool_release`, `_create`, `_delete`, `_get`, `_get_size`, `_set`, `_commit`, `_list`, `_search`, `_read`, `_write` |
| Current init | `x_nvm_pool_init(&pool, NVM_DEVICE_MCU_FLASH, NULL, NVM_DATA_POOL_SIZE)` — 4 positional args, the thing decision 3 replaces |
| HW coupling | Concentrated and small: `HAL_FLASH_Unlock/Lock`, `HAL_FLASHEx_Erase`, `HAL_FLASH_Program` (`FLASH_TYPEPROGRAM_DOUBLEWORD`), a `FLASH_PAGE()` macro over `FLASH_BASE` / `FLASH_PAGE_SIZE` |

**The two copies are NOT meaningfully diverged.** A raw diff reads 1829 lines in the `.c`
and 621 in the `.h`, but that is line-ending noise — **whitespace-insensitive it is 7 and
23**. Do not be alarmed by the raw number; it vanishes once the working trees are
converted to LF (see `.gitattributes`, added 2026-08-12).

The genuine differences are exactly two, and **both are informative**:

- **`NVM_ERROR_NO_CHANGE` (-8)**, SwitchTester only. Lets a caller distinguish "committed"
  from "nothing to commit". The pool's whole purpose is minimising erase/write cycles, and
  that is only visible if the two outcomes are distinguishable. **This belongs in the
  core** — take SwitchTester's version as the baseline for it.
- **SwitchTester's application parameter IDs live in the module header's enum**
  (`NVM_PARAM_SWITCH_PULSE_MS`, the twelve `NVM_PARAM_CYCLE_*`). Which is the finding:

> **The parameter-ID enum is application data sitting inside what will be a vendored
> header.** It has to move to the adopter's side — same class of problem as
> `ACON_MAX_ARGS` living in `.cproject` and `ACON_LINE_MAX` in `device_config.h`, both
> fixed the same day. Note SwitchTester's IDs carry a documented contiguity contract
> guarded by `_Static_assert` in `switch_out.c`, so wherever they land must preserve
> arithmetic ID computation.

### A driver interface already exists in embryo

Worth reading before designing the replacement, because the refactor **formalises
something already there** rather than inventing it:

- `x_nvm_read()` / `x_nvm_write()` are already the device hooks, and the header already
  names `x_mcuflash_read()` / `x_mcuflash_write()` as "device drivers".
- `NVM_DEVICE_MCU_FLASH`, `NVM_DEVICE_FILE`, `NVM_DEVICE_NONE`, `NVM_DEVICE_MAXVAL`
  already exist as an enum — an early sketch of the same idea.
- But today the header instructs: *"The API user is responsible for modifying this routine
  to support ... the storage device(s) that will be used."* **Editing the module to add a
  backend is precisely what the pluggable layer removes.** There is also a stray
  `#define SPIFLASH_NVM_DATA_ADDRESS 0x0400` in the `.c` — evidence of a second backend
  half-started in place.

**The core semi-linked-list pool management is solid** and needs little change. The work is
the device interface, the init metadata, and decision 5's correctness review.

### Test hardware and where the SPI flash driver lands

**A W25Q128 is physically attached to the G0B1 bench platform.** It serves no application
function — it is there so the pluggable driver layer can be exercised against a *real*
second backend rather than only internal flash. That matters: a driver interface validated
against one backend tends to encode that backend's assumptions.

Driving it needs the **core SPI flash driver** from `LED_Strip_Controller_G474`
(`App/spiflash/`) — the raw chip driver only, **not** the partition table or the
VFS/littlefs layers stacked on it there.

**Landing site: SwitchTester, not Skeleton** (user preference — keep the SPI flash API out
of the Skeleton baseline). Preference and architecture agree, which is worth stating
because it is the first real test of the design:

- The **nvmparams core** is hardware-independent by definition, so it is portable and
  belongs in Skeleton as `App/nvmparams/`.
- A **storage driver** is the per-project port — module declares the interface,
  application supplies the implementation. The W25Q128 driver and its nvmparams glue are
  port code.

So SwitchTester carries `App/spiflash/` plus glue, Skeleton carries the core and stays
clean, neither needs the other. **If that split turns out awkward in practice, the
awkwardness is telling us something about the driver interface** — which is exactly what
the bench part is for.

### Carried-over gotchas

- **A NOLOAD NVM sector survives a reflash**, so a foreign project's pool can shadow your
  IDs. Not an nvmparams bug, but it will waste an afternoon if met cold.
- **I8 (parked from the logging plan):** the NVM auto-commit delay is defined in two
  places — `DEV_CONFIG_NVM_COMMIT_DELAY_MS` in `device_config.h` and
  `NVM_AUTO_COMMIT_DELAY` in `platform.h`. Resolve as part of this work; the user chose to
  keep the `device_config.h` one.

## 4. Wire i_getline to self-mute stdout

Mechanism exists (see the stdout-mute section of the strategy doc), not wired. Have
`i_getline` bracket itself: `v_stdout_mute(1)` on entry, `v_stdout_mute(0)` on exit, so
async job logs can't scramble a line being typed; the stderr echo (already done) keeps
keystrokes visible. **The single-char menu idle loop must stay UNMUTED** (logs visible
while waiting for a key) — only multi-char line entry mutes. Today the ONLY caller of
`v_stdout_mute()` is the automation-console SCRIPT session; `i_getline` just moved its
echo to stderr and asserts nothing yet.

## 5. menusystem → vendored module (packaging)

**STATUS 2026-08-14 — ✅ FULLY RECONCILED.** LED_Strip's `x_type`/`c_key`/`pfn_`/`p_x_`
naming is now THE canonical across all three projects. The canonical was built in LED_Strip
(the four SwitchTester-side behaviours merged in — hide-on-NULL return, `[At top-level menu]`
empty-stack message, the adoption README, and the Phase-2 option-flag bitfield union — plus
`uart_stream`-style Doxygen and the `MENU_API_PROMPT`→`MENUSYSTEM_PROMPT` macro rename), then
copied byte-identical to SwitchTester + Skeleton, and each project's menu **definitions**
renamed to the canonical members. The `#ifndef DEBUG_H` guard wart is retired (`#pragma once`
now). All three build 0/0; SwitchTester was bench-verified on hardware. History below.

**STATUS 2026-08-13 — packaging DONE (organization); the code baseline went the OTHER way
from the plan below; full reconciliation DEFERRED.** All three projects now carry the
module at `App/menusystem/menusystem.{c,h}` (directories unified, `.cproject` include paths
added, all three build 0/0). But this plan called for **LED_Strip's** copy to be the code
baseline, and it was not:
- **SwitchTester + Skeleton** carry a canonical built from *SwitchTester's* older
  `item_type`/`key` naming, with Mirror's `no_newline` + `const` merged, `ANSI.h` dropped,
  and hide-on-NULL returns added. Byte-identical to each other. (Still uses the wrong
  `#ifndef DEBUG_H` include guard — carried forward, not fixed.)
- **LED_Strip** was only *relocated* (`menu-api/`→`menusystem/`, files renamed, includes +
  `.cproject` fixed); its code is untouched — still the fuller `x_type`/`c_key`/`pfn_`/
  `p_x_` fork this plan intended as the baseline.

"Consistency in organization, divergence in code" (user, 2026-08-13). **Open decision:**
which member-naming is THE canonical. Adopting LED_Strip's fuller Hungarian (as this plan
intended) means renaming SwitchTester+Skeleton (~130 refs) and redoing this session's
committed work; keeping `item_type`/`key` means renaming LED_Strip's **263** refs. Its own
focused pass — do not blind-sweep it.

**Reconciliation plan (✅ COMPLETED 2026-08-14; direction set by user 2026-08-13).** Done the
CHEAP direction: **LED_Strip's menusystem was the baseline** — its `x_type`/`c_key`/`pfn_`/
`p_x_` naming became THE canonical. Executed exactly as below (menu-def rename touched
SwitchTester's `debug_menu.c` ~113 initialiser tokens and Skeleton's ~30, all via a
word-boundary regex sweep so `.key`/`.menu` could not clobber `.key_list`/`.menu_stack`):
1. Migrate into LED_Strip's copy the behaviour changes made to SwitchTester's this session:
   the hide-on-NULL-text return, the `[At top-level menu]` empty-stack message, and the
   adoption README (with its edits). Fold in Phase 2 here too — collapse the option-flag
   bytes (`b_not_implemented`, `b_no_newline`) into one bitfielded union byte ("bitmap fields").
2. Copy that reconciled module back to SwitchTester and Skeleton (byte-identical).
3. Edit SwitchTester's + Skeleton's menu DEFINITIONS to the canonical member names
   (`item_type`→`x_type`, `key`→`c_key`, `text`→`p_c_text`, `function`→`pfn_function`,
   `menu`→`p_x_menu`, …) — ~130 designated-initialiser refs (SwitchTester 101, Skeleton 29).
   Every miss is a hard compile error, so it is mechanical.

Rationale (user): far fewer LOC than the other direction (renaming LED_Strip's **263** refs
across its 2283-line `debug_menu.c`), since SwitchTester + Skeleton have only a handful of
menu defs. Also retires the `#ifndef DEBUG_H` guard wart (LED_Strip uses `#pragma once`).

**Phase 1 — packaging only, no behaviour change.** The code is already portable: zero
application dependencies in either copy (C library plus its own header; Skeleton's also
pulls `ANSI.h`). What is missing is packaging, following the logging model.

**Baseline: LED_Strip's `App/menu-api/menu-api.{c,h}`** (user decision 2026-08-09). It is
the more evolved copy, not merely the one without the `ANSI.h` dependency:

| | LED_Strip (baseline) | Skeleton / SwitchTester |
|---|---|---|
| Include guard | `#pragma once` | **`#ifndef DEBUG_H`** — wrong name, latent collision |
| Prompt macro | `MENU_API_PROMPT`, `#ifndef`-guarded | `MENUSYSTEM_PROMPT`, unguarded |
| Field naming | `x_type`, `c_key`, `p_c_text`, `pfn_function` | `item_type`, `key`, `text`, `function` |
| `const` correctness | `const char *p_c_text` | `char *text` |
| The spare byte | **`b_no_newline`**, a real flag | `_reserved` padding |

Skeleton and SwitchTester are **byte-identical to each other**, so this is a two-way
reconciliation. The 192 differing lines between the `.c` files are almost entirely the
naming convention, not logic.

**Name it `menusystem`,** so the directory is `App/menusystem/` and the file
`menusystem.c`. Not `console_menusystem` — length without disambiguation, and
`automation_console` already owns "console" here. Note the content comes from LED_Strip
while the *filename* does not: this is also what retires the `menu-api` naming question.

**Measured cost** — all mechanical, and every miss is a hard compile error because these
are designated initialisers:

| Project | Touch points | Where |
|---|---|---|
| Skeleton | **69** | `debug_menu.c` 35, `menusystem.c` 26, header 8 |
| SwitchTester | **154** | `debug_menu.c` **120**, `menusystem.c` 26, header 8 |

Only the three `.cproject` files besides. **No port layer is needed** -- menusystem takes
nothing from the application, so vendoring it is just *copy the directory and call its
public functions in the right places*. A `menusystem_config.h` is **optional**: the prompt
macro is already `#ifndef`-guarded, so a project wanting a different prompt can `-D` it or
define it ahead of the include. Do not create a config header just to have one.

**Accepted trade:** both copies have the key-conflict check, but Skeleton's colours the
warning via `ANSI_FG_YELLOW` and LED_Strip's does not. The warning goes monochrome — which
is exactly what drops the `ANSI.h` dependency, so take it rather than reintroducing ANSI.

**Phase 2 — wish list, not scoped.** Collapse the per-line option flags
(`b_not_implemented`, `b_no_newline` — currently a whole `uint8_t` each) into a single
bitfielded struct/union byte, for size, ease of use and semantic consistency.

## 6. Per-module README → adoption instructions for each vendored module

Requested 2026-08-12. Each vendored module gets a `README.md` in its own directory:
`App/logging/`, `App/uart_stream/`, `App/automation_console/`, and `App/menusystem/` when
it exists.

**Scope is adoption, not conventions.** The README answers "I have a new project, how do I
drop this in": which files to copy, which template to rename and edit, which hooks to wire,
a minimal call sequence, and the gotchas that cost time. It **links** to
`portable-apis-strategy.md` for the cross-cutting model (three tiers, the dependency rule,
the optional port) rather than restating it — that doc stays the single source of truth
for conventions, per the standing rule that they are never duplicated.

Sequencing note: written AFTER the modules settle, not before. `uart_stream`'s README would
have needed rewriting twice had it been started before the config header landed.

Candidate contents, per module: file manifest with which are vendored vs adopter-owned; the
config-header knobs table; the port/hook list (or an explicit "none needed"); minimal
integration snippet; and a "measured gotchas" section — e.g. uart_stream's CubeIDE indexer
trap and its two family boundaries, automation_console's RX-ring-vs-`ACON_LINE_MAX`
constraint.

## 7. LED_Strip's uart_stream — re-vendored 2026-08-12, LIVE TEST STILL OWED

Found 2026-08-12 while closing I11; **resized the same day after actually counting.** An
earlier draft of this item claimed "~1,600 of 1,610 lines differ, a migration with
call-site changes throughout." The line count was real but **textual** — doc-comment style,
banners, formatting — and it badly overstated the job. Corrected below.

**Actual call sites outside `App/uart_stream/`: five.**

```
v_uart_stream_isr_for              1
v_uart_stream_tx_byte_blocking     3
v_uart_stream_tx_multi_blocking    1
```

**What genuinely differs:**

| | LED_Strip | Skeleton / SwitchTester |
|---|---|---|
| Flush | unbounded spin ×2, no timeout | caller drain bound + TC bound derived from live baud |
| Vector mapping | `static e_uart_stream_get_irqn()` INSIDE the module, hardcoded, `HardFault_IRQn` on no-match | app-owned `uart_stream_target_<part>.c`, weak handles |
| ISR entry | `v_uart_stream_isr()` services all; `v_uart_stream_isr_for()` linear-searches | `b_uart_stream_service_uart(huart)` returns bool so shared vectors chain |
| Blocking calls | `void` | return bool/count — a timeout is detectable |
| Queue | heap `p_x_queue_create`/`v_queue_destroy`, `*_blocking` variants | `*_isr` variants skipping PRIMASK where an ISR cannot be preempted |
| Baud get/set, ISR service counters | absent | present |

**The vector map is the real work, and the reason the split happened.** LED_Strip's ladder
is G4-shaped — `USART3_IRQn`, `UART4_IRQn` as distinct vectors — whereas the G0B1 shares
`USART3_4_5_6_LPUART1_IRQn` across five UARTs. Writing `uart_stream_target_g474.c` is the
bulk of the job.

**Safety note worth acting on regardless:** LED_Strip's `v_uart_stream_tx_flush_blocking()`
spins on queue-empty and then on TC with **no bound on either**, so a wedged peripheral
hangs the main loop permanently. That alone may justify the swap.

**DONE 2026-08-12 — EXCEPT the live test.** Migrated: the vendored files are
byte-identical with Skeleton's; `App/Inc/uart_stream_config.h` and
`App/Src/uart_stream_target_g474.c` written; `USART2_IRQHandler` rewired from
`v_uart_stream_isr_for(USART2)` to `b_uart_stream_service_uart(&huart2)`; five call sites
updated for the bounded blocking calls. Build 0 errors / 0 warnings, text 294196, indexer
0.13%. Map confirms the new symbols linked and the old ones gone.

### 7a. LIVE TEST ON G474 HARDWARE — OPEN, BLOCKING CONFIDENCE

**Nothing here has run on a board.** The G474 was unavailable on 2026-08-12. Until it runs,
treat this migration as unverified regardless of how clean the build is.

What to exercise, in order:

1. **Console works at all** — boot banner over COM5 at 921600. That alone proves
   `b_uart_stream_service_uart(&huart2)` is on the right vector and the ring is serviced.
2. **The DMA strips still drive.** This is THE regression risk. `uart_stream_target_g474.c`
   lists all six UARTs, and although the table is read only by the IRQn lookup for the one
   handle being bound, that is static reasoning, not evidence. Run a strip.
3. **`printf` under load** — the blocking TX calls are now BOUNDED where they used to spin
   forever, so a full ring drops a byte after `UART_STREAM_TX_BLOCK_TIMEOUT_MS` (100 ms)
   rather than hanging. Confirm no visible truncation in normal console traffic.
4. **fs_shell binary transfer** — `v_write_byte`/`v_write_bytes` are bounded at 1000 ms and
   DISCARD the result. A short write is newly detectable but not plumbed through, because
   both wrappers are void and their callers have no error path. If transfers prove lossy
   under load, plumbing that return value is the fix.
5. **Flush timeouts** — the replaced module spun unbounded on queue-empty and TC; both are
   now bounded and the TC wait is derived from the live baud. Nothing should visibly
   change, which is the point.

Bench: ST-Link SN `0020002E3137510939383538`, COM5 @ 921600 (`scripts/bench.defaults.json`).
The `/roundtrip`, `/flash` and `/smoke` skills in that repo drive it.
