# G0B1_Skeleton

A minimal, **application-agnostic firmware skeleton** for the STM32G0B1RE
(NUCLEO-G0B1RE). Meant to be copied as the starting point for a new G0-class
project — strip nothing, just add your code on top.

## What's in it

- **Startup banner** with reset-source reporting (`app_main.c`).
- **NVM parameter pool** — flash-backed key/value store with auto-commit
  (`nvmparams`, initialized in `v_param_init`).
- **Job queue + dispatcher** — `v_process_next_job`, fed from ISRs/timers.
- **Periodic-interrupt service** — 1 ms tick on **TIM6** driving a 1 s demo
  job and the NVM auto-commit timer.
- **Console debug menu** on the `menusystem` framework — bare `[?]` help plus
  two no-op quick-test stubs; extend `x_debug_top_menu` in `debug_menu.c`.
- **Polled-HAL stdio** — `printf`/`getchar` retargeted to the console UART
  (`stdio_retarget.c`); cooperative non-blocking input so `i_getline` never
  stalls the main loop.

~20 KB flash / ~3.5 KB RAM at `-Og`.

## Adopting this skeleton — change these first

- **Give the NVM pool a project-specific label.** `NVM_POOL_LABEL` in
  `device_config.h` ships as `"UNNAMED"` — a deliberate placeholder. Every project
  that can be loaded onto the same board needs a *different* one. Setting it to
  `PRODUCT_NAME` is a reasonable choice, since a fork then inherits a distinct
  label for free.

  **The check is already wired up.** `v_param_init()` compares the stored label
  against `NVM_POOL_LABEL` after pool init and before any `x_nvm_get()`; a pool
  that does not match is wiped and rebuilt from defaults. While the placeholder is
  still in place it also logs a warning at boot, so an unadopted clone says so
  rather than waiting to be noticed.

  Why it matters: the `.nvmdata` section is `NOLOAD`, so the pool survives
  reflashing. On a shared board this firmware would otherwise be handed a pool
  written by another project — valid signature, self-consistent, entirely foreign
  IDs. Neither the signature nor a CRC detects that, and no init policy helps: a
  foreign pool is perfectly *valid*. Only the label carries provenance. It has
  already cost a bench session once.

  Note the consequence on a shared bench: two projects alternating on one board
  will each wipe the other's pool at boot. That is intended — a foreign pool is
  not worth preserving — but it means parameters do not survive a swap.

## Layout

| Path | Purpose |
|---|---|
| `App/` | Product code — where your work goes |
| `Core/` | CubeMX-generated startup, clocks, peripheral init |
| `Drivers/` | STM32G0xx HAL + CMSIS |
| `scripts/` | Headless build / flash helpers |

## Board resources (kept in `App/Inc/macros.h`)

- `DEBUG_UART_HANDLE` = **USART2** (ST-Link VCP) — console, **921600 8N1**.
- `PERIODIC_INT_TIMER_HANDLE` = **TIM6** — 1 ms periodic tick.
- `DELAY_US_TIMER_HANDLE` = **TIM7** — microsecond delays (`v_delay_us`).
- Nucleo user button on **EXTI13**, LD2 on **PA5** (free for a heartbeat).

If you retarget these in CubeMX, update the handles in `macros.h`.

## Build

```
scripts\build.ps1            # Debug (default, clean build)
scripts\build.ps1 Release
```

Headless STM32CubeIDE build (locates `stm32cubeidec.exe` automatically);
artifacts land in `Debug\` / `Release\`. Or just open the project in
STM32CubeIDE and build normally.

## Flash

```
scripts\flash.ps1            # flashes Debug\G0B1_Skeleton.elf
scripts\flash.ps1 --list     # list connected ST-Link probes
```

Targets the ST-Link serial in `scripts\bench.defaults.json`. On another
bench, drop a gitignored `scripts\bench.defaults.local.json` overriding
`stlink_sn` / `com_port` / `baud`.

## Note: float printf/scanf is OFF

To keep the image small, newlib-nano float `printf`/`scanf` is disabled
(no `%f`/`%g`) — it pulls in ~24 KB of double-to-string and soft-float math.
The skeleton needs none. If your project must format floats, re-enable
**"Use float with printf/scanf from newlib-nano"** in the linker options
(both Debug and Release), or — preferred on this M0+ core — keep integers
and use fixed-point scaling instead.
