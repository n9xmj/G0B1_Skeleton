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

## 3. nvmparams → own subdir AND fully HW-independent

**Sequenced LAST, deliberately** (user, 2026-08-09): it needs the most planning
investment, and the design is still evolving. Do the low-hanging packaging jobs
(item 5) first.

**This one gets the full decision-log board** (`Docs/planning/nvmparams-plan.md`), and
the user expects to **interrupt frequently to revise and extend the wish list**. That is
the intended working mode here, not a sign of churn: park every open point as a row,
keep the board current as they revise, and do not try to close design questions on their
behalf. Contrast item 5, which is mechanical and wants no board at all.

Migrate to `App/nvmparams/`, then add a **pluggable storage-driver** layer so the pool has
no dependency on the NVM hardware. The caller attaches read/write "drivers" at pool init
via a standardized prototype, for any backend: STM internal flash, SPI/I2C flash,
filesystem (stdio) file, RTC backup memory, RAM-emulated, etc. `nvm_pool_init` also takes
more metadata: device/file label or identifier, sector-or-file offset address, block size
to reserve for the pool, and the like. The **core semi-linked-list pool management is
solid** and needs little change — the work is the device interface + the extra init
metadata.

Known gotcha to keep in mind: a NOLOAD NVM sector survives a reflash, so a foreign
project's pool can shadow your IDs (this is not an nvmparams bug).

### Test hardware and where the SPI flash driver lands

**A W25Q128 is now physically attached to the G0B1 bench platform** (user, 2026-08-09).
It is not needed for any application function — it is there specifically so the pluggable
storage-driver layer can be exercised against a *real* second backend rather than only
against internal flash. That matters: a driver interface validated against one backend
tends to encode that backend's assumptions.

Driving it needs the **core SPI flash driver** from `LED_Strip_Controller_G474`
(`App/spiflash/`) — the raw chip driver only, **not** the partition table or the VFS/
littlefs layers stacked on top of it there.

**Landing site: SwitchTester, not Skeleton** (user preference — keep the SPI flash API out
of the Skeleton baseline). That preference and the architecture agree, which is worth
stating explicitly because it is the first real test of the design:

- The **nvmparams core** is by definition hardware-independent, so it is portable and
  belongs in Skeleton as a vendored module (`App/nvmparams/`).
- A **storage driver** is the per-project port — the D5 pattern, where the module declares
  the interface and the application supplies an implementation. The W25Q128 driver and the
  nvmparams storage driver that wraps it are port code.

So SwitchTester carries `App/spiflash/` plus its storage-driver glue, Skeleton carries the
core and stays clean, and neither needs the other. If that split turns out to be awkward
in practice, the awkwardness is telling us something about the driver interface, which is
exactly what the bench part is for.

## 4. Wire i_getline to self-mute stdout

Mechanism exists (see the stdout-mute section of the strategy doc), not wired. Have
`i_getline` bracket itself: `v_stdout_mute(1)` on entry, `v_stdout_mute(0)` on exit, so
async job logs can't scramble a line being typed; the stderr echo (already done) keeps
keystrokes visible. **The single-char menu idle loop must stay UNMUTED** (logs visible
while waiting for a key) — only multi-char line entry mutes. Today the ONLY caller of
`v_stdout_mute()` is the automation-console SCRIPT session; `i_getline` just moved its
echo to stderr and asserts nothing yet.

## 5. menusystem → vendored module (packaging)

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
