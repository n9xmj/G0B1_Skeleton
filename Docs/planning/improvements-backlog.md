# Improvements backlog

> Promoted from session notes on 2026-08-08. Captured-but-unbuilt ideas for the portable
> APIs. None started yet. Companion: [`portable-apis-strategy.md`](portable-apis-strategy.md).

## 1. Logging API → own source subdir

Already API-ized; just relocate to `App/logging/` (or similar), like uart-stream /
automation-console. Mechanical migration.

## 2. Logging macros: binary switches → compile-time LOG LEVELS

Today the message classes in `debug_config.h` are on/off booleans. Change to a level
scheme: a global constant desired level — none (0), error, warning, info, debug (4) —
compared in the `LOGxx()` macros against each message's `LOG_xxxx` constant. Keep
EVERYTHING `const`/`#define` so the `if()` in the macro folds at compile time and the
guarded code is dead-code-eliminated when the test is false (holds at any `-O` above
`-O0`).

## 3. nvmparams → own subdir AND fully HW-independent

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

## 4. Wire i_getline to self-mute stdout

Mechanism exists (see the stdout-mute section of the strategy doc), not wired. Have
`i_getline` bracket itself: `v_stdout_mute(1)` on entry, `v_stdout_mute(0)` on exit, so
async job logs can't scramble a line being typed; the stderr echo (already done) keeps
keystrokes visible. **The single-char menu idle loop must stay UNMUTED** (logs visible
while waiting for a key) — only multi-char line entry mutes. Today the ONLY caller of
`v_stdout_mute()` is the automation-console SCRIPT session; `i_getline` just moved its
echo to stderr and asserts nothing yet.
