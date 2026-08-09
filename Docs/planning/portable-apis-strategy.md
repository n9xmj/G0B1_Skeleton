# Portable APIs — strategy & conventions

> Promoted from session notes on 2026-08-08 so it travels with the code.
> `G0B1_Skeleton` is the canonical home for this work; keep this doc as the source of
> truth here. Companion: [`improvements-backlog.md`](improvements-backlog.md).

## The portable-API set

These subsystems are being (re)designed as **portable, drop-in APIs**, each in its own
subdirectory, to be reused across STM32 projects:

- **menusystem** — DONE. Lives portable in `LED_Strip_Controller_G474`.
- **uart_stream** — DONE 2026-08-08. Migrated into `G0B1_Skeleton`, bench-verified.
- **automation_console** — DONE 2026-08-08 (see below).
- **nvmparams** — LATER; wants a partial refactor first (see the backlog doc).

## Sharing model (no git submodules)

Submodules are deliberately avoided (clone/pull friction). `G0B1_Skeleton` is the
**canonical base** for new G0B1 projects: a new project clones Skeleton and inherits the
portable APIs as plain files — **cloning IS the delivery**, so the forward path needs no
mechanism. Back-porting a leaf improvement = copy the portable dir *up* into Skeleton.
Defer any sync tooling until a 2nd live sibling exists. (SwitchTester was cloned from
Skeleton and is where uart-stream + the automation console were developed, so it became
their happenstance landing site; Skeleton is the intended home.)

## uart_stream conventions

Established during the 2026-08-08 migration.

- **Module bundle:** `App/uart_stream/{uart_stream.c,uart_stream.h,queue.c,queue.h}` —
  family-neutral **except** the register-surface seam in `v_uart_stream_service`, which
  assumes the FIFO-capable USART IP (ISR/TDR/RDR/ICR + `_RXFNE`/`_TXFNF` bit names).
  G0/C0/G4/L4/L5/U5/H5/H7/WB/WL share it (STM32H723 is a near-drop-in); legacy USARTv1
  (F1/F2/F4/F7/L1) needs that one surface remapped. Contract + seam are documented in the
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
  core dispatches into (the seam, mirroring uart-stream's target table). The core owns the
  builtins, so a command module carries only its domain ops.

Handlers own their parsing: they receive the raw line and either call `u8_acon_args()`
(parsed comma fields) or read the line directly (raw text). Skeleton's whole command set
is the two example commands that show both idioms: `@` (parsed-args CSV echo) and `$`
(raw-text echo).

Build-gated by **`DEV_CONFIG_ENABLE_AUTOMATION_CONSOLE`** (`device_config.h`): 1 compiles
it in, 0 compiles it out to inert inline stubs (~3.7 KB flash / 1.2 KB RAM). Requires the
`PUMP_POLLING_TASK` macro in `platform.h`. Bench-verified both repos (SwitchTester HIL
47/47; Skeleton SCRIPT + human echo on COM3).

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
