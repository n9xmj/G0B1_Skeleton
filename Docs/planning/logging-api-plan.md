# Logging API — vendoring + compile-time log levels

**Feature:** rework the logging subsystem into a properly vendored, drop-in portable
module, and replace its per-class on/off booleans with a compile-time **log level**
scheme that folds away when a message is below threshold.

**Code home:** `App/logging/` (engine + sugar + templates), `App/common/ANSI.h` (vendored
leaf), `App/Inc/logging_config.h` + `App/Src/logging_port.c` (app-owned seams). Was
`App/Inc/logging.h`, `App/Src/logging.c`, `App/Inc/debug_config.h`, `App/Inc/ANSI.h`.

**Parent docs:** [`portable-apis-strategy.md`](portable-apis-strategy.md) (conventions),
[`improvements-backlog.md`](improvements-backlog.md) (items 1 and 2 are this work).

**Status:** DONE for logging — both phases built, bench-verified in Skeleton and
SwitchTester, and the conventions written into the strategy doc. Open rows are follow-on
work tracked here so they are not lost (I8, I11, S4, and the LED_Strip back-port).

**Working mode:** decision-log model — one question at a time in chat, everything else
parked on the board below. Agent never silently resolves a 🔴 or 🟡.

---

## Brief

The logging API already exists and works, but it is not portable in the way the other
vendored modules (`uart_stream`, `automation_console`) now are: the macro layer lives
inside the app-owned `debug_config.h`, so every project re-copies and independently
drifts the macro definitions; `logging.c` hard-includes the family HAL header purely for
`HAL_GetTick()`; and its `ANSI.h` dependency lives outside the module directory
altogether. A third project, `LED_Strip_Controller_G474`, has already solved most of the
organisational half — this plan adopts that structure and finishes the job.

The functional half is the level scheme. Each message class (`LOG_SYSTEM`, `LOG_JOBS`,
`LOG_EXTI`) was a 1/0 boolean tested by an `if()` inside the macro; it now carries a
**verbosity tier** compared against one global `LOG_LEVEL` ceiling, with everything
`const`/`#define` so the comparison folds at compile time and the guarded call is
dead-code-eliminated at any `-O` above `-O0` — measured, and the format-string literals go
with it. The axis question was D1; it resolved to the class carrying the tier, which left
every existing call site unchanged.

**Both phases are built and bench-verified.** What remains is documentation (T1–T3),
retrofitting the two older vendored modules to the finished convention (I11), and two
small unrelated defects parked here so they are not lost (I8, I12).

---

## Big Board

| ID | Status | Subject (one line) |
|----|--------|--------------------|
| **D1** | 🟢 | Level axis — class carries a **verbosity** tier; one global `LOG_LEVEL` ceiling |
| **D2** | 🟢 | Module directory is named for its core source file, verbatim — `App/logging/` |
| **D3** | 🟢 | `ANSI.h` moves to `App/common/`, chartered as vendored leaves only |
| **D4** | 🔵 | Runtime-settable log level (menu / acon command) |
| **D5** | 🟢 | Seam shape — module declares extern prototypes, app defines them in a port source |
| **D6** | 🟢 | Rename/split `debug_config.h` → `logging_config.h`; delete `DEBUG_LOGGING` + `INCLUDE_TESTS` |
| **D7** | 🟢 | `debug_config.h` is deleted; `DEBUG_MENU` folds into `device_config.h` |
| **S1** | 🟢 | "Logging off" is `LOG_LEVEL_QUIET`; the empty-macro block is deleted |
| **S2** | 🔵 | Severity ↔ colour interaction — does severity override the class colour? |
| **S3** | 🟢 | Dissolved by D1's reversal — one guard, `0` means quiet on both sides |
| **S4** | 🔵 | stdio-retarget contract differs across the three projects (`_read` especially) |
| **I1** | 🟢 | Three-layer split adopted: engine / sugar / template |
| **I2** | 🟢 | Seam files live in `App/Inc` + `App/Src`, edited in place |
| **I3** | 🟢 | Timestamp reaches the module via an app-defined extern (superseded by D5) |
| **I4** | 🟢 | All macros wrapped in `do { } while (0)` |
| **I5** | 🟢 | `ANSI.h` include CASE normalised — not a missing include, as first recorded |
| **I6** | 🟢 | `ANSI.h` copies reconciled; Skeleton canonical, include guard added |
| **I7** | 🟢 | `platform.h` guard renamed to `PLATFORM_H`; LL includes retained |
| **I8** | 🟡 | NVM auto-commit delay defined twice — keep `device_config.h`'s, drop `platform.h`'s |
| **I9** | 🟢 | `PRINTF_ATTR` now defined once, in `logging.h` |
| **I10** | 🟢 | Call-site sweep — none needed; D1 leaves every existing call site unchanged |
| **I11** | 🔵 | Existing vendored modules violate the dependency rule — deferred until it blocks |
| **I12** | 🟢 | `ANSI_FG_RGB` / `ANSI_BG_RGB` missing trailing `m` — fixed |
| **I13** | 🟢 | `DPRINTF_TS` called a nonexistent function; never used, so never caught |
| **I14** | 🟢 | `DPRINTF`/`DPRINTF_TS` always compile too, via `LOG_IN_DEBUG_BUILD` |
| **T1** | 🟢 | Tier model, dependency rule and naming folded into `portable-apis-strategy.md` |
| **T2** | 🟢 | Seam inventory table added to `portable-apis-strategy.md` |
| **T3** | 🟢 | `decision-log-model.md` ported into this repo |

## Wish list (v2+)

| ID | Subject |
|----|---------|
| **W1** | Runtime log-level control from the debug menu / automation console |
| **W2** | Split the toolchain sugar out of `platform.h` into `App/common/compiler.h` |
| **W3** | Back-port the finished module — SwitchTester DONE 2026-08-09; LED_Strip pending |

---

## LOCKED CONTEXT

Established; do not re-litigate unless explicitly reopened.

- **`G0B1_Skeleton` is the canonical base.** No git submodules — cloning Skeleton *is*
  the delivery mechanism for a new project. Back-porting a leaf improvement means copying
  the vendored directory *up* into Skeleton.
- **Seams are edited in place in the app's own directories** (I2, user, this session) —
  the FreeRTOS `FreeRTOSConfig.h` / lwIP `lwipopts.h` pattern. There is no separate
  "port" directory.
