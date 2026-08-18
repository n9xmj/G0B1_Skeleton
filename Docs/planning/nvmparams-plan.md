# nvmparams — vendorization and driver-interface plan

**Feature:** turn `nvmparams` into a vendored, hardware-independent module with
adopter-supplied non-volatile storage drivers.

**Code home:** `App/nvmparams/` (core, this repo) · adopter port files and drivers in each
adopting project · phase-1 development happens in **SwitchTester**, which has the most call
sites, the W25Q128 second backend, and the HIL suite.

**Parent spec:** [`portable-apis-strategy.md`](portable-apis-strategy.md) — tier model,
dependency rule, naming. Backlog origin: [`improvements-backlog.md`](improvements-backlog.md)
item 3.

**Status:** PLANNING

**Working mode:** decision-log model. One question at a time in chat, everything else lives
here as a row. The agent never silently resolves a 🔴 or 🟡 — leanings are recorded, locks
require the user's word. The user expects to revise and extend this list as work proceeds,
including off-topic and tangential additions; the Wish list is the landing zone for those.

---

## Brief

**What nvmparams is for, in the author's words (2026-08-17).** A **lean NVM storage system for
resource-constrained environments**, with dead-simple application semantics. Its architecture
was inspired by **Espressif's NVS API**, stripped down to run within the limits of low-end MCUs
— it found its first use on an STM32G030.

**It is explicitly not a filesystem substitute.** That is the reasoning behind the 16-bit caps
that this refactor deliberately leaves alone: 32-bit IDs would buy more namespace and a wider
size field would allow objects past 64 KB, but neither is realistic for the job. *"If you need
large objects, add a filesystem."* Recorded so that a later reader does not re-open
`nvm_param_id_t` or `nvm_object_t.u16_size` on general-purpose grounds — they are as they are on
purpose.

`nvmparams` is a ~10-year-old parameter store used across many of the user's projects. Its
core — a semi-linked-list pool of variable-length objects in a RAM buffer, committed to
non-volatile storage in one write — is solid and is not the subject of this work. What is
being changed is everything around it: the module currently contains its own STM32 flash
driver, its own `.nvmdata` linker-section buffer, and the application's parameter-ID enum,
all inside what is meant to become a vendored module with C-library-only dependencies.

**Phase 1** replaces the built-in device handling with adopter-supplied read/write drivers
reached through function pointers in an init config struct, moves all application-specific
declarations into an adopter-owned header, solves the reserved-ID problem, ships example
drivers for STM32 flash and a RAM array, adds a HIL test suite, and ports the result to
Skeleton and LED_Strip. **Phase 2** adds wear levelling, the CRC implementations, and a
full correctness review. Phase 1's job with respect to phase 2 is narrow but strict: leave
the on-media format and the internal seams such that phase 2 requires no refactor.

---

## The Big Board

| ID | Status | Subject |
|----|--------|---------|
| **D1** | 🟢 | Driver-facing struct (`nvm_media_t`): effective address, size, RAM pointer, context |
| **D2** | 🟢 | Address member type — `uintptr_t` |
| **D3** | 🟢 | Context pointer is non-`const` `void *` |
| **D4** | 🟢 | Two callbacks (read, write) rather than one with a direction flag |
| **D5** | 🟢 | Init takes a `const` config struct plus an uninitialised pool handle |
| **D6** | 🟢 | Zero-is-default for every config field except pool size, which has a hard minimum |
| **D7** | 🟢 | `nvm_param_id_t` becomes an explicit `uint16_t`, not a packed enum |
| **D8** | 🟢 | Adopter declares a plain anchored enum; reserved IDs enforced by checked public wrappers |
| **D9** | 🟢 | Reserved ID-space partition — core takes 0xFF00–0xFFFF only |
| **D10** | 🟢 | CRC supplied as a config function pointer; CRC-32; implementations are phase 2 |
| **D11** | 🟢 | The adopter header is a FreeRTOSConfig-style control panel; app includes ONE header |
| **D12** | 🟢 | `NVM_LABEL_MAX_LENGTH` exposed with a warning and an alignment assert; no signature guard |
| **S1** | 🟢 | Driver contract: no integrity checks, no wear maths, positive returns are device-specific |
| **S2** | 🟢 | Adopter-supplied device allocation unit; 0 means "= pool size", safe only without wear blocks |
| **S3** | 🟢 | Init policy on blank / corrupt / unreadable media, selected from the config struct |
| **S4** | 🟢 | `NVM_ERROR_NO_CHANGE` promoted from SwitchTester into the core |
| **S5** | 🟢 | `u32_write_count` becomes a monotonic sequence number in phase 1 |
| **I1** | 🟢 | Remove `nvm_mcu_flash[]` and the `.nvmdata` section dependency from the core |
| **I2** | 🟢 | Core reduced to C-library-only dependencies |
| **I3** | 🟢 | Example drivers ship as `*.c.example`, never compiled by any build system |
| **I4** | 🟢 | Adopter header split — exactly what leaves `nvmparams.h` |
| **I5** | 🟢 | "Find the live block" isolated as a function; phase-1 body returns block 0 |
| **I6** | 🟢 | Reserve wear-levelling config fields in phase 1 |
| **I7** | 🟢 | HIL/unit tests via the automation console; fault-injecting RAM driver |
| **I8** | 🟢 | Auto-commit delay is defined in two places — keep `device_config.h`, drop `platform.h` |
| **I9** | 🔴 | `x_mcuflash_write()` geometry bugs, carried into the example driver |
| **I10** | 🔴 | Pool-size and alignment constraints the core must validate at init |
| **I11** | 🟡 | Optional logging through an adopter-defined macro shim — no hard dependency |
| **I12** | 🟢 | Header include topology — config included from the middle of `nvmparams.h` |
| **I13** | 🟢 | `x_nvm_list()` leaves the core entirely and ships as an example with its own header |
| **I14** | 🟢 | Commit-timer accessors — the module manages the counter it already owns |
| **T1** | 🟢 | This plan lives in Skeleton |
| **T2** | 🔴 | Module README — adoption instructions |
| **T3** | 🔴 | Port to Skeleton, introduce to LED_Strip |
| **T4** | 🟡 | Provenance boundary for cherry-picked code — no confidential work code in a public repo |

### Wish list (phase 2+)

| ID | Status | Subject |
|----|--------|---------|
| **W1** | 🔵 | Wear levelling — the whole feature |
| **W2** | 🔵 | RTC backup-RAM driver (family-dependent; not viable on G0) |
| **W3** | 🔵 | Separate erase callback for devices where erase wants scheduling |
| **W4** | 🔵 | Program-page size distinct from erase-unit size |
| **W5** | 🔵 | Full correctness review of the core |
| **W6** | 🔵 | Parameter name-string table generated from the adopter's ID list |
| **W7** | 🔵 | Schema/format version object, so a foreign pool is detected and reformatted |
| **W8** | 🔵 | Two-tier commit window — opt-in max-deferral cap on top of the inactivity timer |

---
## Phase plan

Each phase ends at something demonstrable, not at a feature grouping.

| Phase | Status | What it proves | Contents |
|---|---|---|---|
| **1** | 🟡 | *The core runs on an adopter-supplied driver.* | Header split, config template, built-in drivers gutted, init rework, checked wrappers, commit-timer accessors, `x_nvm_list` extracted, STM flash + RAM examples, wear-levelling fields as no-op placeholders (I6), block-scan helper stub (I5), round-trip HIL smoke test, README foundation, Skeleton back-port |
| **2** | 🔵 | *The interface is right.* | MX25R80 cherry-pick and severance (T4), thin SPI glue example, full HIL suite with fault injection, bench validation on real SPI hardware. **Earliest honest mirror-adoption point** |
| **3** | 🔵 | *It is a product other projects can take.* | Wear levelling (W1), CRC implementations (D10), fileio example, LED_Strip introduction (T3), full README (T2), correctness review (W5) |

**Exit criteria.** Phase 1: SwitchTester boots, pool loads, parameters survive a reset, build
clean, size delta known, smoke test green. Phase 2: the same pool code runs on two genuinely
different backends with different erase geometries, and error paths are exercised rather than
assumed.

**The mirror does not need phase 3.** Work asked for SPI-flash NVM; that needs phases 1 and 2.
The endurance win is the move itself (~10 k cycles to ~100 k); wear levelling is a multiplier on
top, not a prerequisite. Decoupling the work deliverable from the hobby roadmap means the mirror
can migrate at the end of phase 2 if the schedule wants it.

**Risk carried at the 1→2 boundary.** The interface freezes in practice at the end of phase 1.
If the SPI work then reveals a flaw, fixing it costs phase 1's call sites — the exact risk the
W25Q128 was wired up to retire, now deferred by one phase. Bounded while only in-house projects
are on it; it stops being bounded when the mirror adopts, which is a further reason the mirror
should land *after* phase 2's validation rather than alongside it.

**Why the Skeleton back-port is in phase 1** despite that risk: it is a directory copy plus nine
call sites, so redoing it after an interface change costs very little — and per the standing
convention, phased back-porting is the only thing that actually tests a portability decision.
It takes the module, **not** the test suite (I7).

## Phase 1 implementation sequence

Work happens in **SwitchTester** (most call sites, the bench, the HIL net); the core goes up to
Skeleton at step 12 per T3. Steps 3–7 are one continuous breakage window — the header split and
the init rework cannot land separately — which is why step 7 is the first green build rather
than step 4.

| # | Status | Step | Ends with |
|---|---|---|---|
| 1 | 🟢 | `App/nvmparams/` created; `nvmparams_config.h.example` written (D11) | new files only |
| 2 | 🟢 | `nvmparams.h` restructured: Part A / config include / Part B (I12), `nvm_param_id_t` → `uint16_t` (D7), reserved-ID constants (D9), `nvm_media_t` + callback typedefs (D1, D10), `nvm_pool_config_t` (D5), new result codes (S3, D8) | header compiles standalone |
| 3 | 🟢 | App-owned `nvmparams_config.h`: anchored ID enum + `_Static_assert` (D8), knobs, log shim (I11) | — |
| 4 | 🟢 | `nvmparams.c`: delete `nvm_mcu_flash[]`, `x_mcuflash_write()`, the `NVM_DEVICE_*` switches, `SPIFLASH_NVM_DATA_ADDRESS`, the `u32_crc32()` stub, the `#if 0` FILE demo (I1, I2) | — |
| 5 | 🟢 | `x_nvm_pool_init()` reworked to the config struct, with blank/corrupt/unreadable classification and policy (S3), validation (I10, D6), block-scan stub (I5), format-by-block-index (W1) | — |
| 6 | 🟢 | Checked wrappers over `_unchecked` internals + `nvmparams_internal.h` (D8); commit-timer accessors (I14); `u32_write_count` monotonic (S5) | — |
| 7 | 🟢 | `x_nvm_list()` out to `nvm_list.c.example` + `nvm_list.h` (I13); call sites updated | **build green** |
| 8 | 🟢 | `nvm_driver_stm_flash.c.example`; SwitchTester config literal using `_nvm_start` (I1); `NbPages` and doubleword fixes (I9) | **bench: pool loads, commits, survives reset** |
| 9 | 🔴 | `nvm_driver_ram.c.example` (I3) | — |
| 10 | 🔴 | Round-trip HIL smoke test: create/set/commit/reset/get + `NVM_ERROR_NO_CHANGE` (I7) | **suite green** |
| 11 | 🔴 | README foundation (T2, partial) | — |
| 12 | 🔴 | Back-port the module — **not** the tests — to Skeleton (T3, partial) | **phase 1 complete** |

**Realistic scope note.** Steps 1–8 plus a green bench is a substantial but plausible sitting.
Steps 9–12 are each their own piece of work; the plan does not assume they all land together.

### Build status — steps 1–8 complete, 2026-08-17

**SwitchTester builds clean: 0 errors, 0 warnings.** Step 8's `bench:` criterion is NOT met —
nothing has been flashed or run. The pool has never been loaded on hardware.

**Three real faults surfaced at first compile**, all worth recording because two of them were
predicted by the design and then violated anyway:

- **`nvm_media_t` was unreachable from the config header.** I12 established that anything the
  config references must be defined *above* the include point, and the driver externs of I3
  reference `nvm_media_t` — which had been placed in Part B. Fixed by moving `nvm_media_t`, the
  three callback typedefs and `nvm_init_policy_t` into Part A, where they belong: none depends
  on a configuration value. **Only `nvm_header_t` genuinely requires Part B**, because it embeds
  `NVM_LABEL_MAX_LENGTH`. Worth stating as the rule for future additions.
- **`#if NVM_ENABLE_INTERNAL_MALLOC` was evaluated before the config header defined it**, so
  `<stdlib.h>` was silently dropped and `malloc`/`free` fell back to implicit declarations. Any
  test of a config symbol must sit *after* `#include "nvmparams.h"`.
