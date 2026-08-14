# Portable APIs — strategy & conventions

> Promoted from session notes on 2026-08-08 so it travels with the code.
> `G0B1_Skeleton` is the canonical home for this work; keep this doc as the source of
> truth here. Companion: [`improvements-backlog.md`](improvements-backlog.md).

## The portable-API set

These subsystems are being (re)designed as **portable, drop-in APIs**, each in its own
subdirectory, to be reused across STM32 projects:

- **menusystem** — **packaged 2026-08-13 (organization); code reconciliation pending.** Now
  a vendored `App/menusystem/` directory in all three projects (all build 0/0). NOT yet
  byte-identical: SwitchTester + Skeleton carry the canonical (`item_type`/`key` naming,
  Mirror's `no_newline` + const, ANSI dropped, hide-on-NULL returns); LED_Strip was
  relocated but keeps its fuller `x_type`/`c_key` fork. Which member-naming is canonical is
  an open decision — see backlog item 5.
- **uart_stream** — DONE 2026-08-08. Migrated into `G0B1_Skeleton`, bench-verified.
- **automation_console** — DONE 2026-08-08 (see below).
- **logging** — DONE 2026-08-09. Vendored as `App/logging/` with compile-time verbosity
  levels; bench-verified in Skeleton and SwitchTester. **It is the reference
  implementation of the conventions below** — the first module built to the finished
  port model. Full decision log:
  [`logging-api-plan.md`](logging-api-plan.md).
- **nvmparams** — LATER; wants a partial refactor first (see the backlog doc).
- **stdio retarget** — CANDIDATE, not yet a module. The three projects have diverged
  (see S4 in the logging plan); most relevant is that `_read` returns `-1` on an empty
  ring in Skeleton/SwitchTester but `0` bytes in LED_Strip, which code ported between
  them would hit silently.

## Vocabulary

**The per-project half of a module's contract is a PORT, not a "seam".** Use *port*,
*port file*, *port source*, *config header*, *port boundary*, *port point*.

"Seam" is a real term, but a narrower one: it comes from Michael Feathers' *Working
Effectively with Legacy Code* (2004), where it means a place you can alter behaviour
without editing at that place -- a **testability** concept about substituting behaviour in
code not designed for it, taxonomised as preprocessor, link and object seams. Nothing in
that definition is about hardware portability.

The libraries this convention is modelled on all say *port*: FreeRTOS (`portable/`,
`portmacro.h`, `port.c`), lwIP (porting layer, `sys_arch.c`), TinyUSB (`portable/`), FatFs
(low-level disk I/O layer, `diskio.c`); ST and Zephyr add *BSP* and *arch/porting layer*.
Citing those libraries as precedent while calling their pattern something they do not call
it just makes the docs harder to line up with the sources they came from.

Narrow exception, if it is ever wanted: the weak-default-overridden-by-strong-definition
trick (`u32_log_timestamp_ms()`) genuinely *is* a link seam in Feathers' sense, since
behaviour changes at link time with no edit at the call site. Even there, "weak override"
is plainer.

## The three tiers

Every file under `App/` answers one question — *"do I edit this when I clone Skeleton?"* —
and the answer determines where it lives.

| Tier | What it is | Where | Edit on clone? |
|---|---|---|---|
| **Vendored module** | portable API | `App/<module>/` | **never** — copy up/down wholesale |
| **Vendored leaf** | shared dependency with no module of its own | `App/common/` | **never** |
| **Port** | the per-project half of a module's contract | `App/Inc/`, `App/Src/` | **always** |
| **App** | the application itself | `App/Inc/`, `App/Src/` | it's yours |

Port files deliberately sit *with* the application rather than in a directory of their own —
the FreeRTOS `FreeRTOSConfig.h` / lwIP `lwipopts.h` / FatFs `ffconf.h` arrangement. The
module ships a documented template; you copy it out and edit it in place. What a
dedicated directory would have made obvious at a glance is carried instead by the port
inventory below and by an origin comment at the top of each copied template.

## The dependency rule

> A vendored module may `#include` the C library, other vendored modules, and its own
> `<module>_config.h`. **Everything else it needs from the application it declares as an
> `extern` prototype, and the application defines.**

That is the FatFs `diskio.c` / lwIP `sys_arch.c` arrangement: the library publishes
prototypes, the application supplies bodies — typically by copying the module's port
template. A module never includes an app-authored header of macros, and avoids the HAL
entirely where it can. Where a sensible default exists, give the declaration a **weak**
definition in the module so a freshly-vendored copy links and runs before any port source
has been written (`u32_log_timestamp_ms()` returns 0); where none does, use a weak
declaration plus a null guard at the call site (`PUMP_POLLING_TASK()`).

**The port layer is OPTIONAL.** A module that needs nothing from the application has no
port half at all, and vendoring it is simply *copy the directory and call its public
functions from the right places*. `menusystem` is the example: C library only, no config
required, no externs to satisfy. Do not invent a config header or a port source for a
module that does not need one -- an empty port is a property of a well-scoped module, not
an omission to be filled in.

**Status:** all three vendored modules — `logging`, `automation_console` and
`uart_stream` — satisfy this rule as of 2026-08-12. Each names exactly one
application file, its own `<module>_config.h`, and nothing else. I11 in the logging plan
is closed.

## Directory naming

A vendored module's directory is named for its **main source file, verbatim** —
underscores included, no `-api` suffix:

| Main source | Directory |
|---|---|
| `logging.c` | `App/logging/` |
| `uart_stream.c` | `App/uart_stream/` |
| `automation_console.c` | `App/automation_console/` |
| `menusystem.c` | `App/menusystem/` |

The same names are used in **all three projects**. Externally sourced libraries are the
exception: `littlefs`, `tlsf`, `berry-lang` and the like keep their upstream identity,
because that name *is* their identity.

## Port inventory

What to edit after cloning Skeleton, and where each file came from. **"none needed"** and
**"none yet"** differ: the first means the module requires nothing from the application,
the second is a conformance gap (I11).

| Module | Config header | Port source | Templates in |
|---|---|---|---|
| `logging` | `App/Inc/logging_config.h` | `App/Src/logging_port.c` | `App/logging/*_template.*` |
| `uart_stream` | `App/Inc/uart_stream_config.h` | `App/Src/uart_stream_target_g0b1.c` | `App/uart_stream/*_template.h` |
| `automation_console` | `App/Inc/automation_console_config.h` | `App/automation_console/automation_commands.c` | `App/automation_console/*_template.h` |
| `menusystem` | *(none needed)* | *(none needed)* | — |

No "none yet" cells remain. `menusystem`'s "none needed" is the optional-port rule doing
its job, not a gap.

`automation_console` has **no port source**. Everything it needs from the application is
either a macro in its config header (`ACON_TICK_MS()`, `ACON_PUMP()`, the `ACON_ID_*`
strings) or the `g_x_acon_command[]` table, which is the app's own command module rather
than a copied port template. That is the optional-port rule in practice, not a gap.

## Per-project adoption status — what each tree actually carries

Snapshot 2026-08-12. "Adopted" means the vendored files are present and byte-identical
with Skeleton's.

| Module | Skeleton | SwitchTester | LED_Strip | Note |
|---|---|---|---|---|
| `logging` | yes | yes | yes | byte-identical across all three |
| `uart_stream` | yes | yes | yes* | *LED_Strip re-vendored 2026-08-12, **live test owed** (backlog 7a) |
| `automation_console` | yes | yes | **not yet, deferred** | long-term intent to migrate; blocked on the host-script cost, see below |
| `menusystem` | yes | yes | organized* | `App/menusystem/` in all three (2026-08-13); SwitchTester+Skeleton byte-identical canonical (`item_type`/`key`). *LED_Strip relocated but code is its diverged `x_type`/`c_key` fork — naming reconciliation pending (backlog 5) |
| `stdio_retarget` | yes | yes | **not yet, deferred** | not a vendored module anywhere yet; LED_Strip uses `syscalls_vfs.c` + `__io_*`. Assessed in S4 (`logging-api-plan.md`); LED_Strip-side plan and consumer audit in that project's `Docs/planning/stdio-retarget-migration-plan.md`. Behind 7a |
| `nvmparams` | — | — | — | not built anywhere yet (backlog 3) |

**`automation_console` in LED_Strip is DEFERRED, with long-term intent to migrate**
(user, 2026-08-12). It is not a permanent exception, and it is not an oversight either.

The firmware port is ordinary work. The blocker is the host side: roughly 5,100 lines of
Python across 17 scripts are written against the current protocol, with **no regression
suite**, so changing the wire format invalidates all of it at once with nothing to say what
broke. Until a regression net exists, starting the firmware would move the risk rather than
reduce it.

Plan, cost breakdown and the two candidate transition strategies live in
`LED_Strip_Controller_G474/Docs/planning/automation-console-migration-plan.md`, and it is
on that project's TODO checklist. Prerequisite: backlog item 7a, since
`automation_console` talks to `uart_stream` directly.

Externally sourced libraries in LED_Strip — `berry-lang`, `littlefs`, `spiflash`, `tlsf` —
keep their upstream identity and are outside this effort's scope, per the directory-naming
rule above.

**So the genuine holdouts for LED_Strip are exactly two: `stdio_retarget` and
`nvmparams`.**

## Sharing model (no git submodules)

Submodules are deliberately avoided (clone/pull friction). `G0B1_Skeleton` is the
**canonical base** for new G0B1 projects: a new project clones Skeleton and inherits the
portable APIs as plain files — **cloning IS the delivery**, so the forward path needs no
mechanism. Back-porting a leaf improvement = copy the portable dir *up* into Skeleton.
Defer any sync tooling until a 2nd live sibling exists. (SwitchTester was cloned from
Skeleton and is where uart-stream + the automation console were developed, so it became
their happenstance landing site; Skeleton is the intended home.)

## uart_stream conventions

Established during the 2026-08-08 migration; config header added 2026-08-12.

- **Config header:** `uart_stream_config_template.h` ships beside the module; copy to
  `App/Inc/uart_stream_config.h` and edit. `uart_stream.h` and `queue.c` include that name
  and nothing else from the application — the `main.h` they used to carry is gone.
  It holds `UART_STREAM_MAX_INSTANCES`, the two flush timeouts, the blocking-write
  deadline, and **the family header** (below).
- **The family header is the adopter's line to edit.** The module is HAL-based, so it
  needs ST's header for the series it builds against; the config header is the only place
  that name appears. Note this is the FIRST of two family boundaries — the second is the
  clock-mux selector list in `u32_uart_stream_kernel_clock()`.
- **CubeIDE indexer trap, measured 2026-08-12.** Writing `#include "stm32g0xx_hal.h"` in
  a config header under `App/Inc` takes CDT from 3 unresolved inclusions / 0.13%
  unresolved names to 21 / 2.1% — a Problems view full of phantoms against an image that
  compiles byte-identical. A `USE_HAL_DRIVER` guard does not help, because a real
  translation unit defines it. **Both G0B1 projects therefore write `#include "main.h"`**,
  which contains only that same include and which CDT has already resolved in `main.c`'s
  context. Costs nothing: the vendored files name only `uart_stream_config.h` either way.
  The *template* keeps the family header, guarded on `USE_HAL_DRIVER` — it is an orphan
  header, and unguarded it poisons the index by itself.
- **Baud getter/setter** `u32_uart_stream_get_baud` / `u32_uart_stream_set_baud` read and
  write `BRR` directly rather than trusting the HAL's cached `Init.BaudRate`, handling
  LPUART's 256x fixed point and USART `OVER8`. The setter is **unguarded by design** and
  returns the rate actually achieved. The flush's TC wait is derived from that rate, so
  `UART_STREAM_FLUSH_TC_TIMEOUT_MS` is a floor rather than the bound.

- **Module bundle:** `App/uart_stream/{uart_stream.c,uart_stream.h,queue.c,queue.h}` —
  family-neutral **except** the register-surface port boundary in `v_uart_stream_service`, which
  assumes the FIFO-capable USART IP (ISR/TDR/RDR/ICR + `_RXFNE`/`_TXFNF` bit names).
  G0/C0/G4/L4/L5/U5/H5/H7/WB/WL share it (STM32H723 is a near-drop-in); legacy USARTv1
  (F1/F2/F4/F7/L1) needs that one surface remapped. Contract + boundary are documented in the
  header and at the function.
- **Target table is app-owned and PART-NAMED:** `uart_stream_target_g0b1.c` (convention:
  `_<part>` suffix). It declares the 8 G0B1 UART handles `__attribute__((weak))`, so ONE
  file lists them all; a UART the build doesn't provision links to NULL and is inert (no
  guard needed — the lookup only compares against a validated non-NULL handle). Do NOT
  `#include "usart.h"` there (weak vs strong extern clash). Weak-attr is a GCC idiom;
  a Keil/IAR port swaps to `__weak`.
