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
- A **storage driver** is the per-project seam — the D5 pattern, where the module declares
  the interface and the application supplies an implementation. The W25Q128 driver and the
  nvmparams storage driver that wraps it are seam code.

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