- **A latent bug in the legacy flash driver** (adds to I9's list). `HAL_FLASHEx_Erase()` requires
  `Banks` to be set and its `Page` is **bank-relative**, 0..(`FLASH_PAGE_NB` - 1). The old
  `FLASH_PAGE()` macro produced an *absolute* index — 255 for a pool at `0x807F800` — and left
  `Banks` zero-initialised, which is out of range on a dual-bank G0B1. The replacement derives
  bank and bank-relative page from the address, guarded on `FLASH_BANK_2` so single- and
  dual-bank configurations of the same part both work with no `#define` to keep in sync.

**Also landed in step 8**, from I9: page count derived from the transfer size rather than
hardcoded `NbPages = 1`, and the source copied into a local `uint64_t` with a `0xFF` tail pad
instead of being cast through a masked pointer.

**Not yet verified:** anything requiring hardware. The ID renumbering means the existing pool
will not be recognised and will reformat to defaults on first boot — expected, and the debug
menu's `[N]` pool-erase command is the fallback if it does not clear cleanly.

---

## LOCKED CONTEXT

Established facts and decisions. Do not re-litigate unless explicitly reopened.

**Scope, phase 1** (user, this session): pool-struct reorganization and init rework;
removal of the hardcoded STM flash allocation and all other application dependencies;
removal of all built-in drivers; migration of adopter constants and the parameter-ID enum
out of `nvmparams.h`; solving the reserved-ID problem; deciding which IDs the core reserves;
STM flash and RAM example drivers; HIL tests in SwitchTester; port to Skeleton and LED_Strip.

**Scope, phase 2:** wear levelling; CRC implementations; full correctness review. The user is
confident in the core as it stands — it has proven itself across many applications — and does
not want phase 1 bogged down in review work. Known uncovered corner cases, such as pool size
exceeding device sector size, are acknowledged and are not an immediate concern.

**STATUS CHANGE 2026-08-17 — the mirror project is migrating nvmparams from STM32 internal
flash to SPI flash.** A work request, on the endurance grounds the user had previously raised:
STM32G0 flash is specified at ~10 k erase cycles where typical SPI NOR (MX25R, W25Q) is ~100 k
per sector, before wear levelling multiplies it further.

Consequences for this plan, none of which change a locked decision:

- **The SPI flash driver stops being a phase-1 stretch and becomes central.** The mirror's
  target backend is now SPI flash, so the pluggable driver layer is the thing it needs, and it
  needs it from **phase 1** — not after wear levelling as previously sequenced (D9, T3).
- **The "validate against a second real backend before the mirror adopts" deadline collapses
  into the adoption itself.** SPI flash *is* the mirror's case now.
- **MX25R80 matters more than W25Q128.** It is the part the mirror's legacy driver targets, so
  bench validation on an MX25R80 breakout is directly relevant rather than merely equivalent.
  The command sets are compatible for everything nvmparams needs, so either part validates the
  interface; only the mirror-specific confidence differs.
- **The linker-section machinery (T2, I1) becomes irrelevant to the mirror** — an SPI-flash pool
  needs a base offset, not a reserved `.ld` region. That simplifies its adoption considerably.
- **W7's foreign-pool hazard changes character rather than disappearing.** An SPI flash region
  survives a reflash exactly as the NOLOAD sector does, and it may additionally share the device
  with other data, so pool placement now wants coordinating with whatever else lives there.

**The user takes this as authorisation to lift what is needed from the mirror project**,
specifically the SPI flash support. This supersedes the earlier "do not open that repo during
planning" restriction for that purpose. See T4 for the boundary that still applies.

### Mirror project — known and accepted footguns (user, 2026-08-17)

**Recorded as accepted, not open. Minimally litigate these when the migration phase arrives.**

- **The mirror has no fallback and no fault detection around nvmparams today**, and it stores
  factory-originated, set-once data there — unit serial number, SKU and similar identity items.
  A pool failure is a bona fide failure case with no easy mitigation. Acceptable responses are
  to ignore it silently, or to light or blink the fault indicator.
- **The product degrades usefully but not completely.** It can run standalone on RAM defaults;
  whether it can still go online or be WiFi-provisioned is not established.
- **Division of secrets:** AWS certificates and keys, and some WiFi parameters, live in the
  ESP32-C6's own NVS store. Only the identity items are in STM32 flash. So an STM-side pool
  failure does not by itself destroy the credentials.
- **Much of this is moot at migration time.** The move is STM flash → SPI flash, so the
  pre-production pilot units require reprovisioning regardless. A bootloader change is also
  under discussion — reclaiming wasted space to enlarge the app slots — which would force a
  full-chip erase and wired reflash.

**Two consequences worth noting now, both additive:**

**`NVM_INIT_REQUIRE_VALID` is the WRONG policy for the mirror** — correcting an earlier note in
this plan that suggested otherwise.

The factory does not flash a provisioned pool onto the part. A mostly-automated line tool
connects to the unit **over BLE at first boot** and writes the identity parameters, and it
assumes the STM and ESP pools are already **live and operational**. So a virgin unit must come
up with a working, empty pool that is ready to receive provisioning.

`REQUIRE_VALID` aborts on blank media, which is exactly the first-boot state. Using it would
leave the pool uninitialised, and the BLE link the tool depends on would be talking to a product
that had refused to start. The policy assumes provisioning happened **out of band** — programmed
into flash by the programmer at manufacture — which is not this flow. Both mirror pools want
`FORMAT_IF_BLANK`.

**What `REQUIRE_VALID` cannot do, an application-level marker can.** The hard case is telling
"virgin, awaiting provisioning" from "provisioned once, and the data has since gone" — they want
opposite responses, and the pool alone cannot distinguish them. A **provisioned marker**
parameter solves it: no marker means virgin, so come up quietly and let the tool do its work;
marker present but identity missing or defaulted means a genuine fault, so light the indicator.

That is an ordinary parameter, not a module feature, and it costs nothing. It also benefits from
commit atomicity: because a commit writes the whole pool in one operation, the marker and the
identity data land together or not at all, so there is no half-provisioned state for the tool to
have to detect or recover from.

**Identity and settings want different policies, and multiple pools are now cheap.** Set-once
identity data wants `REQUIRE_VALID`; mutable operating settings want defaults on blank media.
One pool forces a compromise; two do not. Explicit per-pool base addresses and adopter-supplied
drivers are what make that practical — it is the multi-pool dividend already noted under S3,
arriving where there is a real need for it.

**The full-chip erase is a migration WINDOW, and it relaxes constraints rather than adding
them.** Much of this plan protects on-media compatibility — phase 1's format being phase 2's
(S5, D10), the CRC-enablement transition invalidating existing pools (D10), the label-length
hazard (D12). For the mirror specifically, none of that binds during a migration that erases the
part anyway. The CRC can be enabled from day one there, and the label length and wear-block
count can be chosen freely rather than inherited. Worth planning the migration to *use* that
window, because it does not come round again.

**Keep the legacy function and object names** (user, standing directive) unless the user
explicitly asks for a change, or there is a genuine namespace conflict. This governs the
implementation phase, not just the design: `x_nvm_pool_init`, `x_mcuflash_read`/`_write`,
`NVM_DATA_SIGNATURE`, `u16_commit_timer`, `nvm_param_id_t` and the `NVM_ERROR_*` names all
stay as they are. New members and functions with no legacy name — `u32_alloc_unit`,
`nvm_media_t`, the `_unchecked` internals — are free to be named on merit.

**Wear levelling is deliberately unplanned in detail.** Enough design only to avoid a later
refactor. Rotation policy, block-count defaults, commit-versus-threshold rotation and
torn-write recovery are explicitly *not* being decided now.

**The core computes effective addresses; drivers are blind to wear levelling** (user,
confirmed). A driver never sees a block count or block index. It receives one address and
moves bytes. This also falls out of block discovery, which must call read repeatedly at
different addresses to find the live block.

**Drivers do no integrity checking.** They validate their parameters and report physical
device access errors. Deciding whether data read is valid, or whether data being written is
intact, is the core's responsibility.

**The address member is device-relative, not a pointer.** For MCU flash it is an address;
for a file-backed driver it is the `lseek`/`fseek` offset; for SPI flash a byte offset.

**Native block size is an init parameter and a pool member, with 0 as the don't-care value**
(user, confirmed).

**Backward compatibility is a goal, with a named check target:**
`ee_fw-ST3074-8-inch-Round-mirror-wifi-bt`, an STM32G0B0 work project with existing NVM
concerns. **That repo is not to be opened during planning** (user, explicit) — it is named so
call-site churn stays in view. Practical consequence: the accessors (`x_nvm_get`, `_set`,
`_create`, `_commit`, `_delete`, `_list`) stay recognisable even as init changes shape. Init
is called once per pool; the accessors are called everywhere.

**Current state, measured:** `App/Src/nvmparams.c` + `App/Inc/nvmparams.h`, not yet a module
directory. Present in Skeleton and SwitchTester and **byte-equivalent** between them once
line-ending noise is discounted (7 and 23 whitespace-insensitive lines). **Absent from
LED_Strip** — that is an introduction, not a port. Call sites outside the module in
SwitchTester: `app_main.c` 26, `switch_out.c` 16, `debug_menu.c` 6,
`automation_commands.c` 2.

**The G0B1 has five TAMP backup registers — 20 bytes total** (verified against
`stm32g0b1xx.h` this session). `nvm_header_t` alone is 28 bytes, so RTC backup RAM cannot
hold even an empty pool's header on this part. Other families (F4/F7/H7, 4 KB backup SRAM)
could support it. This is why W2 is a wish and not the degenerate example.

**A driver interface already exists in embryo.** `x_nvm_read()`/`x_nvm_write()` are already
the device hooks and the header already calls `x_mcuflash_read`/`_write` "device drivers";
`NVM_DEVICE_MCUFLASH`/`_FILE`/`_NONE`/`_MAXVAL` are an early sketch of the same idea. The
refactor formalises what is there. A stray `#define SPIFLASH_NVM_DATA_ADDRESS 0x0400` in the
`.c` is evidence of a second backend half-started in place.

**A NOLOAD `.nvmdata` sector survives reflashing** — a foreign project's pool can shadow your
IDs, with a valid signature and self-consistent contents. Not an nvmparams fault; CRC would
not have caught it, because the pool was intact-but-foreign rather than corrupt. This is the
origin of W7.

**`u32_crc32()` is currently stubbed** to return `0xDEADC0DE`; validation today is
signature-only.

---

## Detail sections

### D1 — Driver-facing struct *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** What does a driver receive?

**Options considered:** the `nvm_pool_t` handle itself; a dedicated small struct; a flat
parameter list.

**Leaning:** a dedicated struct, held as a member of the pool so the core stamps the address
into it rather than assembling one per call.

```c
typedef struct
{
    uintptr_t    ux_address;    // effective device address, core-computed
    uint32_t     u32_size;      // bytes to transfer
    void        *p_v_data;      // RAM pool: source for write, destination for read
    void        *p_v_context;   // driver's own; e.g. char* path, SPI handle
}
nvm_media_t;

typedef nvm_error_t (*pfn_nvm_read_t) (const nvm_media_t *p_x_media);
typedef nvm_error_t (*pfn_nvm_write_t)(const nvm_media_t *p_x_media);
```

The pool handle was rejected for two reasons: it carries core-private state
(`u8_need_commit`, `u8_internal_malloc`, `u16_commit_timer`, and future wear-level
bookkeeping) that adopter code should not see or touch, and the effective address is a
per-call value that is not a pool field at all. Four members is also simply less for a driver
author to misread, which serves the stated goal of making driver creation as easy as possible.

**Resolution (user, locked):** the struct members and the function prototypes as shown above.
This locks D2, D3 and D4 with it — the address type, the non-`const` context pointer, and the
two-callback shape are all part of what was accepted.

### D2 — Address member type *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** `void *`, `uint32_t`, or `uintptr_t`?

**Leaning:** `uintptr_t`. It holds both a pointer and a bare offset losslessly on any target.
This matters concretely because two of the intended example drivers — file-backed and
RAM-emulated — are the ones most likely to be compiled on a host for testing, where `void *`
is eight bytes and `uint32_t` silently truncates. For MCU flash it casts cleanly back to a
pointer; for a file driver it is the `lseek` offset; for SPI flash a byte offset.

**Resolution (user, locked): `uintptr_t`.**

**What it is, since it is unfamiliar** — this belongs in T2's README as well as here.
`uintptr_t` is **a plain unsigned integer typedef from `<stdint.h>`**. It is not a union, not a
struct, and not a pointer type; there are no members to reference. Its single guarantee is that
any `void *` converted to it and back compares equal to the original. **On every target in
these projects it is literally `uint32_t`** — 32-bit ARM — so it is not a new representation,
only a more honest name. It widens to 64 bits only in a host build, which is the case that
motivated it.

Using it is always a cast to whatever the target API wants:

```c
/* as a plain number - no cast needed, it is already an unsigned integer */
uint32_t u32_offset = (uint32_t) p_x_media->ux_address;

/* as an STM32 flash address - HAL_FLASH_Program takes a uint32_t address */
HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                  (uint32_t) p_x_media->ux_address, u64_data);

/* as a pointer into the STM32 address space, e.g. for memcpy on read */
const void *p_v_src = (const void *) p_x_media->ux_address;

/* as a SPI flash byte offset - just a number */
x_spiflash_read((uint32_t) p_x_media->ux_address, p_u8_buf, u32_len);

/* as a file offset */
fseek(p_x_file, (long) p_x_media->ux_address, SEEK_SET);
```

One printing gotcha for the examples: `%lu` is not portable for `uintptr_t`. Either cast —
`printf("%08lX", (unsigned long) ux_address)` — or use `PRIuPTR`/`PRIXPTR` from
`<inttypes.h>`. The cast matches what the existing code already does for `uint32_t` and is the
simpler choice.

**If the unfamiliarity outweighs the host-build benefit, `uint32_t` loses nothing on any
current target** and the row can be reopened cheaply — the only cost is a silent truncation in
a hypothetical 64-bit host build of the RAM or file example.

### D3 — Context pointer *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** `const void *` or `void *`?

**Leaning:** non-`const` `void *`. The core never dereferences it, and a driver may want it
mutable — a cached `FILE *`, a lazily-opened SPI handle, a retry counter. A driver wanting
immutability can point it at its own `const` object. `const` here only removes options.

**Resolution (user, locked):** non-`const` `void *`, as part of D1.

### D4 — Two callbacks or one *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Leaning:** two — `pfn_read` and `pfn_write`. The write path usually needs an erase step the
read path does not; merging them puts a direction `if` at the top of every driver for no gain.
Matches the existing `x_nvm_read`/`x_nvm_write` shape, so the core's call sites barely move.
A separate erase callback is deliberately **not** in phase 1 (see W3) — the driver contract
stays "write these bytes, whatever it takes."

**Resolution (user, locked):** two callbacks, as part of D1.

### D5 — Init config struct, and how the pool holds it *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** Does `x_nvm_pool_init` take the pool handle pre-populated by the adopter, or a
separate config struct? If separate, does the pool copy it or point at it?

**Options considered:** (i) pool handle doubles as the settings struct; (ii) separate config,
pool stores a `const` pointer to it; (iii) separate config, pool copies the fields it needs.

**Leaning:** (iii). A separate config lets the adopter declare it `const` at file scope so it
lives in flash rather than RAM; it keeps init's signature stable when core-private pool fields
change, which they will when wear levelling lands; and it reads correctly, the config being a
description and the pool a runtime object. Copying rather than pointing costs ~20 bytes per
pool but removes the dangling-pointer trap when an adopter builds the config as a stack local
inside their own init function — which is exactly what a first-time adopter does. Pools number
one or two; safety is worth the bytes.

Sketch, subject to D8/D9/D10:

```c
typedef struct
{
    const char       *p_c_label;
    pfn_nvm_read_t    pfn_read;
    pfn_nvm_write_t   pfn_write;
    pfn_nvm_crc_t     pfn_crc;          // NULL = signature-only (D10)
    uintptr_t         ux_base_address;
    uint32_t          u32_size;         // pool size, bytes
    uint32_t          u32_alloc_unit;   // device allocation unit; 0 = "= pool size" (S2)
    void             *p_v_ram_buffer;   // NULL = allocate internally
    void             *p_v_context;
    uint8_t           u8_wear_blocks;   // 0 or 1 = no wear levelling (I6)
    nvm_init_policy_t x_init_policy;    // 0 = NVM_INIT_FORMAT_IF_BLANK (S3)
}
nvm_pool_config_t;
```

**Resolution (user, locked): a separate application-facing config struct, taken by `const`
pointer.** The caller must be able to assemble it as a ROM constant, and **nvmparams must never
write to it.** Signature:

```c
nvm_error_t x_nvm_pool_init(nvm_pool_t *p_x_pool, const nvm_pool_config_t *p_x_config);
```

**Consequence the user noted:** init takes both the config *and* a pointer to a (presumably
uninitialised) `nvm_pool_t`. That is still a reduction — two arguments where there are four
positional ones today — and the accessors are untouched, which is what the compatibility goal
actually cares about.

**Sub-question this settles by implication: the pool copies, and does not retain the pointer.**
D1 already forces most of it. The pool holds an `nvm_media_t` whose `ux_address` the core
**stamps the effective address into before every driver call**, so base address, size, RAM
pointer and context must live in mutable pool storage regardless — they cannot be read through
a pointer into ROM. Given that, copying the remaining handful of fields costs a few bytes and
buys three things:

- the config genuinely can be `const` in flash, read once at init and never referenced again;
- it can *also* be a stack temporary safely, so there is no static-storage-duration rule for an
  adopter to get wrong — which serves D11 directly;
- one fewer indirection on the hot path, since driver calls read pool RAM rather than chasing a
  pointer into flash.

Nothing in the pool retains `p_x_config` after init returns.

**Naming:** keep `x_nvm_pool_init` rather than shortening to `x_nvm_init`. The existing name
already carries the distinction, and multiple pools per project is now a highlighted capability
(S3) rather than an afterthought.

### D6 — Zero-is-default *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Leaning:** make it a hard rule that a zeroed config field means the sane default:
`p_v_ram_buffer = NULL` → allocate internally (already true today), `u8_wear_blocks = 0` → no
wear levelling, `u32_alloc_unit = 0` → "= pool size" (S2), `p_v_context = NULL` → unused,
`pfn_crc = NULL` → signature-only (D10), `pfn_read`/`pfn_write = NULL` → the null device (I4),
`x_init_policy = 0` → `NVM_INIT_FORMAT_IF_BLANK` (S3). Adopters then use designated
initializers and write only the fields they care about, and adding a field later cannot break
an existing config. This is what makes the config struct cheaper to adopt than the positional
argument list it replaces — and D5's copy-not-retain means a zeroed stack local is a valid
config too.

**Resolution (user, locked): zero-is-default for every member EXCEPT `u32_size`, which must be
set non-zero and is range-checked at init.**

**The minimum is derived, not a magic number.** It must hold the pool header, an end-of-list
record, and enough data space for two 32-bit objects with their per-object overhead:

```c
#define NVM_POOL_SIZE_MIN   ( sizeof(nvm_header_t)                 /* pool header        */  \
                            + ((sizeof(nvm_object_t) + 4u) * 2u)   /* two u32 objects    */  \
                            + sizeof(nvm_object_t) )               /* end-of-list record */
```

With the default 16-byte label that is **28 + 16 + 4 = 48 bytes**. Writing it as an expression
rather than a constant means it tracks `NVM_LABEL_MAX_LENGTH` automatically — an adopter who
raises the label to 32 (D12) gets a minimum of 64 without touching anything, which is the
derive-from-the-real-thing rule this codebase already applies elsewhere.

**Two checks fall out, one runtime and one compile-time:**

- Init rejects `u32_size < NVM_POOL_SIZE_MIN` with `NVM_ERROR_PARAMETER` (I10). It should also
  require a multiple of 4, since objects are 4-byte-aligned by `ROUNDUP4`. A driver needing
  coarser granularity — the STM32 doubleword program — handles its own tail rather than
  imposing a flash constraint on RAM and file backends (I9).
- `NVM_POOL_SIZE_DEFAULT` in the adopter header gets a `_Static_assert` against
  `NVM_POOL_SIZE_MIN`, so a badly chosen default fails at compile time rather than at first
  boot.

**Why size is the exception:** every other zero-default reads as "this feature is off", which
is unambiguous. A silently-defaulted pool size is a real geometry decision made on the
adopter's behalf, and if the default later changes — or the adopter assumed a different one —
the mismatch surfaces as a pool that reformats rather than as an error.

### D7 — `nvm_param_id_t` becomes an explicit `uint16_t` *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** Should the ID type stay an enum once the enum is split between module and
adopter?

**Leaning:** no — `typedef uint16_t nvm_param_id_t`, with the core's reserved IDs declared as
constants. This is a correctness fix, not a convenience. `nvm_object_t` is the **on-media
record layout** and its first member is the ID. That member is two bytes today only because
`PACKED` sizes the enum by its largest value, which happens to be
`NVM_PARAM_END_OF_DATA = 0xFFFF`. Split the enum so the adopter's half tops out at 0x10C and a
packed enum for it is **one byte** — silently changing the record layout and invalidating
every stored pool. The same hazard exists for any downstream project building without
`-fshort-enums`. An explicit `uint16_t` removes a portability landmine that is live right now
for the droppable-into-other-projects goal.

C enums provide no cross-TU type safety anyway, so nothing real is lost. The adopter is then
free to declare their IDs as any enum they like; enum constants convert implicitly at the call
sites, so no cast is ever needed.

**Resolution (user, locked):** `typedef uint16_t nvm_param_id_t`. The accessor functions take
`uint16_t`. The adopter declares an ordinary enum and passes its labels directly.

**Consequential note for T2:** a *variable* holding an ID should be typed `nvm_param_id_t`,
not the adopter's enum type. Passing an enum constant at a call site is fine and is the normal
usage; storing one in an enum-typed variable reintroduces the width question D7 exists to
remove.

### D8 — How the adopter declares parameter IDs *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** The adopter owns their parameter IDs, the core reserves a range, and the
reservation should be statically enforced. What declaration idiom achieves that?

**Framing:** the "enum merging" problem dissolves once D7 lands. There is no need to merge
anything — two independently-declared sets of `uint16_t` constants interoperate perfectly.
What remains is purely the enforcement question.

**Options considered:**

**(a) Anchored plain enum.** The adopter writes an ordinary enum whose first member is
`= NVM_ID_APP_FIRST` (a core constant) and whose last is a `NVM_PARAM_APP_LAST` marker; the
core emits `_Static_assert(NVM_PARAM_APP_LAST <= NVM_ID_APP_MAX, ...)`. Familiar syntax, no
machinery, auto-increment and contiguity preserved. **Hole:** an explicitly-assigned outlier
escapes entirely — and that is not hypothetical, it is `NVM_PARAM_TEST_1 = 0xFFF0` in the
current header.

**(b) X-macro list.** The adopter writes a list macro; the module header expands it twice,
once to build the enum anchored at the core's base and once to emit one `_Static_assert` per
ID.

```c
/* adopter's nvmparams_config.h */
#define NVM_APP_PARAM_LIST(X)       \
    X(NVM_PARAM_SWITCH_PULSE_MS)    \
    X(NVM_PARAM_CYCLE_A_REPEAT)     \
    X(NVM_PARAM_CYCLE_A_ON_US)      \
    /* ... */

/* module header */
#define NVM_X_ENUM(name)    name,
enum { NVM_PARAM_APP_ANCHOR = NVM_ID_APP_FIRST - 1, NVM_APP_PARAM_LIST(NVM_X_ENUM) };

#define NVM_X_ASSERT(name)  _Static_assert((name) >= NVM_ID_APP_FIRST &&    \
                                           (name) <= NVM_ID_APP_MAX,        \
                                           #name " is outside the application ID range");
NVM_APP_PARAM_LIST(NVM_X_ASSERT)
```

Auto-increment survives, contiguity survives — declaration order is numeric order, so
SwitchTester's `NVM_PARAM_CYCLE_A_REPEAT + (channel * COUNT) + parameter` still holds and its
`_Static_assert` in `switch_out.c` stays valid — and the check is per-ID and airtight. Cost is
an unfamiliar declaration idiom. **Bonus:** the same list generates a name-string table (W6),
so `x_nvm_list()` and the automation console can print `NVM_PARAM_CYCLE_A_REPEAT` rather than
`0x101`. With HIL tests on the phase-1 list, that has real value.

**(c) Declared window.** The adopter defines `NVMPARAMS_APP_ID_FIRST`/`_LAST` and the core
asserts the window lies inside the app range. Cheapest and weakest — it validates the
declaration, not the IDs.

**(d) Checked public accessors over unchecked internal ones** (user, this session). Rename
the existing worker functions to unchecked internals and make `x_nvm_set`, `x_nvm_create` and
`x_nvm_delete` thin public wrappers that range-check the ID first. `x_nvm_get` stays
unwrapped, so the application retains read visibility into core-owned objects.

**Why (d) beats (b), which was the agent's earlier leaning:**

- **It catches computed IDs, which no static assert can.** A `_Static_assert` only sees IDs
  the adopter *declared*. SwitchTester computes them —
  `NVM_PARAM_CYCLE_A_REPEAT + (channel * COUNT) + parameter` — and an out-of-range `channel`
  lands wherever it lands. The wrapper checks every actual access; the static check cannot.
- **Zero adopter churn.** Public signatures stay byte-identical, so every existing call site
  compiles unchanged. The X-macro would have forced every adopting project to rewrite its
  parameter enum as a macro list, including the named compat target — precisely the sweeping
  call-site churn this work is trying to avoid.
- **A plain enum is friendlier to read and edit** (user). The X-macro idiom is the first thing
  an adopter has to write, and it is ugly.

**Resolution (user, locked): (d) plus (a). Three parts.**

**1. The adopter writes an ordinary anchored enum**, with a single static assert:

```c
typedef enum
{
    NVM_PARAM_SWITCH_PULSE_MS = NVM_ID_APP_FIRST,   // core-supplied base
    NVM_PARAM_CYCLE_A_REPEAT,
    /* ... */
    NVM_PARAM_TEST_3,
    NVM_PARAM_APP_LAST                              // marker, not a parameter
}
app_nvm_param_t;

_Static_assert(NVM_PARAM_APP_LAST <= NVM_ID_APP_MAX, "too many application NVM parameter IDs");
```

Plain enum syntax, auto-increment preserved, contiguity preserved. Catches "anchored wrong"
and "ran off the end" at compile time. Does not catch a deliberate mid-list `= 0xFF01`, which
part 2 handles at runtime.

**2. Checked public wrappers.** `x_nvm_set`, `x_nvm_create`, `x_nvm_delete` range-check the ID
and reject a reserved one; `x_nvm_get`, `x_nvm_get_size` and `x_nvm_list` stay unwrapped so the
application can read core-owned objects (useful for a debug-menu pool display). **The
asymmetry must be documented in T2** or an adopter will wonder why `get` works on an ID that
`set` rejects.

**3. The unchecked internals are not declared in `nvmparams.h`, but are not `static` either**
(user). Test code — HIL and HuIL — can `extern` them when it genuinely needs to reach past the
guard. Naming: `x_nvm_set_unchecked` rather than `_private`, since "private" is false for a
non-static symbol whereas "unchecked" describes what it is and warns the reader.

**Agent recommendation on top of part 3:** ship a `nvmparams_internal.h` alongside the module
that declares those prototypes, rather than leaving test code to hand-roll `extern`
declarations. `nvmparams.h` does not include it; reaching the internals stays a deliberate act
of including a second header. The reason is that a hand-written `extern` whose signature
drifts from the definition is a linkage bug the compiler cannot catch — the call passes the
wrong arguments and the failure is silent. One shipped header keeps a single source of truth
for the prototypes while preserving exactly the accessibility the user asked for.

**Rejection is loud.** The wrapper emits a log on rejection (I11) and returns a distinct error
code, `NVM_ERROR_ID_RESERVED`, rather than folding into `NVM_ERROR_PARAMETER`. The reason is
that runtime enforcement otherwise fails *quietly*: `x_nvm_create` returning an error that
nobody checks — and most `create` calls do not check, since `NVM_ERROR_OBJECT_EXISTS` is a
normal non-error return — means the parameter silently never exists and its default never
appears. That is the same shape as the foreign-pool incident: correct behaviour, invisible
cause. A distinct code also gives I7's reserved-ID test something specific to assert on.

### D9 — Reserved ID-space partition *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** How is the 16-bit ID space divided between core and application?

**Finding that shrinks the problem:** the wear-levelling write count lives in `nvm_header_t`
(it is already there as `u32_write_count`), not as a pool object, so it needs **no reserved
ID at all**. The standing brief assumed write-count IDs would be needed; they are not.

**Leaning:** the core reserves **only the top page, 0xFF00–0xFFFF**; the application owns
0x0001–0xFEFF; `0x0000` stays `NVM_PARAM_UNUSED`. `NVM_PARAM_END_OF_DATA = 0xFFFF` is already
in the top page, and 0xFFFF being the erased-flash value is deliberate and worth preserving.
256 reserved IDs is ample headroom for W7 and anything phase 2 wants.

**Migration this forces**, both small and both useful as validation:
- SwitchTester's `NVM_PARAM_TEST_1..3` at 0xFFF0–0xFFF2 move down into app space.
- The `NVM_CONFIG_*` factory IDs at 1–3 stop being notionally "reserved for CONFIG" (per the
  current header's `NVM_PARAM_BASE_ID = 0xFF` comment) and become ordinary application IDs.
  The current low-range reservation is application convention, not core policy, and should be
  documented as such rather than enforced.

**Resolution (user, locked): 0xFF00–0xFFFF is the nvmparams system-reserved block.**

**Accepted with a noted regret** (user): 0xFF is the erased-flash state on STM32, so reserving
the *low* page would have been the tidier choice. It cannot be — `NVM_PARAM_END_OF_DATA` must
stay 0xFFFF for the end sentinel to coincide with erased flash, and the module has to drop into
the mirror project, which constrains the layout.

**Known unharmonised: the mirror project may already allocate IDs in 0xFFxx** — possibly HIL
scratch/test values; not confirmed, and that repo is deliberately not being opened during
planning. Harmonisation is deferred to the mirror migration, which by the user's own sequencing
comes after wear levelling.

**The collision surfaces loudly rather than silently, which is D8 paying off.** If the mirror
does hold IDs in the reserved page, adopting this module makes `x_nvm_create` reject them with
`NVM_ERROR_ID_RESERVED` and a log line naming each one. That is exactly the quiet-failure
scenario the distinct code and the mandatory log were added for — a parameter that silently
never exists and whose default never appears. Concrete migration recipe for T3: build, boot,
read the log, and the rejections enumerate the collisions for you.

**Escape hatch, recorded now rather than invented under pressure later.** The user's stated
fallback was to "remove or relax the ID guards in the mirror's copy of nvmparams" — but that
means forking a vendored module, which is the failure the vendoring model exists to prevent.
Cheaper: make the boundary itself an overridable default.

```c
/* nvmparams.h */
#ifndef NVM_ID_APP_MAX
  #define NVM_ID_APP_MAX    0xFEFFu       /* core reserves 0xFF00-0xFFFF by default */
#endif
_Static_assert(NVM_ID_APP_MAX <= 0xFFFEu,
               "NVM_ID_APP_MAX must leave 0xFFFF for NVM_PARAM_END_OF_DATA");
```

An adopter with legacy IDs high in the space raises `NVM_ID_APP_MAX` from their config header
and edits no vendored file. In phase 1 the core genuinely needs only `0x0000` and `0xFFFF`, so
the reserved page is headroom rather than necessity and reclaiming it is safe today — with the
documented caveat that doing so **forfeits room for future core features** such as W7's schema
object, and an adopter who reclaims it may face a second harmonisation later.

### D10 — CRC as a config function pointer *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** The user proposed a swappable CRC submodule with a compile-time build option
selecting HW or SW generation.

**Leaning:** the same outcome falls out of the pattern already adopted for storage, with less
machinery. Make the CRC function a pointer in the config struct: the adopter points it at the
HAL peripheral, at a software routine shipped alongside the example drivers, or leaves it
`NULL` for signature-only validation. No build option, no separate module, no compile-time
switch — and `NULL` meaning "off" preserves D6. Cost is one indirect call per commit, which is
irrelevant at commit frequency.

**Phase split:** the *plumbing* — a struct member and one call site — is nearly free and
should land in **phase 1**, so that no driver written against phase 1 needs revisiting. The HW
and SW *implementations* can land in phase 2 with no format change, since `u32_crc` is already
in `nvm_header_t`.

**Note:** this also removes the stated dependency concern. Assuming HW CRC availability
becomes unnecessary — the adopter simply supplies whatever they have.

**Signature — the legacy-names rule lands well here.** It matches the existing `u32_crc32()`
exactly, so an adopter can point straight at an implementation they already have:

```c
typedef uint32_t (*pfn_nvm_crc_t)(const void *p_v_data, uint32_t u32_size);
```

The name question for `u32_crc32()` is moot: **the core stops having a CRC function at all**,
so the stub simply disappears rather than being renamed. The typedef's `uint32_t` return
matches the `u32_crc` field exactly, so there is no format change and no width juggling.

**Two CRC examples requested by the user** (this session), alongside the storage drivers of I3
and following the same `.example` convention — `nvm_crc_sw_crc32.c.example` (software
Ethernet CRC-32) and `nvm_crc_stm_hw.c.example` (the STM32 CRC peripheral).

**The HW example must save and restore the peripheral's configuration** (user): preserve the
entry polynomial and generator options, reconfigure for what nvmparams expects, and restore on
exit. Three notes for whoever writes it:

- Because CRC-32 with poly `0x04C11DB7` **is** the STM32 CRC unit's reset default, the
  reconfigure step is usually a no-op — but it must still be written explicitly rather than
  assumed, since the whole point of the row is that some other module may have reprogrammed
  the peripheral before nvmparams runs.
- Direct register save/restore of `CRC->INIT`, `CRC->POL` and `CRC->CR` (POLYSIZE, REV_IN,
  REV_OUT) is more surgical and shorter than going through `HAL_CRC_Init` with a
  `CRC_HandleTypeDef`, and an example benefits from being readable.
- Also handle the **clock**: `__HAL_RCC_CRC_CLK_ENABLE()` on entry, and consider restoring the
  prior enable state on exit. An application that never uses CRC may have it gated off, and
  this codebase cares about low-power state.
- **Save/restore protects a configuration, not an in-flight computation.** If another module
  is part-way through a CRC when nvmparams runs, its partial result is destroyed regardless.
  The example's header comment must say the peripheral cannot be shared with an ISR-driven
  user during a commit.

**Interaction with S3 that must be documented — enabling CRC invalidates every existing pool.**
Validation today is signature-only and `u32_crc32()` is stubbed, so deployed pools carry a
meaningless value in `u32_crc`. The first boot after a real CRC is enabled sees a valid
signature with a failing CRC — which S3 classifies as **corrupt**, not blank. Under the default
`NVM_INIT_FORMAT_IF_BLANK` policy that is an **abort**, so init fails outright rather than
recovering.

That is not an argument against the CRC; it is an argument for the phase split and the NULL
default. Existing projects that never set `pfn_crc` keep signature-only behaviour and see
nothing change. An adopter turning it on takes one deliberate transition — a single boot under
`NVM_INIT_FORMAT_IF_INVALID`, or an explicit pool erase — and it belongs in T2's README as a
migration note rather than being discovered in the field.

**Resolution (user, locked): the function-pointer approach, with CRC-32.**

**Phase 1 delivers the shape only** — the `pfn_nvm_crc_t` typedef and the `pfn_crc` member in
the init config. No working CRC is needed until phase 2, and this row now carries the design
phase 2 will implement.

**Algorithm: Ethernet CRC-32** (poly `0x04C11DB7`). Chosen once it was confirmed that
`u32_crc` is already provisioned as a `uint32_t`, so the wider check is free in storage — and
it is the STM32 CRC peripheral's native default, which makes the HW example simpler rather
than harder.

**`pfn_crc == NULL` reproduces legacy behaviour exactly:** no CRC computed, the placeholder
value written into `u32_crc`. Promote that value out of the stub into a named module constant
— `NVM_CRC_PLACEHOLDER` (`0xDEADC0DE`, the value the stub returns today) — so deployed pools
stay byte-identical in that field and the migration note in T2 has something to name.

**Two details that must be got right, and are free to get right now:**

- **With `pfn_crc == NULL`, validation SKIPS the CRC check — it does not compare against the
  placeholder.** Otherwise a pool written by a CRC-enabled build and later read by a NULL-CRC
  build fails, breaking firmware rollback for no reason. Skipping costs nothing and makes the
  downgrade path work.
- **The CRC covers the data only, not the header.** Existing behaviour: `x_nvm_commit()`
  computes over `p_v_data + sizeof(nvm_header_t)` for `u32_size - sizeof(nvm_header_t)` bytes,
  so signature, write count and label are outside it.

**Why the header exclusion matters for W1, and why it still needs no format change.** Wear-level
block selection reads `u32_write_count` from each candidate's header — an unprotected field. A
torn write could therefore present a plausible-but-wrong write count. That is recoverable
*provided phase 2's selection rule is "highest write count whose data CRC also validates"*
rather than "highest write count", falling through to the next candidate on failure. Stated
here so phase 2 does not conclude it needs to extend CRC coverage into the header, which
**would** be a format change.

### D11 — The adopter header as a control panel *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Requirement (user, this session):** adoption must stay near-frictionless. Today the adopter
includes **one** header, `nvmparams.h`, and that is all. That property must survive
vendorization. It is acceptable for that one header to pull in others; it is not acceptable to
make the application include half a dozen. Equally: no copying files into the project that do
not absolutely have to be there, and no editing half a dozen files. The adopter header should
be the library's build-time **control panel** — knobs, optional inclusions, API configuration
— in the manner of `FreeRTOSConfig.h`, and **extensively commented**, explaining what every
knob and declaration does.

**Structure that satisfies "one include" (see I12):** `nvmparams.h` is the sole application
include and pulls `nvmparams_config.h` from the middle of itself. The application never
includes the config header directly and never needs to know it exists.

**The ownership rule** — worth stating explicitly, because it decides where each future
addition goes:

> The **module** header owns everything whose correctness the module controls: types, error
> codes, reserved IDs and range bounds, and the prototypes of the shipped example drivers.
> The **adopter** header owns everything whose value only the application can know: pool
> sizes, the parameter-ID enum, the log shim, and the feature knobs.

**Candidate knob inventory** — to be finalised with I4:

| Knob | Purpose |
|---|---|
| `NVM_POOL_SIZE_DEFAULT` | the project's usual pool size; `_Static_assert`ed against `NVM_POOL_SIZE_MIN` (D6). Not a fallback — `u32_size` must always be set explicitly |
| `NVM_LABEL_MAX_LENGTH` | label length — set once at adoption, never after; see D12 |
| `NVM_ENABLE_INTERNAL_MALLOC` | compile out the `malloc()` path entirely for no-heap projects |
| `NVM_LOG_ERROR` | the logging shim (I11); undefined means no logging |
| the application parameter-ID enum | D8's anchored enum plus its single `_Static_assert` |
| driver externs | commented-out illustrative block; see I3 |

**The panel is deliberately short** (user, this session): once the built-in drivers are gone
there is little left worth adjusting. Two candidates were considered and dropped —
`NVM_ENABLE_LIST`, because I13 removes `x_nvm_list()` from the core outright rather than
gating it, and `NVM_ENABLE_CRC`, because D10's NULL function pointer already expresses "no
CRC" at runtime and a second mechanism for the same thing is only another way to be
misconfigured. `NVM_ENABLE_WEAR_LEVELLING` is not reserved either; block count in the config
struct already expresses it.

**On extensive commenting:** the current header banner is ~110 lines of adoption prose that is
genuinely good and is currently in the wrong file. Most of it moves — the usage walkthrough to
T2's README, the per-knob explanation into this header. That is a relocation, not new writing.

**Resolution (user, locked):** as stated above, plus two additions.

**1. The module ships `nvmparams_config.h.example`,** carrying typical default settings for
every knob. The adopter either edits and renames it in place, or copies it to their own include
directory and renames it. Same `.example` convention as the drivers (I3), and it earns the
suffix for the same structural reason: a file named `*.h.example` cannot be `#include`d by
accident, so "you must rename this" is enforced by the filename rather than by a comment.

**2. `NVM_ENABLE_INTERNAL_MALLOC` stays on the panel,** with one consequence the user
specified: when internal allocation is compiled out, a NULL `p_v_ram_buffer` becomes an
**error** rather than a request to allocate, and init must reject it. That check is itself
`#if`-gated on the knob.

Note what that does to D6, and document it in the panel comment: `p_v_ram_buffer` is the one
field whose zero-value meaning is **contingent on a compile-time setting** — "allocate for me"
with the knob on, "configuration error" with it off. Every other zero-default is unconditional.

`u8_internal_malloc` in `nvm_pool_t` stays present unconditionally even when the knob is off
(it simply reads 0). One byte is cheaper than a second conditional variant of the struct.
`x_nvm_pool_release()` also stays, skipping the `free()` and still clearing the handle.

### D12 — `NVM_LABEL_MAX_LENGTH` and the layout-change hazard *(resolved)*

**Status:** 🟢 · **Needs user:** no

**The hazard (user, this session):** `NVM_LABEL_MAX_LENGTH` sits inside `nvm_header_t`, so it
is part of the **on-media layout**. Changing it after any pool has been created shifts every
object in that pool. Existing data is not merely stale, it is misread — and misread with a
valid signature, so nothing detects it. It also must stay a multiple of 4 or the header stops
being 32-bit aligned and every object after it inherits the misalignment.

**User's leaning:** expose it in the adoption header, but with strongly worded commentary that
it is set **once** at adoption and never changed, and that the 16-byte default should be kept.

**Two mechanical reinforcements, so the comment is a courtesy rather than the only defence:**

**1. Assert the alignment.** One line in the module, and the constraint stops depending on
anyone reading a comment:

```c
_Static_assert((NVM_LABEL_MAX_LENGTH % 4) == 0,
               "NVM_LABEL_MAX_LENGTH must be a multiple of 4 to keep the pool header aligned");
```

**2. Derive the signature from the layout, so a change is *detected* rather than silently
destructive.** `NVM_DATA_SIGNATURE` is currently the fixed magic `0x5AA5A55A`. If the
effective signature instead folds in the layout constants, a pool written under one label
length and read by firmware built with another fails validation and is reformatted to defaults
— a clean loss with visible defaults, rather than plausible garbage. Zero runtime cost, since
it is a compile-time constant, and no change to the header format.

**The migration trap, and how to avoid it entirely:** naively deriving the signature would
invalidate every pool already deployed in the field on first flash. For SwitchTester that is
nothing; for the named compat target it is a data wipe on every unit. Avoided by deriving it
as a *delta from the default*, so the shipped configuration reproduces the existing magic value
bit-for-bit:

```c
#define NVM_EFFECTIVE_SIGNATURE  (NVM_DATA_SIGNATURE ^ (NVM_LABEL_MAX_LENGTH - 16))
```

At the default 16 this XORs by zero and every existing pool validates exactly as it does
today. Only an adopter who *changes* the length sees a different signature — which is precisely
the case that must not go undetected.

**Note this is not W7.** It detects a *layout* change in your own firmware. It does not detect
a foreign pool with the same layout, which is the NOLOAD-reflash case and needs an application
identity rather than a layout identity.

**Resolution (user, locked): expose it, with the strongly-worded comment and the alignment
`_Static_assert`. The derived signature is DECLINED.**

Rationale (user): the mirror project will receive this eventually — probably after wear
levelling — and in that case, or any commercial adoption, the user will be conscious of the
constraint. For third-party adopters the comment plus the assert are judged sufficient. The
signature guard also costs a small amount of bench debuggability, since the magic value would
no longer be a fixed constant to eyeball in a hex dump.

**Measured while resolving this — the label really is inert.** `c_label` is touched in exactly
two places: `strncpy()` into the header at init, and the `x_nvm_list()` dump. It plays **no**
part in any integrity check, search, or decision anywhere in the core. The user's guess was
correct.

**Two small findings that travel with it, recorded for I9 and I13:**

- `strncpy(p_x_header->c_label, p_c_label, NVM_LABEL_MAX_LENGTH)` copies at most exactly the
  field width, so a label of 16 or more characters leaves `c_label` **not NUL-terminated**.
  Currently harmless because the only reader is a `%.16s` print, but it is a trap for anyone
  who later treats it as a C string.
- That reader hardcodes `%.16s` rather than deriving the precision from
  `NVM_LABEL_MAX_LENGTH`, so changing the knob silently truncates or over-reads the dump. It is
  a concrete small instance of the hazard being accepted here, and it lives in the code moving
  out to the `nvm_list` example — fix it there rather than shipping it as a demonstration.

### S1 — The driver contract

**Status:** 🟡 · **Needs user:** no

The contract to be written into the module README and the header:

- A driver validates its own parameters and reports **physical device access errors only**.
- It performs **no** data integrity checking. Deciding whether data read is valid, or whether
  data written is intact, is the core's job.
- It knows **nothing** about wear levelling. It receives one effective address.
- Return `NVM_ERROR_NONE` on success. Negative values are core error codes. **Positive values
  are reserved for device-specific errors** — the existing header already documents this and
  it gives driver authors somewhere to put an SPI status code without inventing enum values.

**To verify during implementation:** that the core tests `!= NVM_ERROR_NONE` rather than
`< 0` anywhere it checks a driver return, or the positive-value convention silently fails.

**Resolution (user, locked): the contract as stated, and positive device codes propagate to
the caller unchanged.**

The core does not translate, clamp or swallow a positive return — it treats any non-`NONE`
value as failure without interpreting it, and passes it up. An application on a bench can then
see the actual SPI status that failed, which is exactly the information you want when a flash
part starts failing intermittently. The cost is that `x_nvm_commit()` may return a value the
core has never heard of, so **T2 must state the caller-side rule: test against
`NVM_ERROR_NONE`, never against a list of known codes, and never `< 0`.**

### S2 — Device allocation unit and block striding *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** How does the core place wear-levelling blocks without knowing the device's
physical layout?

If pool size is not a multiple of the device's erase unit, two wear-level blocks share a sector
and erasing one destroys the other. The core computes the per-block stride as
`ROUNDUP(u32_size, alloc_unit)` rather than assuming `u32_size`.

**Resolution (user, locked):** the allocation unit is an adopter-supplied member of
`nvm_pool_t` (and of the init config). **`0` means "allocation unit = pool size."**

**nvmparams has no knowledge of the target device's physical layout, so this cannot be made
foolproof — the adopter has to supply it** (user). `0` is a convenience for the typical case:
a small pool, smaller than the device's real allocation unit, no striding and no wear
levelling. It **breaks** if the device's real allocation unit is anything other than exactly
the pool size, or if the pool size exceeds the real allocation unit. That must be stated in the
member's comment.

**But the one dangerous case is mechanically detectable, and should be rejected rather than
documented.** With a single block, the stride is never used — there is only one pool at the
base address — so `0` is harmless no matter what the device's real geometry is. `0` becomes
destructive *only* when the core is asked to pack multiple wear blocks with no erase-granularity
knowledge, which is exactly the combination `u8_wear_blocks > 1 && alloc_unit == 0`. The core
cannot validate the *value*, but it can reject that *combination* at init. Recorded under I10;
per the standing guard policy, init rejects rather than clamps.

**Naming:** call it `u32_alloc_unit`, not `u32_block_size`. "Block" already means a wear-level
block in this struct, and the same word in two senses in one header is precisely the sort of
thing that costs an adopter an hour.

**Phase 1 obligation:** compute and store the stride even though only one block exists, so
phase 2 changes no addresses. With `u8_wear_blocks <= 1` the stride is unused but correct.

**Selection rule for phase 2, fixed here by D10:** "highest `u32_write_count` **whose data CRC
also validates**", falling through to the next candidate on failure — not "highest write count".
The write count lives in the header, which the CRC does not cover, so the validation step is
what makes an unprotected selector safe.

**Connects to I9:** a pool larger than one allocation unit needs the driver to erase *every*
unit it spans. `x_mcuflash_write()` currently hardcodes `NbPages = 1`, which is the same bug
from the driver's side. The driver knows its own geometry and can derive the page count from
the transfer size, so this is fixed in the example driver rather than in the core.

### S3 — Init policy on blank, corrupt, or unreadable media *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** What does init do when the media does not hold a valid pool, and what does it
return? The current code formats silently. Once D10 makes the CRC real, "corrupt" becomes
genuinely detectable and the distinctions start to matter.

**User's direction (this session):** the *caller* chooses, via an option in the init config
(a struct member under D5, not a separate argument). Always aborting is not acceptable —
you could then never create a new pool.

**There are four outcomes at init, not three, and the fourth is the safety-critical one:**

| Outcome | How the core knows |
|---|---|
| **Valid** | driver read succeeded; signature and CRC check out |
| **Blank** | driver read succeeded; buffer is uniformly 0xFF or uniformly 0x00 |
| **Corrupt** | driver read succeeded; buffer is neither blank nor valid |
| **Unreadable** | the driver returned an error — the core knows *nothing* about the contents |

**Blank detection is doable without any new configuration.** The core scans the RAM buffer
after read for uniform 0xFF or uniform 0x00, covering both erase polarities in use. No knob and
no third driver callback is needed, and the heuristic cannot produce a harmful false positive:
`NVM_DATA_SIGNATURE` is `0x5AA5A55A`, so "uniformly blank" and "holds a valid signature" are
mutually exclusive by construction. The only pool it could misjudge is a corrupt one that
happens to be uniformly blank — which is indistinguishable from never-written *by definition*,
so treating it as blank is the correct answer anyway.

**Unreadable must never trigger a format, under any policy.** Writing to a device you could not
read is how a transient fault — a loose SPI line, a device not yet powered, a filesystem not
yet mounted — turns into permanent data loss. A driver returning an error means "I could not
access this device," and the core's response is to abort regardless of the policy setting.

That rule stays clean only if drivers handle their own creation cases: a file driver meeting a
nonexistent file **creates it and returns blank data**, rather than reporting an error and
relying on the core to guess. Worth stating in the driver contract (S1) and demonstrating in
`nvm_driver_fileio.c.example`.

**The null device is not a driver** (user, clarifying I3): "remove all built-in drivers" means
removing all *hardware* driver references from the core. The only module-internal path is
`if (pfn != NULL) { call it } else { unconditional success }`.

**One edge that must be pinned or the null device is non-deterministic.** If a NULL `pfn_read`
simply returns success and leaves the RAM pool untouched, what blank-detection then scans
depends on where the buffer came from: an adopter-supplied static buffer is `.bss`-zeroed and
reads as blank, but an internally `malloc()`ed one holds indeterminate bytes and will usually
read as **corrupt** — which under the default `FORMAT_IF_BLANK` policy aborts. Same
configuration, opposite outcomes, decided by a field the adopter may not have thought about.

**Fix, and it reproduces legacy behaviour exactly:** a NULL `pfn_read` `memset`s the pool to
`0x00` before returning success, as today's `NVM_DEVICE_NONE` case already does. Blank
detection then always fires, the pool always formats to defaults, and the outcome no longer
depends on buffer provenance.

**Consequence of I4 — the driverless pool.** With `nvm_device_t` gone, a pool with NULL
`pfn_read`/`pfn_write` is the old `NVM_DEVICE_NONE`: the RAM buffer works normally and nothing
persists. That is a *valid* configuration, not an error. It reads as "blank" every time, so it
formats to defaults on each boot, which is exactly the old behaviour.

**Taxonomy — the user's first two options collapse into one.** Once blank and corrupt are
distinguishable, `NVM_INIT_ABORT_IF_CORRUPT` and `NVM_INIT_FORMAT_IF_BLANK` describe the same
policy: format when blank, abort when corrupt. The proposed set therefore reduces to two
distinct behaviours, plus one genuinely different third:

| Policy | Blank | Corrupt | Unreadable |
|---|---|---|---|
| `NVM_INIT_FORMAT_IF_BLANK` *(leaning: value 0, the default)* | format | **abort** | abort |
| `NVM_INIT_FORMAT_IF_INVALID` | format | reformat | abort |
| `NVM_INIT_REQUIRE_VALID` *(open — is it wanted?)* | **abort** | abort | abort |

`NVM_INIT_REQUIRE_VALID` never writes at all. It is meaningful only for a pool that is supposed
to have been provisioned already — factory calibration, a serial number written on the line —
where silently manufacturing defaults would hide a production fault. Whether that case is worth
an enum value is the open part of this row.

**Why `FORMAT_IF_BLANK` should be the zero value:** per D6, zero is the default, and this is
the safest policy that still lets a first boot work. It never destroys data that a caller might
have wanted to recover, but it needs no configuration to bring up a new pool. Destroying a
corrupt pool becomes something the adopter opts into.

**Return codes.** The module already has the idiom of negative-but-not-really-an-error returns
(`NVM_ERROR_OBJECT_EXISTS`, `NVM_ERROR_NO_CHANGE`), so formatting should report through the
same channel — and with **two** codes rather than one, because the difference matters to
anyone diagnosing a unit:

- `NVM_ERROR_POOL_FORMATTED` — the media was blank; nothing was lost.
- `NVM_ERROR_POOL_REFORMATTED` — the media was corrupt; **data was destroyed.**

Same discipline caveat as D8: a caller who ignores init's return learns nothing, so I7's tests
must assert on these and T2 must show a call site that checks.

**Phase 2 note:** W7's foreign-pool case — valid signature and CRC but belonging to a different
application — is a fifth outcome this taxonomy has room for and phase 1 cannot detect.

**Interaction with D10:** the first boot after an adopter enables a real CRC sees a valid
signature with a failing CRC, which this taxonomy calls **corrupt** — so the default policy
aborts. That transition needs a deliberate one-time `FORMAT_IF_INVALID` boot or a pool erase.
Written up under D10 and owed to T2 as a migration note.

**Resolution (user, locked):** all three policies as tabled, with `NVM_INIT_FORMAT_IF_BLANK`
as value 0 and the default; the four-outcome model including unreadable-never-formats; blank
detection by uniform-0xFF-or-0x00 scan; and the two distinct format/reformat return codes.

**`NVM_INIT_REQUIRE_VALID` is included** (user). The mirror project does not currently use a
separate factory-provisioned pool, but it is a plausible option for a future project —
**especially since the post-refactor module is markedly better at multiple pools.** Today's
version can host several, but the built-in driver switch and the in-module STM flash allocation
make several pools in STM flash awkward; explicit per-pool base addresses and adopter-supplied
drivers remove that. Worth recording as a design dividend that was not among the original
goals.

### S4 — `NVM_ERROR_NO_CHANGE` into the core *(resolved)*

**Status:** 🟢 · **Needs user:** no

`NVM_ERROR_NO_CHANGE = -8` exists only in SwitchTester's copy. It lets a caller distinguish
"committed" from "nothing to commit". The pool's entire purpose is minimising erase/write
cycles, and that is only observable if the two outcomes are distinguishable. Take
SwitchTester's version as the core baseline.

**Resolution (user, locked):** included in the core.

### S5 — `u32_write_count` becomes a monotonic sequence number *(resolved)*

**Status:** 🟢 · **Needs user:** no

This is the cheapest and most important phase-1 forward-compat move. `u32_write_count` already
exists in `nvm_header_t`. If phase 1 increments it on every commit and never resets it, phase
2's block selection is "the highest write count among blocks with a valid signature and CRC"
— with **zero** change to the on-media format.

Combined with D10's phase-1 plumbing, this means **phase 1's on-media format is already the
phase-2 format**, which is the whole corner-avoidance requirement satisfied by two small
pieces of work.

**To confirm during implementation:** that the current code actually increments it on commit
and does not reset it on reformat.

**Resolution (user, locked):** phase 1 makes it monotonic, never reset, incremented on every
commit.

### I1 — Remove the built-in flash buffer and linker-section dependency *(resolved)*

**Status:** 🟢 · **Needs user:** no

`static uint32_t nvm_mcu_flash[...] __attribute__((section(".nvmdata")))` in `nvmparams.c` is
the module reaching into the adopter's linker script. It moves out with the STM flash driver
(I3): the driver owns the buffer, or the adopter declares it and passes its address as
`ux_base_address`. The latter is more honest — the address is exactly what D1's struct exists
to carry — and it removes the last reason for the module to know that `.nvmdata` exists.

**Cleanest form, and it needs no `section` attribute at all.** SwitchTester's linker script
already exports `_nvm_start` and `_nvm_end` (see T2), so the adopter's config literal reads:

```c
extern uint32_t _nvm_start;      /* provided by the linker script */
...
.ux_base_address = (uintptr_t) &_nvm_start,
```

No array in the module, no `__attribute__((section))` anywhere, and the adopter can sanity-check
their own pool size against `&_nvm_end - &_nvm_start`. This is the concrete reason D2 chose a
`uintptr_t` rather than a pointer: a linker symbol's address is exactly the sort of thing that
is an address in one backend and an offset in another.

**Resolution (user, locked):** all legacy pre-allocated STM flash buffers and related baggage
are removed. The linker-symbol access pattern above is approved **on condition that it is
documented clearly in the README** (T2), which already carries the matching linker-script
section.

The stray `#define SPIFLASH_NVM_DATA_ADDRESS 0x0400` goes at the same time.

**Resolution:** _(pending)_

### I2 — C-library-only dependencies *(resolved)*

**Status:** 🟢 · **Needs user:** no

Target state, consistent with `logging` / `uart_stream` / `automation_console`: the core
names exactly one application file — its own `nvmparams_config.h` — plus C library headers,
and nothing else. Today it pulls the STM32 HAL for `HAL_FLASH_*` and `FLASH_BASE` /
`FLASH_PAGE_SIZE`. All of that leaves with the driver.

**To audit at the end:** the include list of `nvmparams.c` and `nvmparams.h` should contain
only `<stdint.h>`, `<string.h>`, `<stdlib.h>`, `<stdbool.h>`, and `nvmparams_config.h`.

**Resolution (user, locked):** audit and trim the header list as above. Note that `<stdlib.h>`
is itself conditional on `NVM_ENABLE_INTERNAL_MALLOC` (D11), so a no-heap build's include list
is four headers. `<stdio.h>` is gone entirely once `x_nvm_list()` leaves (I13), and the STM32
HAL leaves with the flash driver (I1/I3).

### I3 — Prepackaged driver set and where the files live *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** Which drivers ship, and are they part of the vendored module or adopter port
code?

**User's list:** STM32 flash, a filesystem/stdio driver, and a RAM-emulated degenerate case.
RTC backup RAM was raised as a candidate for the degenerate case and is **ruled out on this
part** — see LOCKED CONTEXT, 20 bytes against a 28-byte header. It becomes W2.

**Tension to resolve:** phase 1 says "removal of all built-in drivers", and also "creation of
STM flash and RAM driver examples". The reading that satisfies both is that drivers are
removed from the **core translation unit** and shipped as **separate, optional translation
units** the adopter chooses to compile — `nvmparams_drv_mcuflash.c`, `nvmparams_drv_ram.c`,
`nvmparams_drv_file.c`. Adoption then reads: copy the module, copy the one driver that matches
your target, write one config literal. That directly serves the "as easy as possible" goal and
the named compat target.

**User's proposal (this session), and it is better than the house convention:** ship them
inside the module directory with a double extension —

```
nvm_driver_stm_flash.c.example
nvm_driver_fileio.c.example
nvm_driver_ram.c.example
nvm_driver_*.c.example
```

The adopter either renames one in place to use it as-is, or copies it into application space
and modifies it.

**Supporting finding — the `.example` suffix removes a manual step the existing convention
requires.** `logging_port_template.c` ships as a plain `.c` and its banner says, in a boxed
warning, *"THIS FILE IS EXCLUDED FROM THE BUILD."* That exclusion is a per-project CubeIDE
`.cproject` setting the adopter has to replicate by hand, and forgetting it compiles a template
into their image — for the mcuflash driver, dragging in the STM32 HAL and breaking any
non-STM32 or headless adopter immediately. A file named `*.c.example` matches no build system's
source glob anywhere, so the exclusion is structural rather than procedural and there is
nothing to remember. This directly serves D11's "don't make them edit half a dozen files."

Consequence worth noting: this makes nvmparams' convention diverge from `logging`'s. The
divergence is an improvement and `logging` should arguably follow later, rather than nvmparams
adopting the weaker pattern for consistency's sake.

**Where the prototypes live — a refinement on the user's sketch.** The proposal was that the
adopter header carry all example-driver prototypes commented out, for the adopter to uncomment.
That works, but commented-out declarations are invisible to every tool and drift silently: if
a driver's signature changes in phase 2, the stale commented prototype sits in every adopter's
header and nobody notices until they uncomment it. Per D11's ownership rule, the module owns
signatures it controls. So:

- **`nvmparams.h` declares the shipped example drivers unconditionally.** An `extern`
  declaration for a function nobody calls generates no code and costs nothing, so there is no
  knob to set and nothing to uncomment. It also makes the module header a discoverable menu of
  what is available.
- **The adopter header carries a commented-out block for *modified* copies** — the case where
  an adopter renames or reshapes a driver and the module cannot know its signature. That is
  where the uncomment-what-you-need idiom genuinely belongs.

Adoption then reads: copy the module directory, rename one `.c.example`, add it to the build,
write one config literal. No prototype editing at all in the common case.

**Resolution (user, locked):** packaging as above.

**Phase-1 example set, narrowed** (user, this session):

| Example | Phase | Note |
|---|---|---|
| `nvm_driver_stm_flash.c.example` | **1 — required** | the real backend |
| `nvm_driver_ram.c.example` | **1 — required** | degenerate teaching case; also I7's fault injector |
| `nvm_driver_spiflash.c.example` | **1 — required as of 2026-08-17** | promoted from stretch: the mirror is migrating its NVM to SPI flash (LOCKED CONTEXT) |
| `nvm_driver_fileio.c.example` | **2** | developed and tested in LED_Strip, which already has a partition manager, VFS and littlefs with stdio integration |
| `nvm_crc_sw_crc32.c.example` | 2 | D10 |
| `nvm_crc_stm_hw.c.example` | 2 | D10 |

**Measured — SwitchTester's SPI hardware support is further along than assumed.** The user's
note was that the W25Q128 has "no software support"; in fact `SwitchTester.ioc` already
configures **SPI3** with the pins labelled for this exact purpose — `PA15 SPIFLASH_NCS`,
`PB3 SPIFLASH_SCK`, `PB4 SPIFLASH_MISO`, `PC12 SPIFLASH_MOSI` — master, 8-bit, 8 MBit/s, and
`MX_SPI3_Init()` is already called from `main.c:117`. **The peripheral is up. The gap is the
W25Q128 chip driver only**, which is `spiflash.c/.h` + `spiflash_ll.c/.h` from LED_Strip.

**Correction to the record — LED_Strip's `spiflash` is the user's own code, not a third-party
clone.** The user recalled "a vendored SPI module cloned from a public github repo". Measured:
`App/spiflash/` carries no copyright, licence or upstream markers, is written to this project's
own D-log conventions (its headers cite "D4 layer 2", the "D7" opcode namespace, and a W3
OCTOSPI wish row) and states its opcodes were verified against the W25Q128JV datasheet. The
public-repo modules in that project are **`App/littlefs/`** and **`App/tlsf/`**.

Sizes, for the build-versus-borrow decision:

| File | Lines | Role |
|---|---|---|
| `spiflash_ll.c/.h` | 257 + 115 | bus transport: chip-select, opcode/address/dummy header, polled-or-DMA data phase, shared-bus lock, re-entrancy guard |
| `spiflash.c/.h` | 648 + 306 | opcodes, status registers, write-enable, busy-wait, JEDEC id |
| `spiflash_common.h` | 55 | error/address/line-width types |

About **960 lines** for the two layers that matter, and `spiflash_ll.h` includes `main.h`, so it
is not dependency-clean by this effort's own standard.

**Which of the two routes is cheaper:**

- **Vendor the chip driver into SwitchTester.** Cost is concentrated in porting the LL layer
  from G474 to G0B1. The *storage* side is clean — SwitchTester's W25Q128 is entirely unused,
  so a raw pool at offset 0 needs no coordination with anything.
- **Do it in LED_Strip instead.** The chip driver already works there, but LED_Strip has no
  nvmparams yet (T3 — an introduction, not a port), and its SPI flash is already carved up by
  `spiflash_part.c` with littlefs above it. A raw nvmparams pool would have to take a partition
  or reserve space outside the table, which drags a design question in *that* project into this
  one.

**Leaning for the SPI example specifically: roll a minimal polled HAL-only driver** (the user's
second suggestion), and keep it separate from whatever SwitchTester eventually does for bench
use of the part.

An example is not a product. Its job is to teach the *nvmparams driver interface*, and one that
first requires vendoring ~960 lines of someone else's SPI-NOR stack teaches the wrong lesson —
an adopter on a different MCU, a different flash part, or a different bus gets nothing from it.
A minimal W25Q128 driver is roughly **150 lines** against plain `HAL_SPI_Transmit`/`_Receive`:
`WREN` (0x06), `READ` (0x03), `PAGE_PROGRAM` (0x02), `SECTOR_ERASE_4K` (0x20), `RDSR1` (0x05)
with a BUSY poll, and chip-select on `PA15`. That is the right size for a file whose purpose is
to be read.

**Two device details the example should deliberately demonstrate**, because both are exactly
what S1's contract says belongs in a driver rather than the core:

- **The 256-byte page-program limit.** A program that crosses a page boundary *wraps within the
  page* rather than continuing, so a 512-byte pool write must be split into 256-byte chunks.
  A driver-side concern the core neither knows nor needs to.
- **The 4 KB sector erase**, against the G0 internal flash's 2 KB page. Two backends on the same
  board with different `u32_alloc_unit` values is a live demonstration of why S2 made it
  adopter-supplied rather than assumed.

Keeping the example minimal also leaves SwitchTester free to vendor the fuller `spiflash`
module later for bench work — that is SwitchTester's business, not nvmparams'.

**Risk being accepted if SPI slips.** The W25Q128 was wired to this bench specifically so the
pluggable driver layer could be exercised against a *real second backend*, on the reasoning
that an interface validated against one backend encodes that backend's assumptions. Shipping
phase 1 with STM flash and RAM only does not discharge that: RAM is degenerate — no erase, no
alignment constraint, no failure modes — so it exercises almost nothing the interface could get
wrong.

**The real deadline is not phase 1, it is the mirror adoption.** While adoption is limited to
the three in-house projects the interface can still be changed cheaply; once the mirror takes
it, it cannot. Mirror adoption is scheduled after wear levelling (D9), so validating against a
second real backend must land before then — in phase 1 as a stretch, or early in phase 2, but
not later.

### I4 — What leaves `nvmparams.h` *(resolved)*

**Status:** 🟢 · **Needs user:** no

Inventory of application-specific content currently in the module header, all of which moves
to the adopter's `nvmparams_config.h`:

- The entire `nvm_param_id_t` body — `NVM_CONFIG_SERIAL_NUMBER`, `_PRODUCT_ID`, `_SKU`,
  `NVM_PARAM_BASE_ID`, `NVM_PARAM_SWITCH_PULSE_MS`, the twelve `NVM_PARAM_CYCLE_*`, and
  `NVM_PARAM_TEST_1..3` (mechanism per D8).
- `extern nvm_pool_t g_x_nvm_param` — the pre-declared project pool.
- `NVM_POOL_SIZE_DEFAULT`, and `NVM_LABEL_MAX_LENGTH` if adopters should be able to trade
  label length against per-pool overhead.
- The `nvm_device_t` enum in its entirety — it is superseded by function pointers.
- The usage prose in the header banner, which is adoption documentation and belongs in T2's
  README.

What **stays** in the module header: `nvm_error_t`, `nvm_header_t`, `nvm_object_t`,
`nvm_pool_t`, `nvm_media_t`, `nvm_pool_config_t`, the callback typedefs, the reserved ID
constants and range bounds, `NVM_DATA_SIGNATURE`, and the public function declarations.

**Resolution (user, locked):** the inventory above stands, with three points settled.

**`nvm_device_t` is deleted outright, along with its `x_device` member in `nvm_pool_t`.** It is
not relocated anywhere — the driver function pointers supplant it completely. The module's only
awareness that a driver exists is the pointer handed to init.

**But `NVM_DEVICE_NONE` carried a real semantic that must survive:** a pool whose RAM buffer
works normally while nothing is ever persisted. With function pointers that becomes simply
**leaving `pfn_read`/`pfn_write` NULL**, which D6's zero-is-default gives for free. The core
must therefore treat NULL driver pointers as a valid configuration — read is a no-op leaving
the RAM pool untouched, write is a no-op reporting success — rather than rejecting them at
init. Specified under S3.

**`x_nvm_list()` leaves the core entirely** rather than being knob-gated — see I13.

**Consequential edit:** `x_nvm_list()` currently prints `p_x_pool->x_device`. That line goes
with the member.

### I5 — Block discovery as a function *(resolved)*

**Status:** 🟢 · **Needs user:** no

Isolate "which block holds the live pool" into a single internal function whose phase-1 body
returns block 0. Phase 2 replaces one function body instead of restructuring init.

**Resolution (user, locked):** as above, with the shape below.

**The user's model is correct: read from the highest write count, write to the lowest.** Each
block holds a *complete* pool image, and `u32_write_count` is a per-version sequence number in
the header, so the highest is the newest and the lowest is the stalest — which is the one whose
contents can be discarded. Two refinements:

- **Blank blocks are used first.** A never-written block has no write count at all. The write
  target is therefore "any blank block, else the lowest valid write count."
- **Selection reads must validate.** Per D10, the write count sits in the header, which the CRC
  does not cover, so the rule is "highest write count *whose data CRC also validates*", falling
  through to the next candidate.

**One helper, not two, and not one with a find-lowest/find-highest flag.** Both queries need the
same scan — read each block's header, validate it, note its write count — so a flag argument
would double the device reads, which on SPI flash is real time rather than a rounding error.
One function scans once and returns the whole answer:

```c
typedef struct
{
    uint8_t  u8_live_block;      /* highest valid write count, or 0 if none */
    uint8_t  u8_write_block;     /* blank if any, else lowest write count */
    uint32_t u32_live_count;     /* live block's write count, 0 if none valid */
    bool     b_any_valid;
}
nvm_block_scan_t;
```

Only init scans. After that the module knows the live block, so commits consult the cached
result. **Phase 1's body returns `{0, 0, header count, validity}` without scanning at all.**

**Worth knowing before phase 2 is scheduled: wear levelling also buys atomicity.** Because the
live block is never erased until its replacement has been written and verified, a power loss
mid-commit leaves the previous copy intact and selection simply falls back to it. Today a torn
write destroys the only copy. That is a robustness argument for wear levelling independent of
the endurance one, and it may matter more to the mirror than the cycle count does.

### I6 — Reserve wear-levelling config fields now *(resolved — user, locked)*

**Status:** 🟢 · **Needs user:** no

`u8_wear_blocks` and `u32_block_size` go into `nvm_pool_config_t` and `nvm_pool_t` in phase 1
even though phase 1 rejects any block count above 1. With D6's zero-is-default they cost
nothing to leave unset, they document intent, and they mean phase 2 does not change the
adopter-facing struct — which matters because changing it means revisiting every adopting
project's config literal.

Phase 1 should return `NVM_ERROR_PARAMETER` for `u8_wear_blocks > 1` rather than silently
ignoring it.

**Resolution:** _(pending)_

### I7 — HIL and unit tests *(resolved)*

**Status:** 🟢 · **Needs user:** no

Tests land in SwitchTester, driven through the automation console with host scripts, joining
the existing suite (`scripts/hil/test_acon.py`, currently 48/48).

**SwitchTester is the ONLY viable HIL platform, and tests do not migrate** (user, this
session). This is a bigger simplification than it looks:

- **Skeleton is the minimalistic base for new projects and must not carry test dead weight.**
  So T3's back-port takes the *module* and not the suite — worth stating, because "port to
  Skeleton" would otherwise reasonably be read as including it.
- **LED_Strip** has only a precursor version of the automation console.
- **The mirror** has its own, completely different HIL test interface.

**Consequence: test coverage is cheap here in a way it never is in a portable module.** There is
no three-project migration cost per test, so the suite can be generous without that generosity
being paid for repeatedly. Add tests freely.

**Phase split** (see the phase plan in Global notes): a round-trip smoke test in phase 1 —
create/set/commit/reset/get, plus `NVM_ERROR_NO_CHANGE` — because phase 1 is what rewrites 800
lines of a production-proven module and that is exactly where a regression net earns its keep.
The exhaustive suite, including fault injection, follows in phase 2 once the interface has
stopped moving.

**The RAM driver is a test instrument, not only a teaching example.** Because drivers are
function pointers rather than a compile-time switch, the suite can install a
**fault-injecting** RAM driver at runtime — one returning `NVM_ERROR_DEVICE` on the Nth access
— and exercise every error path in the core without a storage device that can actually fail.
This is the strongest testability argument for the pointer-based design and it makes the suite
substantially more valuable than a happy-path test.

**Coverage sketch:** create/get/set/commit round trips; `NVM_ERROR_NO_CHANGE` on a no-op
commit; delete and its garbage collection; pool-full behaviour; object larger than the pool;
`x_nvm_get` on a nonexistent ID; corrupt-pool detection; init against blank media; the
fault-injection paths; and — once D9 lands — an attempt to use a reserved ID.

**Resolution:** _(pending)_

### I8 — Auto-commit delay defined twice *(resolved)*

**Status:** 🟢 · **Needs user:** no

Parked from the logging plan. The NVM auto-commit delay exists as
`DEV_CONFIG_NVM_COMMIT_DELAY_MS` in `device_config.h` and `NVM_AUTO_COMMIT_DELAY` in
`platform.h`.

**Resolution (user, decided during the logging work; carried in here):** keep the
`device_config.h` definition, drop the `platform.h` one.

Note the split this reflects, which should be stated in T2: the auto-commit *timer field*
(`u16_commit_timer`) lives in `nvm_pool_t`, but the *policy* — how long, and who decrements it
— is entirely the application's. The module offers a place to keep the count and nothing more.
Neither name belongs on D11's control panel for that reason: it is not a knob the module reads.

### I9 — `x_mcuflash_write()` geometry bugs

**Status:** 🔴 · **Needs user:** no

These carry into the example driver, so they get fixed as part of writing it rather than
deferred to W5's review. All three are latent at the current 512-byte pool and become
reachable the moment pool size is adopter-supplied or blocks are plural:

- It erases exactly **one page** (`NbPages = 1`) but programs `u32_size` bytes. Any pool
  larger than the 2 KB G0 page writes into unerased flash.
- It masks the RAM source pointer to 4-byte alignment and dereferences it as `uint64_t *`.
- It does not round `u32_size` up to a doubleword, so the final `HAL_FLASH_Program` can read
  past the end of the RAM buffer.

Also in the same neighbourhood: `x_nvm_read()`'s `NVM_DEVICE_NONE` case **zeroes** the RAM
pool. Reasonable for a null device, but it is the opposite of what a RAM-emulated backend
wants, and RAM-emulation is an intended example. The `#if 0` FILE demo does not compile as
written (`fread` with the wrong argument count, a read call in the write path,
`x_status = ERRNO`); if it graduates to the shipped file driver it must actually build.

**Resolution:** _(pending)_

### I10 — Constraints the core validates at init

**Status:** 🔴 · **Needs user:** no

Now that pool geometry is adopter-supplied rather than compiled in, init is the only place
these can be caught. Checks: **`u32_size >= NVM_POOL_SIZE_MIN` and a multiple of 4** (D6 —
this one is locked); `u32_alloc_unit` a power of two or at least non-pathological;
`u8_wear_blocks` within range (I6); RAM buffer alignment when adopter-supplied.

**Two checks promoted from other rows, both of which turn a silent data-destroying
misconfiguration into an init error:**

- **`u8_wear_blocks > 1` together with `u32_alloc_unit == 0` must be rejected** (S2). The core
  cannot validate the allocation unit's *value* against a device it knows nothing about, but
  this *combination* is unambiguously wrong — it asks the core to pack multiple wear blocks
  with no erase-granularity knowledge. With a single block the stride is never used, so `0` is
  harmless everywhere else.
- **NULL `pfn_read`/`pfn_write` is explicitly valid**, not an error — it is the old
  `NVM_DEVICE_NONE` (I4, S3). The obvious implementation of "validate the config" would reject
  exactly this case, so it needs stating.

Per the standing guard policy, init is a **host/automated-reachable** path in the HIL context,
so it rejects rather than clamps.

**Resolution:** _(pending)_

### I11 — Optional logging with no hard dependency

**Status:** 🟡 · **Needs user:** no

**Requirement (user):** nvmparams may use the logging API's `LOGCT()` macro, but must not
*depend* on `logging` — consider a headless application that vendors nvmparams and has no
console port. The user proposed `#if`-guarded log sites, with the adopter's config header
including `logging.h` if it wants logging.

**Complication found while checking the macro.** `LOGCT(tag, fmt, ...)` token-pastes:
it expands `tag ## _TAG` and `tag ## _COLOR`. So a call of `LOGCT(LOG_NVM, ...)` requires the
adopter to have defined **three** things — `LOG_NVM`, `LOG_NVM_TAG`, `LOG_NVM_COLOR` — in
their `logging_config.h`. Guarding the call sites with `#if` does not remove that coupling; it
only makes it conditional. The module would still be naming a logging class it does not own.

**Leaning — invert it: the adopter defines the shim, the module only calls it.**

```c
/* nvmparams.c — the module's entire logging surface */
#ifndef NVM_LOG_ERROR
  #define NVM_LOG_ERROR(...)    ((void)0)
#endif
```

```c
/* nvmparams_config.h — adopter, only if they want logging */
#include "logging.h"
#define NVM_LOG_ERROR(fmt, ...)   LOGCT(LOG_NVM, fmt, ##__VA_ARGS__)
```

nvmparams then never mentions `logging.h`, `LOGCT`, or any tag name — not even inside a guard.
The dependency lives entirely in the one application file the module is already permitted to
name, which is exactly what the strategy doc's dependency rule asks for. A headless project
simply does not define the macro. A project using a different logger — `printf`, RTT, a vendor
logger — maps the shim to that instead, with no change to the module. And the module body
carries **no** `#if` blocks at all: every log site is a plain `NVM_LOG_ERROR(...)` call, rather
than each invocation being individually guarded.

**Two trade-offs to state in T2 rather than discover:**

- The no-op fallback discards its arguments, so **log arguments must have no side effects** —
  `NVM_LOG_ERROR("%d", i++)` loses the increment in a logging-less build.
- It also means format strings are not compiler-checked in a logging-less build. This departs
  from the logging module's own principle of having exactly one macro definition with no
  no-op `#else`, so that every call site is format-checked in every configuration. The
  mitigation is that all three reference projects have logging, so a format error surfaces
  there; the headless case inherits whatever the reference builds validated.

**Keep the log surface small.** Candidate sites only: reserved-ID rejection (D8), pool-corrupt
detection at init, a device error returned by a driver, and allocation failure. A vendored
module that chatters is a nuisance to whoever adopts it.

**Resolution:** _(pending)_

### I12 — Header include topology *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Question:** D8's anchored enum needs `NVM_ID_APP_FIRST` and `NVM_ID_APP_MAX` visible inside
`nvmparams_config.h`, but the dependency-rule pattern has `nvmparams.h` include
`nvmparams_config.h` — which is circular.

**The previous leaning was a separate `nvmparams_ids.h` that both headers include. D11
supersedes it:** a third header is a third thing in the adopter's mental model, and the
requirement is that the application include exactly one. So the cycle gets cut by ordering
inside a single file instead.

**Resolution:** `nvmparams.h` is the sole application include and is structured in three parts:

```
   nvmparams.h
   ├── Part A  core constants the config needs
   │           nvm_param_id_t, NVM_ID_APP_FIRST, NVM_ID_APP_MAX,
   │           NVM_ID_RESERVED_FIRST, nvm_error_t
   ├── #include "nvmparams_config.h"        <-- the adopter's control panel
   └── Part B  everything that depends on config values
               nvm_header_t (needs NVM_LABEL_MAX_LENGTH), nvm_pool_t,
               nvm_media_t, nvm_pool_config_t, prototypes
```

The application includes `nvmparams.h` and nothing else, exactly as today. The config header is
never included directly, and to make that failure mode legible rather than confusing it opens
with the standard sentinel guard:

```c
#ifndef NVMPARAMS_H_INSIDE
  #error "Do not include nvmparams_config.h directly - include nvmparams.h"
#endif
```

Without that guard, including the config first yields a partially-defined `nvmparams.h` and an
error some distance from the cause. With it, the message names the fix.

### I13 — `x_nvm_list()` leaves the core *(resolved)*

**Status:** 🟢 · **Needs user:** no

`x_nvm_list()` is the **only** part of the core that uses stdio — a dozen `printf` calls
formatting the pool header and an object dump. Its body is already wrapped in
`#if DEBUG_MENU`, an *application* macro the module has no business knowing: the same class of
coupling as I11's logging problem, and a second instance of the pattern.

**Resolution (user, locked): move it out of the core entirely** and ship it as another
`.example` with its own small header — `nvm_list.c.example` plus `nvm_list.h`. It is a
debugging tool, not something a release build is likely to need. **This is the one case where
an adopter including a second header is acceptable** (user, explicit), because they are opting
into a debug facility rather than into the library.

That is structurally better than the knob it replaces: the stdio dependency does not become
conditional, it ceases to exist. `NVM_ENABLE_LIST` accordingly drops off D11's panel, and I2's
C-library-only goal is met by construction rather than by configuration.

**Two small dependencies to resolve when writing it**, and both are informative:

- It needs pool traversal. `p_x_next_nvm_object()` is currently module-internal, and
  `ROUNDUP4` comes from outside the module. The example either re-implements the three-line
  traversal against the public types or the core exports the helper.
- **This doubles as a test of the public header.** If a debug dumper can be written entirely
  against `nvmparams.h`, the header exposes enough for a client to do real work. If it cannot,
  that is information about what the header is missing — which is exactly the sort of thing
  that is cheap to learn now and expensive to learn after three projects have adopted it.

**Also drop with it:** the `p_x_pool->x_device` line (I4), and the question of what the
function returns when compiled out — with the file simply absent, there is nothing to return.

**Bug found in passing, recorded for I9:** the `p_x_pool == NULL` check sits *after*
`p_x_nvm_header` is initialised from `p_x_pool->p_v_data`, so a NULL pool dereferences before
the guard meant to prevent it. It travels with the function into the example, and should be
fixed there rather than shipped as a demonstration of how to write one.

### I14 — Commit-timer accessors *(resolved)*

**Status:** 🟢 · **Needs user:** no

**Measured: no structured management exists.** The module only ever *resets*
`u16_commit_timer`, in five places — `x_nvm_commit()` (both the wrote-it and nothing-to-write
paths), `x_nvm_create()`, `x_nvm_delete()` and `x_nvm_set()`. Incrementing and comparing are
entirely the application's, done inline in SwitchTester's `v_timer_update()` against
`DEV_CONFIG_NVM_COMMIT_DELAY_MS` (5000).

**User's proposal:** add a tick accessor for a periodic ISR to call, and
`b_nvm_commit_time_elapsed(&pool, compare_value)`. Direct access to `pool.u16_commit_timer`
stays available; the accessors encapsulate rather than replace it.

**Endorsed** — six lines of code that every adopter would otherwise write slightly differently,
which is the same argument that justified D8's checked wrappers. Three details the existing
call site exposes:

**1. The tick is not a count, it is an interval.** SwitchTester increments by
`PERIODIC_TIMER_INTERVAL_MS`, not by 1, so the units are milliseconds and the tick rate is the
application's business. The accessor must take the increment rather than assume it:

```c
void v_nvm_commit_timer_tick (nvm_pool_t *p_x_pool, uint16_t u16_elapsed_ms);
bool b_nvm_commit_time_elapsed(const nvm_pool_t *p_x_pool, uint16_t u16_limit_ms);
void v_nvm_commit_timer_reset (nvm_pool_t *p_x_pool);
```

**2. Saturate; never wrap.** A `uint16_t` of milliseconds wraps in 65.5 s. Today's call site
avoids that by gating the increment on `timer < LIMIT`, which stops the count at the limit —
the accessor has no limit in hand, so it saturates at `0xFFFF` instead. Without that, a pool
left uncommitted long enough would see `elapsed()` silently go false again.

Both functions should also gate internally on `u8_need_commit`: tick is a no-op and elapsed is
false when there is nothing to commit. That makes both safe to call unconditionally and removes
the external gate the current call site has to remember.

**3. Level-triggered where today's code is edge-triggered.** `v_timer_update()` fires
`JOB_NVM_COMMIT` exactly once, on the tick that crosses the threshold. A polled
`b_nvm_commit_time_elapsed()` stays true until something resets it. That is self-limiting in
the normal case, because committing resets the timer — but if a commit *fails*, the predicate
stays true and the application retries every tick. Worth documenting; an application wanting
backoff resets the timer itself on failure. (An alternative collapsing both functions into one
edge-triggered `b_..._tick(pool, increment, limit)` returning true only on the crossing was
considered, and rejected as merging two concerns into three arguments.)

**Debounce is intentional, not inherited** (user, this session). Because `x_nvm_set()`,
`x_nvm_create()` and `x_nvm_delete()` all reset the timer, it measures time since the **last**
change, and holding off indefinitely under frequent updates **is the design intent**.

**The guard against indefinite deferral is architectural, not temporal.** An application that
needs a commit at a particular moment calls `x_nvm_commit()` directly; the mirror project does
exactly this at several key points, including a final commit before power-down. The background
auto-commit is not expected to protect against starvation, and a future reader should not
re-raise that objection without reading this paragraph first.

**Resolution (user, locked):** all three accessors in phase 1, debounce semantics kept
unchanged, saturating at `0xFFFF`, both functions gating internally on `u8_need_commit`, and
the level-triggered behaviour documented in T2.

**Phase 2 extension is already free — see W8.** The user considered and deferred a two-tier
window: the existing inactivity timeout, plus a second longer timeout measuring time since the
*first* uncommitted write, forcing a commit when it expires. Nothing in phase 1 blocks it:
`nvm_pool_t` is never serialised — only `nvm_header_t` and `nvm_object_t` reach the media — so
phase 2 can add a second counter field freely, `v_nvm_commit_timer_tick()` can tick both
without a signature change, and the second threshold gets its own predicate rather than
overloading `b_nvm_commit_time_elapsed()`. The only foresight phase 1 owes is naming: keep
`v_nvm_commit_timer_reset()` documented as resetting **the inactivity timer specifically**, so
the name does not become ambiguous when a second timer exists.

### T1 — Plan home *(resolved)*

**Status:** 🟢 · **Needs user:** no

This plan lives in `G0B1_Skeleton/Docs/planning/nvmparams-plan.md`, per the standing brief in
`improvements-backlog.md` item 3 and the Skeleton project memory. The core lands in Skeleton;
SwitchTester carries only driver and test work, so the plan follows the core.

Note a deviation from the decision-log model's step 6: it says to sync
`Docs/SwitchTester-Design.md` from the plan once decisions land. That does not apply here —
nvmparams is not a SwitchTester feature. The contract document for this work is
`portable-apis-strategy.md` plus the module README (T2).

**Resolution:** Plan lives in Skeleton. Design-doc sync target is the module README and the
strategy doc, not `SwitchTester-Design.md`.

### T2 — Module README

**Status:** 🔴 · **Needs user:** no · **Required before phase 1 can be called shipped**
(user) — but explicitly *not* an implementation blocker.

Per backlog item 6, each vendored module carries an adoption README. For nvmparams it must
cover: the file manifest (core versus optional `.example` files versus adopter-owned); the
config header's knobs; **how to write a driver**, with the RAM driver as the worked minimal
example and S1's contract stated plainly; the ID-space rules from D9 and the anchored-enum
idiom from D8; a minimal integration snippet; and the measured gotchas.

**Much of it is relocation, not new writing** (user): the current header banner is ~110 lines
of adoption prose that already amounts to an incomplete how-to.

**Required section — reserving a flash region in the linker script**, for adopters using the
STM flash driver. The existing header points at "the STM32G030K8TX_FLASH.ld file for the
ESprayer project", which an adopter cannot follow. SwitchTester's own
`STM32G0B1RETX_FLASH.ld` is a working, already-commented example and the README should carry
it directly:

```ld
MEMORY
{
  RAM       (xrw) : ORIGIN = 0x20000000, LENGTH = 144K
  FLASH     (rx)  : ORIGIN = 0x8000000,  LENGTH = 510K   /* <- 512K MINUS the NVM sector */
  /* NVM_FLASH should be located at the end of MCU FLASH memory, aligned to the
     start of a STM32G0 FLASH sector, with its size set to the sector size
     (2K for STM32G0) */
  NVM_FLASH (r)   : ORIGIN = 0x807F800,  LENGTH = 2K
}

  .nvmdata (NOLOAD) :
  {
    . = ALIGN(8);
    _nvm_start = .;
    *(.nvmdata)
    *(.nvmdata*)
    . = ALIGN(8);
    _nvm_end = .;
  } >NVM_FLASH
```

Three points the README must call out, because each is a real trap:

- **`FLASH` LENGTH must be reduced** by the reserved size (512K → 510K here). Forgetting this
  is the classic error: the regions overlap and the linker happily places code where the pool
  will be erased.
- **`(NOLOAD)`** is what makes parameters survive a firmware update — and is also why a foreign
  pool can outlive the code that wrote it. Cross-reference the gotcha below.
- **`_nvm_start` / `_nvm_end`** are how the adopter supplies the address without the module
  owning any linker knowledge — see I1.

**Measured gotchas section:** the NOLOAD reflash trap above all (a foreign pool with a valid
signature reads as intact, and CRC does not catch it because it is not corrupt); the
CRC-enablement migration from D10; the label-length hazard from D12; I14's level-triggered
commit predicate; and D8's asymmetry, where `x_nvm_get` accepts an ID that `x_nvm_set` rejects.

### T3 — Port to Skeleton, introduce to LED_Strip

**Status:** 🔴 · **Needs user:** no

Phase-1 development happens in SwitchTester; the core then goes up to Skeleton per the
back-port model, and LED_Strip receives it.

**LED_Strip is an introduction, not a port** — it has no nvmparams today. It does have
`App/spiflash/` with littlefs above it, which makes it the natural home for the file-backed
driver and gives the interface a third genuinely different backend. Three projects, three
backends — internal flash, raw SPI flash, and a filesystem — is about as good a validation of
the driver interface as is available without inventing work.

**Note:** SwitchTester is also to receive the W25Q128 raw-SPI backend using the chip driver
from LED_Strip (`spiflash.c`/`.h` and `spiflash_ll.c`/`.h` only — not `spiflash_part.*` or
`spiflash_lfs.*`). Whether that is phase 1 or phase 2 is not yet stated; it is the only thing
that exercises a real second backend on this bench.

### T4 — Provenance boundary for cherry-picked code

**Status:** 🟡 · **Needs user:** no — the user has set the boundary; this records it

**Constraint (user, 2026-08-17):** SwitchTester is a **public** repository. Private or
confidential work code must not land in it. The `ee_fw-lib` library as a whole is **not** to be
vendored — only the SPI flash driver is to be cherry-picked, and the user's basis for that is
authorship: the template is heavily derived from `App/Src/MX25R80.c`, legacy code the user
hand-wrote for personal use.

**Measured — the cherry-pick is genuinely clean.** `templates/modules/spi_flash_nvm/`
(`mx25r80.h` 205 lines + `mx25r80.c` 420) has exactly **two** `ee_fw-lib` dependencies, both in
trivial positions:

| Dependency | Where | Severance |
|---|---|---|
| `sh/sh_err.h` | the `sh_err_t` return type on all eight prototypes | swap for a local error type, or plain `int` — this layer sits *below* the nvmparams driver glue and needs no `nvm_error_t` |
| `sh/sh_log.h` | logging inside `mx25r80.c` | map onto I11's `NVM_LOG_ERROR` shim pattern, or drop the calls |

Everything else is `<string.h>`, `<stdint.h>`, `<stdbool.h>` and `stm32g0xx_hal.h`. Severing
those two headers is required for the module to build standalone **and** removes the last
textual tie to the work library, so the technical and the provenance requirement are the same
edit.

**Practical hygiene to apply when the file lands:** strip work-specific identifiers, naming and
comments along with the two includes; keep the file's header comment honest about its
derivation from the user's own legacy driver; and confirm nothing work-specific rides along in
the retained TODO markers.

**API surface confirms the intended layering.** The template exposes `x_mx25r80_init`, `_read`,
**`_write_page`**, `_erase_sector`, `_erase_chip`, `b_..._is_busy`, `_read_status`,
`_write_status`. The name `write_page` puts the 256-byte page constraint at the chip layer's
boundary, meaning the caller splits — which is exactly the split I3 proposes between the thin
nvmparams glue and the chip driver beneath it.

---

## Wish-list details

### W1 — Wear levelling

**Status:** 🔵 — phase 2.

**Block-initialisation model (user, 2026-08-17).** On first-time pool init, **every** wear
block is written with a copy of the empty pool, all starting at write count 1. This removes the
blank-block exception from I5's selection rule entirely.

Better than the blank-exception alternative for a second reason the user did not raise: after
first init, every block always carries a valid signature, so **"invalid" unambiguously means
"damaged"** rather than "not yet used." The blank-exception scheme conflates those two, and they
want opposite responses — prefer an unused block, avoid a damaged one. Cost is N erase/program
cycles once per device lifetime, which is nothing.

**Tie-breaking.** Recommend **lowest block index (lowest address)**, purely for determinism:
ties only occur between equally-worn blocks so any rule is correct, and a predictable one is
verifiable on the bench.

Two properties that make the scheme provably clean:

- **The read-side tie is self-resolving.** Commit always writes `live_count + 1`, strictly
  greater than any existing count, so the maximum is unique after the very first commit. The
  only tie at the top is at first init, when all copies are identical and the choice cannot
  matter.
- **Rotation is even without extra bookkeeping.** Starting from all-equal counts, "write to the
  lowest, ties by index" walks the blocks in order and then cycles.

**One failure mode to design for in this phase:** a block whose header does not validate has no
usable write count. Treated naively as count 0, it becomes the permanent lowest and every commit
targets the damaged sector. The write-target rule therefore needs to prefer valid blocks and
treat invalid ones as last resort, with a read-back verify after write to detect a sector that
has genuinely failed.

**Terminology note:** the plan uses *block* for the wear unit and *allocation unit* for the
device sector. The user's shorthand "sector" for the wear unit collides with the device sense —
worth keeping the two words distinct in code and comments.

**Phase-1 consequence, small but real:** structure the format path as a function taking a block
index, with first-init looping over blocks. In phase 1 the loop runs once. Without it, phase 3
has to restructure init rather than extend it — the same foresight as I5's scan helper.

Deliberately unplanned in detail. What phase 1 must preserve so this stays an addition rather
than a refactor: core-computed effective addresses (LOCKED), `u32_write_count` as a monotonic
sequence number (S5), a real CRC or at least the plumbing for one (D10), block discovery
isolated in a function (I5), config fields reserved (I6), and stride computed against the
native block size (S2). Undecided and explicitly not being decided now: rotation policy,
default block count, rotate-every-commit versus threshold, and torn-write recovery.

### W2 — RTC backup-RAM driver

**Status:** 🔵 — family-dependent, not viable on G0 (20 bytes, see LOCKED CONTEXT). Viable on
parts with real backup SRAM. Worth having as a fourth example eventually because it is the
one backend with no erase concept at all, which is a useful stress on the interface.

### W3 — Separate erase callback

**Status:** 🔵. For devices where erase is expensive enough to want scheduling separately from
programming. Phase 1's contract is deliberately "write these bytes, whatever it takes".

### W4 — Program-page distinct from erase-unit

**Status:** 🔵. Flash has one granularity; some EEPROM and SPI parts have a program-page size
smaller than the erase unit. Phase 1 ships one value — name the field for what the core uses
it for (stride rounding) so a second can be added later without a rename.

### W5 — Full correctness review

**Status:** 🔵 — phase 2, by explicit user decision. The user is confident in the core as it
stands; it has proven itself across many applications. Known uncovered corner cases such as
pool size exceeding device sector size are acknowledged and are not an immediate concern.
Some review happens incidentally in phase 1 where the new interface forces it (I9, I10).

### W6 — Generated name-string table

**Status:** 🔵 — falls out free if D8 option (b) is chosen. Lets `x_nvm_list()` and the
automation console print `NVM_PARAM_CYCLE_A_REPEAT` instead of `0x101`, which matters for the
HIL suite's readability.

### W7 — Schema/format version object

**Status:** 🔵. Originally proposed after the 2026-08-02 foreign-pool incident, and proposed
then as **application-side**. That argument may now flip: with the module vendored and IDs
adopter-owned, the core is arguably the right owner, because the core is what has to decide
"this pool is not mine, reformat it." Flagged as a reversal of a prior decision rather than
slipped in — it needs a deliberate look when phase 2 opens. Requires one reserved ID from
D9's range.

### W8 — Two-tier commit window

**Status:** 🔵 — phase 2, opt-in.

Considered and deliberately deferred by the user. On top of the phase-1 inactivity timeout
(I14), a second and longer timeout measuring time since the **first** uncommitted write, which
forces a commit when it expires — bounding the worst case without giving up the write-cycle
savings that debounce provides. Opt-in, so projects that want today's behaviour keep it by
leaving the second threshold unset (D6).

Phase 1 costs nothing to enable this: `nvm_pool_t` is never serialised, so a second counter
field can be added later; `v_nvm_commit_timer_tick()` ticks both without a signature change;
and the second threshold gets its own predicate rather than overloading the first. The one
piece of foresight owed is naming — see I14.

---

## Global notes

**Phase-1 exit criteria**, as currently understood: the core has C-library-only dependencies;
no built-in drivers; `nvmparams.h` holds no application-specific declarations; the adopter
declares IDs through a documented mechanism with static enforcement of the reserved range; STM
flash and RAM example drivers exist and work; the HIL suite covers the core including error
paths; and the module is in Skeleton and LED_Strip as well as SwitchTester.

**T2's README is part of "shipped"** (user) — not an implementation blocker, so it need not
gate coding, but phase 1 is not complete without it.

**The two forward-compat commitments** that make phase 2 cheap are S5 and D10's plumbing.
Together they mean phase 1's on-media format is already phase 2's, so no adopter who ships on
phase 1 faces a migration.

**Plan status: 30 🟢 · 1 🟡 · 3 🔴 · 8 🔵.**

**The design is complete.** Every Design and Semantics row is locked — D1–D12 and S1–S5 between
them fix the driver interface, the behaviour a driver author must assume, the header split, the
ID space, and init's policy. Nothing about the module's shape is still open.

**What is not green is work, not decisions.** T4 (🟡) is the provenance boundary, recorded
rather than debated. I9 and I10 (🔴) are fixes to apply while implementing. T2 and T3 (🔴) are
phased deliverables — see the phase plan near the top.

**Working mode reminder:** the user takes open rows one or two at a time in board order. Do not
batch them, and never silently resolve a 🔴 or 🟡.