- **ISR-service counter** `u32_isr_service_count` (accessor
  `u32_uart_stream_get_isr_service_count`) rides beside `u32_error_count`; a bare
  unconditional `++` at the top of `v_uart_stream_service`. It is the tripwire for the one
  unavoidable footgun: forgetting the `*_it.c` service hook when a new UART vector is
  provisioned (HAL would then service it and kill RX on the first ORE). Both counters have
  `v_uart_stream_clear_*` reset-ers.
- **Adopting a new UART** on a Skeleton-derived build: (1) enable it in CubeMX + regen —
  REQUIRED so the `*_it.c` vector handler exists (uart_stream enables the NVIC channel at
  runtime, but the handler must be generated); (2) add `x_uart_stream_init(&huartN, …)` in
  `v_uart_streams_init()` (app_main.c); (3) IF it is the first UART on a new NVIC vector,
  paste the 2-line service hook + `return` into that vector's USER CODE block. Console
  UART NVIC IPL = 2.
- **Performance note:** USART4/5/6 lack a FIFO and cap at 230400 baud; everything else
  reaches 921600.

## automation_console

Vendored 2026-08-08. `App/automation_console/`, split into:

- `automation_console.c` — **portable core**: executive, framing, dispatch, and the
  builtins (quit / list / version / no-op).