- **Vendored modules must not depend on app-specific code.** Final formulation (D5): a
  vendored module may `#include` the C library, other vendored modules, and its own
  `<module>_config.h` (app-owned, from the module's template). Everything else it needs
  from the application it **declares as an `extern` prototype** and the application
  **defines** — typically in a port source copied from the module's template. The module
  never includes an app-authored header of macros, and never includes the HAL if it can
  avoid it.
- **Everything stays `const`/`#define`** so the level test folds at compile time and the
  guarded code is eliminated. This is the point of the exercise, not a nice-to-have.
  `LOG_LEVEL` is deliberately not a runtime variable: a flash-constrained part such as a
  G030 may carry hundreds of `LOGxx()` invocations, and the elimination is the design.
  This is the standing reason D4 / W1 stay deferred.
- **The ladder measures VERBOSITY, not severity** — `QUIET 0`, `ALWAYS 1`, `ERROR 2`,
  `WARNING 3`, `INFO 4`, `DEBUG 5`, ascending from terse to chatty. The emit predicate is
  therefore `tag <= LOG_LEVEL`. A tag's number is "the verbosity tier at which this class
  becomes visible"; `LOG_LEVEL` is "how verbose I am willing to be". **`0` means quiet on
  both sides** — that is the design constraint everything else serves.
- **A vendored module's directory is named for its core source file, verbatim** (D2) —
  `logging.c` → `App/logging/`, `uart_stream.c` → `App/uart_stream/`,
  `automation_console.c` → `App/automation_console/`. Underscores, not hyphens; no `-api`
  suffix. **The same names are used in all three projects.** Applies to every vendored
  library created from here on. Third-party libraries keep their upstream names.
- **`App/common/` holds vendored leaves only** (D3) — files copied unchanged between
  projects. Seams never go there; they live in `App/Inc` / `App/Src` per I2.
- **Test / HIL code inclusion belongs behind a compiler command-line define**, in the style
  of `DEBUG` — i.e. its own build configuration — **not** an in-tree config symbol. User
  convention, this session, carried from a commercial project. This is why `INCLUDE_TESTS`
  was deleted rather than repaired (D6): the mechanism was wrong, not just unused.
- **Because Skeleton is the starter project, its own `App/Inc` seam files ARE the
  templates.** A `*_template.h` shipped inside a module directory serves *foreign*
  adopters (e.g. back-porting into LED_Strip), not Skeleton clones.

**Facts verified in-tree this session** (not assumptions):

- `logging.h` is **byte-identical** between Skeleton and LED_Strip. `logging.c` differs by
  exactly **one line** — `stm32g0xx_hal.h` vs `stm32g4xx_hal.h`.
- `ANSI.h` contains **zero `#include` lines** — a pure leaf header.
- **Nothing** in either project uses `#if LOG_<CLASS>` at preprocessor level, and there
  are **no direct `v_log*()` calls** outside the logging module itself. The class
  constants are consumed only by the macros, so redefining them is contained.
- `ANSI.h` is `#include`d only by `logging.h`, but its macros are *used* by
  `menusystem.c` (Skeleton) and `term.c` + `debug_menu.c` (LED_Strip) — all three reach
  it transitively.
- Nothing under `App/` uses any `LL_GPIO_*` or `LL_EXTI_*` symbol **yet** — but the two LL
  includes in `platform.h` are deliberate (user, this session): `platform.h` is where
  GPIO set/clear, mode reconfiguration, open-drain control and EXTI mask macros live once
  an app grows past bare-bones. They stay.
- **Include map.** `device_config.h` is the app's aggregator — it pulls `debug_config.h`,
  `main.h`, `platform.h`, `globals.h` plus the C library, and nine app modules include it
  to get everything in one line. Only `nvmparams.c` includes `platform.h` directly.
- `debug_config.h` is `#include`d by exactly **one** file, `device_config.h`. Of the build
  flags it defines, `DEBUG_LOGGING` and `INCLUDE_TESTS` have **no consumers anywhere** in
  the tree, and `DEBUG_MENU` has exactly one (`nvmparams.c:857`). Renaming or splitting it
  is therefore a one-line change at the include site. See D6.
- **Rows written before D6 refer to `debug_config.h`.** If D6 lands, read those as
  `logging_config.h` — the logging half of the split.
- **The already-vendored modules do not currently obey the dependency rule:**
  `automation_console.c` and `automation_console.h` include `device_config.h` (the app
  aggregator, tier 3); `uart_stream.h` and `queue.c` include `main.h` (CubeMX-generated).
  The convention is so far aspirational, not enforced. See I11.
- Current Skeleton call-site count: 9 `LOGCT`, 8 `LOG`, 5 `LOGC`, 4 `LOG_PLAIN`,
  4 `LOGC_PLAIN`, 3 `LOGCT_PLAIN`, 3 `DPRINTF`, 3 `DPRINTF_TS`, 3 `RPRINTF`.

---

## Detail sections

### D1 — Level axis *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** the backlog says "a global constant desired level … compared against each
message's `LOG_xxxx` constant." That reads three ways, and they produce different code.

**Options considered:**

- **(A) Class *is* the severity.** `LOG_SYSTEM` becomes `3` (info), `LOG_JOBS` becomes `4`
  (debug); one global `LOG_LEVEL`; the macro tests `if (LOG_SYSTEM <= LOG_LEVEL)`.
  **Zero call-site churn** — every existing `LOG(LOG_SYSTEM, …)` compiles unchanged.
  Cost: a class carries exactly one severity forever, so an *error* raised inside the jobs
  subsystem is suppressed at the same threshold as jobs debug chatter — which is the
  message you most want to survive.
- **(B) Severity replaces class.** Tags become `LOG_ERROR` / `LOG_WARNING` / `LOG_INFO` /
  `LOG_DEBUG`; the tag string *is* the severity and colour falls out of it naturally.
  Simplest possible scheme. Cost: the SYSTEM / JOBS / EXTI categorisation disappears
  entirely.
- **(C) Two independent axes.** Class stays a category (tag + colour as today) but its
  constant becomes a *per-class verbosity ceiling*; severity moves to the call site via
  severity-named macros — `LOGE(LOG_JOBS, …)`, `LOGW`, `LOGI`, `LOGD` — folding on
  `if (SEV <= LOG_JOBS && SEV <= LOG_LEVEL_MAX)`. Gives "jobs at info, exti at error,
  everything else off" *and* errors always survive. Cost: a one-time sweep to pick a
  severity at each existing call site (~30 here, more in SwitchTester).

**Leaning / recommendation:** was **(C)**, on the grounds that an error in a quiet
subsystem should still reach the bench. Not taken — see Resolution.

**Resolution:** **(A) — the class carries the level, one global ceiling.** User decision,
this session.

> **Reopened and re-locked, same session.** The ladder was first written as *ascending
> severity* (`DISABLED 0, DEBUG 1, INFO 2, WARNING 3, ERROR 4, ALWAYS 255`) with the
> predicate `tag >= LOG_LEVEL`. That was wrong: the constants are **verbosities**, not
> severities, and the two orderings run opposite ways. The mismatch is what produced S3 —
> under ascending severity, a global of `0` was the *most permissive* value the ladder
> could express, which is the reverse of what its name promised. Ladder reversed and
> re-locked below; the original is preserved here for audit.

Ladder, symbolic, **ascending verbosity** — terse at the bottom, chatty at the top:

```c
#define LOG_LEVEL_QUIET         0    // never emitted — valid for a tag AND for the global
#define LOG_LEVEL_ALWAYS        1    // shown whenever logging is on at all
#define LOG_LEVEL_ERROR         2
#define LOG_LEVEL_WARNING       3
#define LOG_LEVEL_INFO          4
#define LOG_LEVEL_DEBUG         5    // chattiest
```

One global verbosity ceiling referenced by the macros:

```c
#define LOG_LEVEL               <one of the LOG_LEVEL_* above>
```

Per subsystem, the class constant becomes its verbosity tier — the existing `_TAG` and
`_COLOR` companions are unchanged:

```c
#define LOG_SYSTEM              LOG_LEVEL_ERROR
#define LOG_SYSTEM_TAG          "SYSTEM"
#define LOG_SYSTEM_COLOR        LOGC_BRIGHT_MAGENTA
```

**Reading the numbers:** a tag's value is *the verbosity tier at which this class becomes
visible* — equivalently, how much noise you must accept to see it. ERROR is cheap to show
(2); DEBUG costs a lot (5). `LOG_LEVEL` is how verbose you are willing to be. The predicate
is therefore `tag <= LOG_LEVEL`, plus one guard for the `QUIET` tag (S3):

```c
#define LOG_EMIT(tag)   ( (tag) != LOG_LEVEL_QUIET && (tag) <= (LOG_LEVEL) )
```

A global of `LOG_LEVEL_WARNING` passes ALWAYS, ERROR and WARNING and drops INFO and DEBUG.
A global of `LOG_LEVEL_QUIET` passes nothing — including ALWAYS, so the master switch beats
ALWAYS arithmetically rather than by legislation.

**Note on `LOG_LEVEL_ALWAYS`:** it moves from the *top* of the original ladder (255) to the
*bottom* (1). Its meaning is unchanged — "shown whenever logging is on at all" — but its
numeric position inverts, which surprises anyone who saw the first draft.

**Optional, not adopted:** a global-only `LOG_LEVEL_ALL 255` meaning "show everything,
including tiers added later". Useful future-proofing, but it reads uncomfortably close to
`ALWAYS`; left out unless it is asked for.

**Rationale (user):** everything stays a compile-time constant so the `if()` and the
logging call fold out at any `-O` above `-O0`. `LOG_LEVEL` is deliberately *not* a runtime
variable — on a flash-constrained part such as a G030 an application may carry hundreds of
`LOGxx()` invocations, and the elimination is the point of the design, not a side benefit.
This is the standing reason D4/W1 stay deferred.

**Consequences:**

- **Zero call-site churn.** Every existing `LOG(LOG_SYSTEM, …)` compiles unchanged (I10).
- **`#define LOG_JOBS 0` keeps working** — `0` is `LOG_LEVEL_DISABLED`, so tags already
  set to 0 stay off with no edit.
- **The legacy 0/1 scheme is preserved by `#define LOG_LEVEL LOG_LEVEL_DEBUG`.** A tag left
  at literal `0` fails the guard and stays off; a tag left at literal `1` passes
  `1 <= 5` and stays on. **One line, one edit, no refactor** — the documented adoption path
  for a project that wants the new macros without reclassifying its tags, and it belongs in
  `logging_config_template.h` as such. Under the reversed ladder this now reads honestly as
  *"maximum verbosity"*, rather than working by the coincidence of `DEBUG == 1` in the
  original draft.
- **Migration for Skeleton and SwitchTester** (user decision, this session): mechanical —
  every tag currently `1` becomes `LOG_LEVEL_DEBUG`, and `LOG_LEVEL` is set to
  `LOG_LEVEL_DEBUG`. Both sides move together, so every class still emits exactly as it
  does today and the change carries no behavioural risk. Assigning subsystems their real
  verbosity tiers is later work, done when someone actually wants to turn a firehose down.
- A subsystem needing messages at two severities takes a second tag (e.g. `LOG_SYSTEM` at
  ERROR plus `LOG_SYSTEM_DBG` at DEBUG, free to share the same `_TAG` string). That is the
  in-model answer to the concern raised under option (C); no macro change required.

---

### D2 — Module directory naming *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** `App/logging/` or `App/logging-api/`?

**Options considered:** LED_Strip uses `logging-api` and `menu-api` but plain
`uart-stream`, `spiflash`, `tlsf`, `littlefs` — the suffix is already inconsistent *within*
that project. Skeleton uses `uart-stream` and `automation-console`, no suffix.

**Resolution:** **`App/logging/`.** User decision, this session, with a general rule
attached: **a vendored module's directory is named for its core source file** — `logging.c`
→ `App/logging/`, `menusystem.c` → `App/menusystem/`, and likewise for every vendored
library created from here on. The `-api` suffix is dropped.

**Rider — resolved 2026-08-09.** The rule is unambiguous for single-word modules, but the
two multi-word directories used to separate differently from their files: `App/uart-stream`
held `uart_stream.c`, and `App/automation-console` held `automation_console.c` — hyphen in
the directory, underscore in the file.

**User decision: the directory name matches the main source file name verbatim, underscores
included, and the same names are used across all three projects.** Renamed in Skeleton to
`App/uart_stream/` and `App/automation_console/`; SwitchTester and LED_Strip take the same
names as they are migrated (LED_Strip's `logging-api/` → `logging/`). Verified as a pure
rename: clean build, `text` unchanged at 32920.

Scope boundary worth respecting: the rule governs **our** vendored APIs. Third-party
libraries carried in LED_Strip (`littlefs`, `tlsf`, `berry-lang`) keep their upstream
identity — renaming those would fight the projects they come from and buys nothing.

---

### D3 — Home for `ANSI.h` and other portable shared leaves *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** `ANSI.h` is a pure leaf used by logging *and* by terminal/menu code. Where
does it live?

**Options considered:**

- **Inside `App/logging/`** — rejected. `term.c` / `menusystem.c` do not depend on
  logging; they depend on ANSI. Putting it there creates a dependency edge that is not
  real and forces a project wanting `term` but not `logging` to vendor logging anyway.
- **Leave it in `App/Inc/`** — rejected. It is vendored content, not app content, and the
  two copies have already drifted (see I6), which is exactly the failure this convention
  exists to prevent.
- **Its own directory, `App/ansi/`** — honest but accumulates one-file directories.
- **`App/common/`** — a vendored directory for portable shared leaves, chartered
  *strictly* to tier-1 content: things copied unchanged between projects, never edited.

**Leaning / recommendation:** **`App/common/`**, with the charter written into the
strategy doc. The user's original instinct was right; it only felt unresolvable because
the candidate contents mixed two opposite kinds of file — portable leaves (`ANSI.h`,
`utils.c`) that are copied unchanged, and *seams* (`debug_config.h`,
`uart_stream_target_g0b1.c`, `device_config.h`) that are meant to be rewritten in every
project. I2 removes the seams from the picture, and `common/` becomes coherent: it holds
only files where the answer to "do I edit this on clone?" is always *no*.

**Resolution:** **`App/common/`.** User decision, this session — `ANSI.h` is the odd one
out, depended on by both the app and by vendored libraries, and `common/` is the best
available home. Charter: **vendored leaves only** — files where the answer to "do I edit
this on clone?" is always *no*. Seams never go here (I2).

Open sub-question for when the directory grows: does `utils.c`/`utils.h` qualify, or does
it have app dependencies that disqualify it? Needs an audit, not an assumption — and it is
not part of phase 1.

---

### D4 — Runtime-settable log level

**Status:** 🔵 deferred · **Needs user:** no

**Question:** should the level be changeable at run time from the debug menu or automation
console, in addition to the compile-time fold?

**Options considered:** the two compose cheaply —
`if (COMPILE_LEVEL >= sev && g_runtime_level >= sev)`. The compile-time test still folds,
and the runtime test costs one load and branch only at sites that survived the fold.

**Leaning / recommendation:** deferred to W1. The backlog is emphatic that everything stay
`const` so it folds, and mixing in a runtime path now would muddy D1. Revisit once D1 is
locked and built.

**Resolution:** deferred to W1 by scope choice, not rejected on merit.

---

### D5 — Seam shape: how a vendored module reaches the application *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** `platform.h` is one of the app's *core* includes, pulled into almost every
app module (via `device_config.h`) alongside `main.h`. It is also where GPIO set/clear,
mode-reconfiguration, open-drain and EXTI-mask macros go as an app grows — which is why the
LL includes are there. I3 originally proposed it as the header vendored modules include for
`SYSTEM_TICK()` / `PUMP_POLLING_TASK()`. Those two roles conflict. So how does a vendored
module actually reach the app?

**Options considered:**

- **Leave one fat `platform.h`; vendored modules include it.** Every vendored module then
  compiles against the whole board definition, and a syntax error in the app's GPIO macros
  breaks the logging build.
- **Extract a thin app-authored contract header (`app_hooks.h`) that modules include.**
  Proposed and **rejected** — it inverts ownership. The app would author the contract and
  the module would consume it, which is backwards from how the reference libraries work,
  and it couples every vendored module to one shared file.

**Resolution:** **The module declares, the app defines.** User direction, this session,
citing the commercial/FOSS precedent: a vendored library publishes `extern` prototypes for
the bridge functions it needs and expects the application to supply the definitions in
whatever form it likes — typically by copying the library's template source module and
editing it in place. The library never includes an app-authored header of macros.

Reference implementations of exactly this: FatFs declares `disk_read()` / `disk_write()` /
`disk_status()` / `disk_ioctl()` / `get_fattime()` in `diskio.h` and the user implements
them in `diskio.c` from a template; lwIP declares `sys_now()` and the port implements it in
`sys_arch.c`; FreeRTOS declares its `vApplicationXxxHook()` prototypes and the app supplies
bodies; littlefs takes read/prog/erase/sync as function pointers in `lfs_config`.

**The seam therefore has two halves**, both app-owned, both copied from templates
(consistent with I2):

| Half | What it is | Precedent | Here |
|---|---|---|---|
| **Config header** | constants and feature switches; the module names the file and includes it | `ffconf.h`, `lwipopts.h`, `FreeRTOSConfig.h` | `debug_config.h`, `uart_stream_config.h`, `automation_console_config.h` |
| **Port source** | defines the `extern` functions the module declared | `diskio.c`, `sys_arch.c` | `logging_port.c` |

**Consequences:**

- `app_hooks.h` is not created. There is no shared app-authored contract header.
- `platform.h` stays **exactly as it is** — LL includes, future GPIO/EXTI macros, handle
  assignments — and simply stops being something any vendored module knows about. The
  question of where the hardware macros live never needed an answer; they were always in
  the right place.
- `logging.h` declares `extern uint32_t u32_log_timestamp_ms(void);`. `logging.c` then
  includes **nothing but the C library** — no HAL, no `platform.h`. Strictly better than
  the `SYSTEM_TICK()` route, which would still have dragged in a family-specific header.
- The existing `v_app_polling_task` weak symbol is **already this pattern**, correctly
  built — module declares, app defines, absent definition degrades to inert. Its only
  defect is that the declaration sits in `platform.h` rather than in the module that needs
  it. Moving it is part of I11.

**Naming note:** the config half is named `<module>_config.h` throughout. D6 renames
logging's to match.

**Open sub-point (leaning, not blocking):** give `u32_log_timestamp_ms()` a **weak default**
in `logging.c` returning 0, so an adopting project links and runs before it has written a
port source, with timestamps simply reading `0.000`. `v_app_polling_task` deliberately uses
the *other* form — weak declaration plus a null guard — because it has no sensible default.
The two cases differ; use the form that fits each.

---

### D6 — Rename and split `debug_config.h` *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** `debug_config.h` predates the logging API entirely — the name is inertia from
before `logging.c` existed. Its primary job today is logging configuration, but it also
carries app build options that are not logging's business. Should the logging half move to
a better-named header?

**Options considered:** keep one file and accept the name, or split by owner.

**Leaning / recommendation:** **split.** Two independent arguments:

1. **The name is wrong for what it does.** Its content is the logging seam.
2. **The convention we just settled requires it.** D5/I11 name every module's config header
   `<module>_config.h` — `uart_stream_config.h`, `automation_console_config.h`. Under that
   rule `debug_config.h` is the only odd one out, and `logging_config.h` is not merely a
   nicer name, it is the name the convention demands.

Proposed split:

| File | Owner | Contents |
|---|---|---|
| `logging_config.h` | app, from `logging_config_template.h` | `LOG_LEVEL`, the tag triplets, its own `#ifndef DEBUG` response, trailing `#include "log_helpers.h"` |
| `debug_config.h` | app | `DEBUG_MENU`, `INCLUDE_TESTS`, and the `#ifndef DEBUG` guard for those |

Each header answers `DEBUG` for its own symbols rather than sharing one block, so neither
depends on the other or on include order.

**Cost is one line.** `debug_config.h` is included by exactly one file — `device_config.h`
— which gains an `#include "logging_config.h"` alongside it. Every other module reaches
both through that aggregator and needs no edit. The template renames to
`logging_config_template.h`.

**Rider — delete `DEBUG_LOGGING`.** It has **zero consumers** outside `debug_config.h`
itself, and under D1 `LOG_LEVEL` is already the master switch: `#ifndef DEBUG` →
`#define LOG_LEVEL LOG_LEVEL_DISABLED`. Keeping both would leave two symbols meaning
"logging off" in two files, which is the same drift trap as I8's duplicated NVM delay.
Recommend one knob: `LOG_LEVEL`. This also simplifies S1's mechanism — with `DEBUG_LOGGING`
gone there is no `#if DEBUG_LOGGING` wrapper around the tag triplets to remove, because
there is nothing left to wrap them in.

**Resolution:** **split and rename as proposed above, and delete both dead build flags.**
User decision, this session.

- **`DEBUG_LOGGING` deleted.** Redundant under D1 — `DEBUG_LOGGING` ≈ `LOG_LEVEL`, and
  `LOG_LEVEL` is the more expressive of the two. One knob.
- **`INCLUDE_TESTS` deleted.** Never used in this tree; probably a remnant of an older
  project, possibly once gating HIL or unit-test code. See the convention note in LOCKED
  CONTEXT for where that belongs instead.
- `DEBUG_MENU`'s only consumer is `nvmparams.c:857` (`#if DEBUG_MENU`) — a module slated
  for vendoring, using an app build flag. Another instance of I11; it becomes part of
  `nvmparams_config.h` when that module is done. What happens to `DEBUG_MENU` in the
  meantime is D7.

---

### D7 — Does `debug_config.h` survive the split? *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** after D6 removes the logging half, `DEBUG_LOGGING` and `INCLUDE_TESTS`, the
file contains exactly one symbol — `DEBUG_MENU` — plus a `#ifndef DEBUG` guard for it. Is a
one-symbol header worth keeping?

**Options considered:**

1. **Keep `debug_config.h`** as the home for future app build options. Costs nothing, but
   it is currently a file with one define whose sole consumer is a module that is on its
   way out of the app tree.
2. **Fold `DEBUG_MENU` into `device_config.h` and delete the file.** `device_config.h` is
   already titled *"Product options and constant parameter settings"* and already holds a
   build switch of exactly this kind — `DEV_CONFIG_ENABLE_AUTOMATION_CONSOLE`. A debug-menu
   inclusion switch is the same species and would sit naturally beside it, arguably as
   `DEV_CONFIG_ENABLE_DEBUG_MENU` for consistency with the existing naming.

**Leaning / recommendation:** option 2. It leaves the tree with one app-config header
instead of two, and removes a file whose entire remaining purpose is a symbol that is
itself nearly vestigial — `DEBUG_MENU` does not gate `debug_menu.c`'s compilation, only one
`#if` inside `nvmparams.c`, and once nvmparams is vendored that consumer moves to
`nvmparams_config.h`, leaving `DEBUG_MENU` with zero consumers.

**Resolution:** **option 2 — `debug_config.h` is deleted.** User decision, this session.
`DEBUG_MENU` moves into `device_config.h` beside `DEV_CONFIG_ENABLE_AUTOMATION_CONSOLE`,
**keeping its current name**: renaming it to `DEV_CONFIG_ENABLE_DEBUG_MENU` would be more
consistent but touches `nvmparams.c`, a file about to be restructured anyway — so the
rename rides along with the nvmparams vendoring instead. The app is left with one
app-config header (`device_config.h`) plus one module-config header per vendored module.

---

### S1 — What "logging off" means

**Status:** 🟡 · **Needs user:** no (recommendation only)

**Question:** today `DEBUG_LOGGING == 0` expands the macros to *nothing*.

**Options considered:** keep the empty-macro `#else` block, or express "off" as
`LOG_LEVEL_MAX = LOG_LEVEL_NONE` and delete the block.

**Leaning / recommendation:** delete it. With empty macros, disabled call sites are never
compiled — format-string typos and stale variable names sit there rotting until someone
re-enables logging, at which point a build that was green for months breaks. Expressing
"off" as a level means the `if()` is always compiled (so `PRINTF_ATTR` keeps type-checking
the arguments) and folds away at any `-O` above `-O0`. Strictly better, and it removes an
entire duplicated macro block from the vendored header.

Caveat to document either way: arguments in a folded-away call are **not evaluated**, so
`LOG(LOG_JOBS, "%d", counter++)` silently loses the increment. True today, true after.

**Worked example of the defect.** With logging off today, this compiles clean:

```c
LOG(LOG_SYSTEM, "count=%d", some_stale_variable);
```

The macro expands to nothing, so the arguments are never parsed. It breaks months later
when someone enables logging — in code nobody touched. Every format check that
`PRINTF_ATTR(format(printf))` sets up in `logging.h` is silently switched off across the
whole codebase in that configuration.

**Mechanism — already provided by D6.** The reason the `#else` block *has* to exist today
is that the tag triplets sit inside `#if DEBUG_LOGGING`: with logging off the tag symbols
do not exist, and an undefined `LOG_SYSTEM` inside a live `if()` is a compile error, not a
zero. D6 deletes `DEBUG_LOGGING` outright, so the triplets become unconditional and the
block has nothing left to protect against. S1 then reduces to "delete it", with `LOG_LEVEL
= LOG_LEVEL_QUIET` as the off switch — which is what made S3 load-bearing rather than
cosmetic, and why D1's ladder was reversed to make that setting behave as its name says.

**Caveat to verify, not assume.** An always-compiled macro *does* emit the call and its
string at `-O0`, where today's empty expansion emits nothing at any optimisation level.
That only bites in a `-O0` + logging-off build, an unusual combination — but check the
Debug configuration's actual `-O` setting before treating it as theoretical.

**Resolution:** _(pending)_

---

### S2 — Severity ↔ colour interaction

**Status:** 🔵 blocked on D1 · **Needs user:** no yet

**Question:** colour is currently per-class (`LOG_SYSTEM_COLOR`). Once severity exists,
should an error print red regardless of which class raised it?

**Leaning / recommendation:** dissolved by D1. Under (A) a class has exactly one severity
*and* one colour, so `LOG_<CLASS>_COLOR` already tracks severity by construction — pick
`LOGC_ERROR` for classes set to `LOG_LEVEL_ERROR` and so on, purely by convention in
`debug_config.h`. No macro change, nothing to decide.

**Resolution:** no action — the question only existed under options B and C.

---

### S3 — `0` must mean quiet at both ends of the comparison *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** with the original ascending-*severity* ladder and the predicate
`tag >= LOG_LEVEL`, the value `0` meant opposite things on the two sides. As a tag it
worked (`0 >= LOG_LEVEL` is false for any real global, so the class stayed quiet). As the
global it inverted: every tag satisfies `tag >= 0`, so `#define LOG_LEVEL
LOG_LEVEL_DISABLED` was arithmetically the **most permissive** setting the ladder could
express — silently the opposite of its name, and per S1 that is exactly the logging-off
build.

The cause was a category error, not an off-by-one: the constants are **verbosities**, but
the predicate was written as if they were **severities**, and the two orderings run in
opposite directions. Renaming the symbol (`LOG_LEVEL_NONE`, `LOG_LEVEL_QUIET`) would not
have touched it — the behaviour comes from the value `0` and the direction of the
comparison, not the name.

**Options considered:**

1. Keep the severity ordering and carry the special case in a shared predicate:
   `(LOG_LEVEL) != LOG_LEVEL_DISABLED && (tag) >= (LOG_LEVEL)`. Correct, but leaves the
   ladder reading backwards from what the numbers actually mean.
2. Document "never set the global to DISABLED". Zero code; leaves a legal-looking setting
   that does the reverse of its name.
3. Renumber `DISABLED` above `ALWAYS` (e.g. `256`). Removes the special case but forfeits
   `0`-means-off for tags — the property with the most value.

**Resolution:** **none of the above — D1's ladder was reversed instead.** With ascending
*verbosity* and `tag <= LOG_LEVEL`, `0` is naturally the most restrictive value at both
ends: nothing can be `<= 0` except `0` itself, and one guard on the tag covers that:

```c
#define LOG_EMIT(tag)   ( (tag) != LOG_LEVEL_QUIET && (tag) <= (LOG_LEVEL) )
```

| `LOG_LEVEL` | tag | result | |
|---|---|---|---|
| `DEBUG` (5) | `QUIET` (0) | guard fails | ✓ class silenced |
| `DEBUG` (5) | `ERROR` (2) | `2 <= 5` → emit | ✓ |
| `ERROR` (2) | `DEBUG` (5) | `5 <= 2` → no | ✓ chatty class hidden |
| `QUIET` (0) | `DEBUG` (5) | `5 <= 0` → no | ✓ global off |
| `QUIET` (0) | `ALWAYS` (1) | `1 <= 0` → no | ✓ master switch beats ALWAYS |

One guard in one place, instead of a special case at each end. The guard sits where it
reads naturally — *a class set to QUIET never emits, full stop* — and all operands stay
constant, so the whole predicate still folds to nothing.

**Rider, now answered arithmetically rather than by legislation:** `LOG_LEVEL_ALWAYS` does
**not** survive a logging-off build. `1 <= 0` is false, so a global of `QUIET` silences it
along with everything else. That is the intended reading — `ALWAYS` means *never filtered
out by verbosity tuning*, while the master switch outranks verbosity entirely, and
`RPRINTF()` remains the escape hatch for output that must survive a release build. State it
in the template so nobody reads "ALWAYS" as outranking the master switch.

Note the immediate migration does not exercise the quiet case: Skeleton and SwitchTester
run `LOG_LEVEL = LOG_LEVEL_DEBUG`, so a global of `QUIET` appears only in a logging-off
build.

---

### S4 — stdio-retarget contract differs across the three projects

**Status:** 🔵 deferred · **Needs user:** not yet

**Question:** raised by the user 2026-08-09. Skeleton and SwitchTester retarget stdio via
`stdio_retarget.c`; LED_Strip uses `__io_putchar()` / `__io_getchar()`. Should LED_Strip be
refactored to match?

**What the code actually shows.** The gap is much smaller than the framing suggests.
LED_Strip does **not** rely on Core's `_write` — `App/Src/syscalls_vfs.c` provides strong
`_write`/`_read` with fd routing, and stdout and stderr are already separate seams
(`__io_putchar` vs `__io_putchar_stderr`, the latter deliberately distinct "so it can be
re-pointed independently of stdout"). That is the same architecture as `stdio_retarget.c`,
plus a byte-level indirection and VFS routing for fd >= 3.

| Capability | Skeleton | LED_Strip |
|---|---|---|
| fd-routed `_write`, stderr separated | yes | yes |
| VFS fds >= 3 | no | yes |
| stdout mute + cursor-column tracker | yes | no |
| non-blocking `_read` | yes, returns `-1` when empty | no, `__io_getchar()` returns `0` |
| block writes | yes, buffer handed to uart_stream | byte-at-a-time loop |

**Trap to record before any code moves between these trees:** the `_read` semantics differ.
Skeleton returns `-1` on an empty ring; LED_Strip returns `0` bytes, and `fs_shell_hrn.c`
depends on that ("returns 0 when the RX ring is empty — same as a 0x00 data byte"). Code
that reads stdin will misbehave silently if ported without adjustment.

**Leaning / recommendation:** **defer — but not on difficulty.** Adding the mute to
LED_Strip's existing fd-1 branch is ~20-30 lines and needs no architectural change. The
reason to wait is that nothing in LED_Strip *consumes* it: the mute exists to protect a
SCRIPT frame stream, and LED_Strip has no automation console. Building it now is
speculative.

Better framing than "port `stdio_retarget.c` into LED_Strip": converge the **contract** —
fd routing, stderr always-through, mute semantics, `_read` return convention — and let each
project keep its back-end, since one has a filesystem and the others do not. That likely
makes stdio-retarget a fifth vendored API, but a thin one. Belongs in
`portable-apis-strategy.md` when T1 is written.

---

### I1 — Adopt the three-layer split

**Status:** 🟡 · **Needs user:** no (recommendation only)

**Question:** how is the module physically divided?

**Options considered:** LED_Strip already does this and it works:

| File | Role | Owner |
|---|---|---|
| `logging.h` / `logging.c` | engine — `log_color_t`, `v_log*()`, timestamp | vendored |
| `log_helpers.h` | macro sugar — `LOG`/`LOGC`/`LOGCT`/`_PLAIN`, `DPRINTF`, `RPRINTF` | vendored |
| `debug_config_template.h` | documented starter, copied to `App/Inc/debug_config.h` | vendored template |

**Leaning / recommendation:** adopt as-is. The key move is extracting the macros out of the
app-owned `debug_config.h` into the vendored `log_helpers.h`, so the macro definitions stop
being re-copied and drifting per project. `debug_config.h` shrinks to build guards + the
tag table + a trailing `#include "log_helpers.h"`. This is the same core/seam shape already
used by `automation_console.c` vs `automation_commands.c` and `uart_stream.c` vs
`uart_stream_target_g0b1.c`.

Note the include direction is already correct: `debug_config.h` includes the module
headers, not the reverse. The contract is an *include-order* one — `log_helpers.h` requires
the tag defines to precede it — which works but is implicit; document it at the top of the
header.

**Resolution:** _(pending)_

---

### I2 — Seam files live in the app's own directories *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** do the per-project seam files get their own directory, or go in `App/Inc` and
`App/Src` alongside app code?

**Options considered:** a dedicated `App/port/` directory makes "must I edit this?"
visible at a glance; putting them in `App/Inc`/`App/Src` matches established practice.

**Resolution:** **`App/Inc` and `App/Src`, edited in place.** User decision, this session:
this is their existing practice across past projects and matches how commercial and FOSS
libraries ship — FreeRTOS `FreeRTOSConfig.h`, lwIP `lwipopts.h`, FatFs `ffconf.h`,
TinyUSB `tusb_config.h`, mbedTLS `mbedtls_config.h`. The module ships a documented
template; the app copies it out of the module directory into its own include directory and
edits it there. No separate port directory.

Consequence to cover elsewhere: the "which files are seams?" visibility that a dedicated
directory would have given is instead carried by T2's seam inventory plus a header comment
in each copied template naming its origin — a convention LED_Strip's `debug_config.h`
already follows.

---

### I3 — `platform.h` as the single app-provided contract header

**Status:** 🟡 · **Needs user:** no (recommendation only)

**Question:** `logging.c` includes `stm32g0xx_hal.h` solely to call `HAL_GetTick()` in
`v_print_timestamp()`. That one line is the entire portability gap in the file — it is a
hand-edit on every adoption, and it is the *only* difference between Skeleton's copy and
LED_Strip's.

**Options considered:** give logging its own tiny time hook, or name an existing header as
the shared contract.

**Leaning / recommendation:** **superseded by D5 🟢.** `logging.h` declares
`extern uint32_t u32_log_timestamp_ms(void)`; the app defines it in `logging_port.c`,
copied from a template, returning `HAL_GetTick()`. `logging.c` then includes nothing but
the C library.

> This row originally proposed routing the timestamp through `platform.h`'s
> `SYSTEM_TICK()`, then through a shared `app_hooks.h`. Both were wrong in the same way —
> they had the module including an app-authored header. D5 records the correct shape and
> the reasoning; keeping this row for the audit trail only.

Worth recording: the weak-symbol mechanism behind `PUMP_POLLING_TASK()` and the weak UART
handles in `uart_stream_target_g0b1.c` are the same idea invented twice already — the
module declares what it needs, the app supplies it or doesn't, and an unresolved weak
symbol degrades to inert. Generalising that is the whole convention.

`platform.h` currently straddles all three tiers, which is why it feels hard to place:

- *Portable:* `STR`/`VSTR`, `PACKED`/`MAYBE_UNUSED`/`NEVER_RETURNS`, `ATOMIC_BLOCK_*`,
  `BM2N`, `US_IN_1S`/`MS_IN_1S`.
- *Contract:* `SYSTEM_TICK()`, `ELAPSED_TIME()`, `PUMP_POLLING_TASK()`, `KICK_WATCHDOG()`.
- *Board-specific:* `DEBUG_UART_HANDLE`, `PERIODIC_INT_TIMER_HANDLE`,
  `DELAY_US_TIMER_HANDLE`, the NVM sizes.

Splitting the portable third out is W2; not required for this work.

**Resolution:** _(pending)_

---

### I4 — Wrap all macros in `do { } while (0)`

**Status:** 🟡 · **Needs user:** no

**Question:** the macros currently expand to a bare `{ if (tag) { … } }`.

**Leaning / recommendation:** fix it. The current form breaks
`if (x) LOG(…); else …` — the `;` after the closing brace becomes an empty statement and
orphans the `else`. Worse, the *disabled* form expands to nothing at all, silently turning
the same line into `if (x); else …`, which compiles and misbehaves. `do { } while (0)` is
the standard fix and costs nothing. Free to do while the macros are being rewritten
anyway; S1 removes the empty-expansion half of the hazard independently.

**Resolution:** _(pending)_

---

### I5 — Make `ANSI.h` includes explicit

**Status:** 🟡 · **Needs user:** no

**Question, as first recorded:** `menusystem.c` uses `ANSI_*` macros but never includes
`ANSI.h`, relying on a transitive path through `logging.h`.

**Correction.** That was wrong — an artefact of a case-sensitive grep for `ANSI\.h`. The
includes were there all along, just spelled inconsistently:

| File | Spelling |
|---|---|
| `logging.h` (both projects) | `#include "ANSI.h"` |
| `menusystem.c` (Skeleton) | `#include "ansi.h"` |
| `debug_menu.c`, `term.h` (LED_Strip) | `#include "ansi.h"` |

The real defect is therefore not a missing include but a **case mismatch against the actual
filename**, which resolves on Windows and NTFS and fails outright on a case-sensitive
filesystem — Linux CI, WSL, a Docker build. That matters more for a module intended to
travel than the transitive-dependency concern did.

**Resolution:** normalised every spelling to `ANSI.h`, matching the file. Done in Skeleton;
`debug_menu.c` and `term.h` need the same when this reaches LED_Strip (W3). The header of
the reconciled `ANSI.h` states the rule so it does not drift back.

---

### I6 — Reconcile the diverged `ANSI.h` copies

**Status:** 🟡 · **Needs user:** no

**Question:** the two `ANSI.h` copies have drifted.

**Leaning / recommendation:** LED_Strip's copy is a strict superset — it adds DECSCUSR
cursor-shape macros (`ANSI_CURSOR_STYLE_*`) and the XTWINOPS text-area report
(`ANSI_REPORT_TEXT_AREA`) — except that `ANSI_CLEAR_AND_HOME` is composed in the opposite
order in the two trees. That ordering difference is functionally equivalent (ED does not
move the cursor), so the merge is near-free: take LED_Strip's content, land it in Skeleton
as canonical, note the ordering choice.

**Resolution:** _(pending)_

---

### I7 — `platform.h` housekeeping

**Status:** 🟡 · **Needs user:** no

**Question:** small defects found while reading the file.

**Leaning / recommendation:**

- Include guard is still `MACROS_H`, left over from an earlier filename. No collision
  today, but it is a trap for the next file that legitimately wants that name. Rename to
  `PLATFORM_H`.
- ~~The two LL includes are unused and should be removed.~~ **Withdrawn** (user, this
  session): `stm32g0xx_ll_gpio.h` / `stm32g0xx_ll_exti.h` are deliberate. `platform.h` is
  where GPIO set/clear, mode switching, open-drain and EXTI-mask macros go as an app grows
  past bare-bones; the includes are there ahead of that content. The observation that
  nothing uses them *today* was correct as a fact and wrong as a recommendation. They stay,
  and D5 exists so their presence does not leak into vendored modules.

**Resolution:** _(pending)_

---

### I8 — Duplicated NVM auto-commit delay

**Status:** 🟡 · **Needs user:** no

**Question:** the same 5-second delay is defined twice, in two files, in two units —
`NVM_AUTO_COMMIT_DELAY 500` (`platform.h`, 10 ms units) and
`DEV_CONFIG_NVM_COMMIT_DELAY_MS 5000` (`device_config.h`).

**Leaning / recommendation:** keep one. Guaranteed to drift otherwise, and the unit
mismatch makes the drift silent. Strictly out of scope for logging — captured here so it
is not lost; may belong on the nvmparams plan instead.

**Resolution (decided, not yet applied):** **keep `DEV_CONFIG_NVM_COMMIT_DELAY_MS` in
`device_config.h` and delete `NVM_AUTO_COMMIT_DELAY` from `platform.h`.** User decision,
this session. The surviving one is in milliseconds rather than 10 ms units and sits with
the other product options, so it reads correctly at the point of use. Whoever applies it
must check `v_nvm_commit_check()` in `app_main.c`, which consumes the 10 ms-unit form and
will need its arithmetic adjusted. Deferred to the nvmparams work.

---

### I9 — `PRINTF_ATTR` defined twice

**Status:** 🟡 · **Needs user:** no

**Question:** LED_Strip defines `PRINTF_ATTR` identically in both `logging.h` and
`log_helpers.h`.

**Leaning / recommendation:** define it once, in `logging.h`, and let `log_helpers.h` get
it via its existing `#include "logging.h"`. Identical redefinition is legal C so there is
no warning today — which is exactly why it will go unnoticed if one copy is ever edited.

**Resolution:** _(pending)_

---

### I10 — Call-site sweep scope and staging *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** how many call sites change, and does the sweep land in one commit or stage?

**Options considered:** under (B) or (C) every site would have picked a severity — 32 macro
invocations in Skeleton, more in SwitchTester and LED_Strip.

**Resolution:** **no sweep.** D1 chose (A), where the severity lives on the class constant
in `debug_config.h`, so every existing `LOG(LOG_SYSTEM, …)` compiles unchanged in all three
repos. The only edits are in the logging config header itself, and per D1 they are
mechanical: tags at `1` become `LOG_LEVEL_DEBUG`, `LOG_LEVEL` becomes `LOG_LEVEL_DEBUG`,
behaviour unchanged. Phases 1 and 2 touch no application source at all.

---

### I11 — Existing vendored modules already violate the dependency rule

**Status:** 🟡 · **Needs user:** yes (scope call)

**Question:** the include map shows the convention is not actually enforced today:

| Module file | Includes | Problem |
|---|---|---|
| `automation_console.c` | `device_config.h` | the app aggregator — drags `main.h`, `globals.h`, `debug_config.h` |
| `automation_console.h` | `device_config.h` | same, and it is the *public* header |
| `uart_stream.h` | `main.h` | CubeMX-generated app header, in a public vendored header |
| `queue.c` | `main.h` | same |

So both already-vendored modules reach into app-tier files. Nothing is broken — it builds
and runs — but "copy this directory into a new project" currently does not work without
also having a `device_config.h` shaped like this project's.

**Options considered:** per D5, the modules need two different things from the app and they
come from the two halves of the seam:

- **Bridge functions** — the polling pump; a timestamp for logging. The module declares
  `extern` prototypes in its own header; the app defines them in a port source copied from
  the module's template. `v_app_polling_task`'s declaration moves out of `platform.h` and
  into the header of each module that calls it (identical redeclaration across modules is
  legal, and the independence is the point).
- **Config** — `DEV_CONFIG_ENABLE_AUTOMATION_CONSOLE`, `ACON_LINE_MAX`,
  `DEV_CONFIG_CONSOLE_TX_BUF_SIZE` / `_RX_BUF_SIZE`. Per-module app-owned config header
  from a template: `automation_console_config.h`, `uart_stream_config.h` — the same
  `debug_config.h` / `ffconf.h` / `lwipopts.h` pattern this plan adopts for logging.

That generalises cleanly: **a vendored module includes the C library, other vendored
modules, and its own `<module>_config.h` — nothing else from the app. Everything else it
needs, it declares and the app defines.** Logging is then the first module built to the
finished convention rather than a special case.

**Leaning / recommendation:** fix logging to the convention now, since it is being
rewritten anyway. Retrofitting `uart-stream` and `automation-console` is mechanical but
touches two working, bench-verified modules and would want a reflash to confirm.

**Resolution:** 🔵 **deferred** — user decision, this session: revisit when it actually
blocks progress. Consequence to respect: **T1 must not claim the convention is universal**
while two vendored modules still reach into `device_config.h` and `main.h`. Describe it as
the target convention, with logging as the reference implementation and the other two
noted as not yet conforming.

---

### I12 — `ANSI_FG_RGB` / `ANSI_BG_RGB` cannot render

**Status:** 🟡 · **Needs user:** yes (trivial, but it is a behaviour change)

**Question:** the 24-bit colour forms are missing the trailing `m` that terminates an SGR
sequence:

```c
#define ANSI_FG_RGB(r,g,b)   CSI_S "38;2;" #r ";" #g ";" #b      // no "m"
#define ANSI_FG_RGB_FMT      CSI_S "38;2;%u;%u;%u"               // no "m"
```

Emitted as-is, a terminal sees an unterminated CSI and swallows whatever text follows until
it finds a final byte. The 256-colour forms directly above them (`ANSI_FG_FMT` and friends)
all have it, so this is an omission rather than a convention.

Pre-existing in **both** project copies and used **nowhere**, so nothing is broken today.
Not fixed during phase 1, which was scoped to no behaviour change — flagged in a comment at
the definitions instead.

**Leaning / recommendation:** add the `m` to all four macros. It cannot regress anything
that works, since nothing currently calls them and they could not have worked if it did.

**Resolution:** _(pending)_

---

### I13 — `DPRINTF_TS` called a function that does not exist *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** Skeleton's `debug_config.h` defined `DPRINTF_TS` to call
`v_log_printf_ts()`. No such function exists — the real one is `v_log_printf_time()`.

Any use of the macro in a `DEBUG` build would have failed to compile. It survived because
`DPRINTF` and `DPRINTF_TS` are used in **no source file in the tree**; the earlier count of
"3 uses" was matching the definitions and their comments, not call sites. LED_Strip's
`log_helpers.h` already had it right, so the two copies had silently diverged on a defect.

**Resolution:** the vendored `log_helpers.h` takes LED_Strip's correct form. Not a
behaviour change — code that cannot compile has no behaviour. Now exercised by the quick
test hook and confirmed working on the bench (`(11.312) DPRINTF_TS: ...`).

---

### T1 — Fold the tier model into the strategy doc

**Status:** 🟡 · **Needs user:** no

**Question:** the vendored / seam / app tier model and the refined dependency rule are
cross-cutting — they govern `nvmparams` and every future module, not just logging.

**Leaning / recommendation:** write a short section into
[`portable-apis-strategy.md`](portable-apis-strategy.md) once D3 and I3 lock. That doc is
already the source of truth for exactly this topic; this plan is the negotiation log, the
strategy doc is the contract.

**Resolution:** _(pending)_

---

### T2 — Seam inventory

**Status:** 🟡 · **Needs user:** no

**Question:** with seams living in `App/Inc`/`App/Src` (I2), nothing at a glance
distinguishes "seam you must edit on clone" from "app code you rewrite anyway".

**Leaning / recommendation:** a small table — in `README.md` or the strategy doc — mapping
each vendored module to its seam file and the template it came from. That is the checklist
a new project actually wants, and the clone-based workflow currently has no equivalent.
Pair it with the origin comment each copied template already carries.

**Resolution:** _(pending)_

---

### T3 — Port the decision-log model into this repo

**Status:** 🟡 · **Needs user:** yes (trivial)

**Question:** `Docs/planning/decision-log-model.md` does not exist in Skeleton — this plan
is the first D-log board here and currently references a model documented only in
`SwitchTester` and `LED_Strip_Controller_G474`.

**Leaning / recommendation:** copy SwitchTester's adapted version across (it is already
stripped of the PLAY / `.grok` / skill machinery) and adjust its repo-specific references.
Skeleton is the starter project, so the model should ship with it.

**Resolution:** _(pending)_

---

## Global notes

**Proposed implementation phases**, once enough rows are 🟢:

1. **Reorganise — ✅ DONE 2026-08-09, commit `59a46d1`, branch `feature/logging-api`.**
   Module directory (D2), three-layer split (I1), `ANSI.h` moved and reconciled (D3, I6),
   include case normalised (I5), timestamp port hook (I3/D5), `logging_config.h` rename +
   dead-flag deletions (D6), `DEBUG_MENU` folded and `debug_config.h` deleted (D7),
   `platform.h` guard (I7), `PRINTF_ATTR` de-duplicated (I9), `DPRINTF_TS` typo (I13).

   **Verification.** Clean build, 0 errors / 0 warnings. Size against the pre-change
   baseline built from `HEAD` in a throwaway worktree: `text 31528 → 31536` (**+8**),
   `data` and `bss` unchanged. The 8 bytes are exactly `logging_port.o`'s
   `u32_log_timestamp_ms`; `nm` and the map confirm the app's **strong** definition
   overrode the module's weak default (`T` at `0x08000aa0`, from `./App/Src/logging_port.o`).

   Bench-verified on the NUCLEO-G0B1RE (ST-Link `0671FF485251667187121242`, COM3 @ 921600)
   via the new logging test on debug-menu key `q`. All macro forms correct; tag colours
   resolve (`38;5;13` = `LOGC_BRIGHT_MAGENTA`); plain forms carry no timestamp or tag;
   `LOG_JOBS` at `0` emitted **nothing**, confirming the compile-time fold; and timestamps
   advance truthfully — `(11.314)` → `(11.564)` across a `v_delay_ms(250)`, proving the
   application bridge is live rather than the weak default's `0.000`.
2. **Levels — ✅ DONE 2026-08-09.** The D1 verbosity ladder (constants in `logging.h`),
   S3's `LOG_EMIT()` predicate, S1's removal of the empty-macro block, I4's `do{}while(0)`
   wrapper, I14's always-compiled `DPRINTF`. `DEBUG_LOGGING` deleted; the tag triplets are
   no longer wrapped in a conditional. Application source untouched apart from the test
   hook.

   **Verification — the fold is real.** Built twice, identically except for `LOG_LEVEL`:

   | `LOG_LEVEL` | text | log format strings present in the `.bin`? |
   |---|---|---|
   | `LOG_LEVEL_DEBUG` | 32920 | yes |
   | `LOG_LEVEL_QUIET` | 32044 | **no** — every one gone |

   876 bytes recovered, and grepping the image for `"LOGCT: tag color"`, `"LOG: no color"`
   and `"timestamp check"` returns **zero** matches in the QUIET build while control
   strings from plain `printf()` still appear twice. So the elimination removes the
   `.rodata` literals as well as the code — the specific thing flagged earlier as worth
   measuring rather than assuming. (`"SYSTEM"` survives once because `LOG_SYSTEM_TAG` is
   also consumed by the plain-`printf` ladder table in the test hook.)

   **Bench-verified** on key `q`: all macro forms behave as in phase 1 (no regression);
   the ladder table reports `LOG_LEVEL = 5`, `SYSTEM tier 5 emit=1`, `JOB tier 0 emit=0`,
   `EXTI tier 0 emit=0`; and all four ordering edge cases return what the design requires —
   QUIET class under a DEBUG ceiling `0`, ALWAYS class under a QUIET ceiling `0`, ERROR
   under WARNING `1`, DEBUG under WARNING `0`. The dangling-else case compiles and takes
   the `else`, confirming I4. Timestamps still advance truthfully (`15.467` → `15.718`).
   (and the matching move of the tag triplets out of `#if DEBUG_LOGGING`), I4's `do/while`
   wrapper. Application source is untouched; the edits land in `log_helpers.h` and
   `debug_config.h` only.
3. **Docs** — T1, T2, T3.

Keeping these as two commits gives a clean bisect point and keeps the level diff readable.

**Verification note:** D1's whole value is that disabled sites cost nothing, so confirm it
rather than assume it — compare `.text`/`.rodata` in the map file before and after with a
class set to `LOG_LEVEL_DISABLED`. Worth checking specifically that the **format-string
literals** of eliminated calls leave `.rodata`, which is the part that most often survives
dead-code elimination.

**Plan status summary:** 🟡 1 · 🟢 23 · 🔵 4 — 28 rows.
**No open questions remain on the board.** Every 🟡 is either implementation detail to be
carried out (I1, I4–I9), a follow-on scope call (I11), or documentation (T1–T3); S1 is a
recommendation with no dissent. Both phases are fully specified.
**Next action:** LED_Strip migration (build-verified only — no G474 target on the bench), then T1-T3 docs.

**End of logging-api-plan.md**