- `automation_commands.c` — **per-app** handlers plus `g_x_acon_command[]`, the table the
  core dispatches into (the port point, mirroring uart_stream's target table). The core owns the
  builtins, so a command module carries only its domain ops.
- `automation_console_config_template.h` — copy to `App/Inc/automation_console_config.h`
  and edit. Added 2026-08-12; the core includes that name and nothing else from the app.

Handlers own their parsing: they receive the raw line and either call `u8_acon_args()`
(parsed comma fields) or read the line directly (raw text). Skeleton's whole command set
is the two example commands that show both idioms: `@` (parsed-args CSV echo) and `$`
(raw-text echo).

Build-gated by **`ACON_ENABLE`** (`automation_console_config.h`): 1 compiles it in, 0
compiles it out to inert inline stubs (~3.7 KB flash / 1.2 KB RAM). Renamed from
`DEV_CONFIG_ENABLE_AUTOMATION_CONSOLE` on 2026-08-12, when the module took ownership of
its own settings.

Everything the core needs from the application is now a macro in that config header:

| Knob | What it is |
|---|---|
| `ACON_ENABLE` | build switch |
| `ACON_TICK_MS()` | free-running millisecond counter (`SYSTEM_TICK()` here) |
| `ACON_PUMP()` | cooperative polling hook (`PUMP_POLLING_TASK()` here) |
| `ACON_ID_PRODUCT` / `_PLATFORM` / `_FIRMWARE` / `_BUILD` | strings the `V` builtin reports |
| `ACON_MAX_ARGS` | widest comma-split `u8_acon_args()` will do |
| `ACON_LINE_MAX` / `ACON_EMIT_MAX` | the module's two static buffers |
| `ACON_IDLE_TIMEOUT_MS` / `ACON_TX_TIMEOUT_MS` | timeouts |

Omitting `ACON_TICK_MS()` or `ACON_PUMP()` is legal and each carries a `#warning` naming
the consequence — a wedged session, or a board that appears to hang. Omitting an
`ACON_ID_*` reports `?` for that field. So the module builds standalone, but never
silently loses a behaviour.

SwitchTester sets `ACON_MAX_ARGS` to 14 for the baud sweep's host-supplied rate list; it
was a `-D` in `.cproject` until the config header existed. Bench-verified both repos
(SwitchTester HIL 48/48 plus a 13-field sweep; Skeleton `V`/`Z`/`@`/`L` on COM3).

## stdout mute — a stdio capability, not a console one

Refactored 2026-08-08. `stdio_retarget.c` owns the flag and the API:
`v_stdout_mute()` / `u8_stdout_is_muted()`.

- The mute applies to **stdout only**; **stderr is the always-through channel** (never
  muted). `i_getline` echoes via **stderr**, so line entry stays visible inside a muted
  span. The cursor-column tracker counts both streams.
- **Contract:** don't put async output on stderr — it would bypass the mute and corrupt a
  SCRIPT frame stream.
- The automation console is just a mute client (asserts on SCRIPT entry, releases on
  exit). It is currently the ONLY caller of `v_stdout_mute()`.
- **Where the mute is meant to go:** wrap **multi-char LINE input only** (`i_getline`).
  The single-char menu idle loop must **NOT** mute — log output must stay visible while
  the menu waits for a keypress. Wiring `i_getline` to self-mute is unbuilt; see the
  backlog doc.
