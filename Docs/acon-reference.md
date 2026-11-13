# Automation console — protocol reference

The **acon** is a machine-facing command console: a second personality on the same UART the
human debug menu uses, entered by a sentinel byte, speaking a line-oriented request/response
protocol instead of a redraw-heavy menu. It is not test-only. It is the interface *for* a
host-side harness, and equally the way a host drives the device in normal operation.

**This document is the project-neutral protocol reference:** how to get a session, the frame
grammar, the builtins every adopter gets, how to drive it from a host, and the gotchas. It
travels with the module. Each adopting project adds its **own** command reference (its
domain ops) on top. See *Documenting a project's ops* at the end.

| Document | Covers |
|---|---|
| [`App/automation_console/README.md`](../App/automation_console/README.md) | The **portable module**: adopting it, its config header, and the command-author API (`v_acon_emit()`, `u8_acon_args()`, …). |
| This file | The **protocol**, from the host's side. |
| [`scripts/hil/acon.py`](../scripts/hil/acon.py) | The **host driver** (Python, pyserial). |
| [SwitchTester `automation-console-plan.md`](https://github.com/n9xmj/SwitchTester/blob/main/Docs/planning/automation-console-plan.md) | The **decision log**: *why* the protocol is shaped this way. |

---

## 1. Getting a session

### Wire

The console shares the debug menu's UART. Port, baud and connector are **project settings**;
a project's own reference states them. Skeleton's defaults:

| | |
|---|---|
| Port | USART2 (the debug-menu console) |
| Baud | **921600** |
| Framing | 8-N-1, no flow control |
| Line ending | Host sends `CR`. `LF` is discarded wherever it appears, so a CRLF host still produces exactly one frame per command. Device replies `CRLF`. |

### Entering and leaving

| Byte | Meaning |
|---|---|
| `0xDA` | **ACON_ENTER**: enter SCRIPT mode. Intercepted before the menu's echo, so the sentinel never appears on the wire and never reaches the menu dispatcher. |
| `0xA5` | **ACON_EXIT**: leave. Recognised mid-line. |
| `Q` | Quit (a normal command; answers `=Q` then `=~,BYE`). |
| `0x03` | Ctrl-C, quit alias. |

Both sentinels have the MS bit set, so neither collides with a printable menu key nor can be
typed by accident from a terminal. They are a bit-complement pair (`0x5A|0x80` and `~0x5A`).

Entering emits a **ready banner** carrying the protocol version; leaving emits a farewell:

```
0xDA  ->  =~,V1                 in, speaking protocol version 1
0xA5  ->  =~,BYE                out
```

`~` is the **session opcode**, used for frames the console itself originates rather than a
command. It sits in a normal opcode position, so a host parses session frames with the same
code it uses for everything else:

| Frame | When |
|---|---|
| `=~,V<n>` | Session opened; `n` is the protocol version (currently 1) |
| `=~,BYE` | Session closed by sentinel, `Q` or Ctrl-C |
| `!~,TMO` | SCRIPT-mode idle timeout |
| `!~,OVF` | Input line exceeded `ACON_LINE_MAX`, or a response frame would have exceeded `ACON_EMIT_MAX` |
| `!~,DUP,E<byte>` | Two table entries claim the same opcode; reported once, at first dispatch |

**Wait for the banner before sending a command.** It is the only positive confirmation that
the sentinel was seen and the menu is no longer holding the wire.

### Two modes

Same dispatcher, same frames. Only the *reader* differs, so a command tried by hand behaves
exactly as it will from a script.

- **SCRIPT**: entered by the `0xDA` sentinel. Raw byte-at-a-time, no echo, idle timeout
  armed, ordinary `printf` output suppressed for the session's duration. This is what a host
  driver uses.
- **HUMAN**: entered from a debug-menu item. Full line editing and echo, because there is an
  operator present. **No idle timeout**: a person is allowed to think.

### Timeouts and limits

These are config knobs (`automation_console_config.h`); a project's reference states its
values. Template defaults:

| Knob | Default | Meaning |
|---|---|---|
| `ACON_IDLE_TIMEOUT_MS` | 15000 | SCRIPT mode only. A host that goes quiet mid-line frees the console rather than wedging the board. |
| `ACON_TX_TIMEOUT_MS` | 100 | Emit-side backstop. |
| `ACON_LINE_MAX` | 512 | Longest accepted input line. |
| `ACON_EMIT_MAX` | 128 | Longest response frame. |
| `ACON_MAX_ARGS` | 6 | Comma fields `u8_acon_args()` will split out. Extra fields are dropped **silently**. |

An idle timeout answers with `!` rather than `=`, because the host did not ask to leave:

```
!~,TMO
```

### Session brackets

`ACON_ON_ENTER()` runs as a session opens, **before** the banner. `ACON_ON_EXIT()` runs on
**every** way out: quit, exit sentinel, and the idle timeout. A project uses them for state a
host session should own and hand back. In particular, anything that must be made safe if a
host dies mid-session (a motor, a heater, an output left driven) belongs in `ACON_ON_EXIT()`,
because the timeout is the path a dead host actually takes.

### While a session is open

The console calls `ACON_PUMP()` (normally `v_app_polling_task()`) on **every spin** of its
reader, so jobs, the periodic tick and the watchdog all keep running. The board is never
"frozen" inside a session. The debug menu's re-entry lock is held for the session's lifetime,
which stops the menu from stealing input.

---

## 2. Frame grammar

```
host   → device    <op>[,<arg>]...<CR>
device → host      <sigil><op>[,<token>]...<CRLF>
```

### Arguments — every number is hex

`b_acon_arg_u32()` is the console's **only** numeric-argument parser, and it is
`strtoul(arg, &end, 16)`. Every numeric argument of every op is therefore **hexadecimal**.
There is no decimal input anywhere, and no per-op exception.

A `0x` prefix is accepted but never required and never emitted. Parsing is **strict and
whole-field**: a trailing character rejects the argument rather than truncating it, so `3x`
is an error, not `3`. An empty field fails too, which is not the same as an omitted trailing
field (ops with optional arguments check the count first).

> **The trap.** Every all-digit decimal literal is also valid hex, so a decimal argument is
> silently accepted as the wrong number, and **nothing will ever warn you**. Typing `115200`
> gives `0x115200` = 1,135,104. Convert before you type.

Replies are hex as well, so the two directions match.

### Sigils — the first byte of every device→host line

| Sigil | Meaning |
|---|---|
| `=` | Command response, success |
| `!` | Command response, failure |
| `+` | Payload continuation line |
| `*` | Async event (emitted only by streaming ops a project defines) |
| `#` | Not protocol (stray output, banners) |

**Dispatch on the sigil, never on position.** Any line may turn out to be noise or an async
event; a host that assumes "the next line is my response" desynchronises the first time it is
wrong.

### Tokens

Fields after the opcode are `<KEY><hexvalue>` with **no separator inside the token**: `M8` is
key `M`, value `0x8`. Values are hex without a `0x` prefix. Some ops also emit bare mnemonic
fields (`PASS`, `BYE`, `TMO`, error codes) or free text (`V`'s identity strings), which carry
no key.

### Multi-line replies

A response is **one line** unless its header carries `K<n>`, in which case exactly *n* `+`
payload lines follow:

```
=L,K4
+@ echo args as CSV (example)
+$ echo raw text (example)
...
```

Declaring the count up front, rather than scanning for a terminator, is what makes a
truncated response *detectable*.

**`K` is reserved for exactly that meaning**, protocol-wide. An op must not use it for any
other count: a host reading a `K` will block waiting for payload lines.

### Errors

```
!<op>,<CODE>[,<extra tokens>]
```

Generic codes, provided by the core (`ACON_ERR_*`):

| Code | Meaning |
|---|---|
| `UNK` | No such opcode |
| `ARG` | Missing or unparseable field |
| `RNG` | Parsed, but outside limits |
| `OVF` | Line or frame too long |
| `DUP` | Op-table conflict, reported once |
| `TMO` | Idle timeout (session frame) |

Projects define their own domain codes (for example `BUSY`, `NVM`, `I2C`) and list them in
their reference.

A control-character opcode is echoed in **caret notation** (Ctrl-C appears as `^C`), so a host
never has to handle a raw control byte in the opcode position. Opcodes are
**case-sensitive**; there is no `toupper()` folding on this path.

---

## 3. Builtins — always present, provided by the module core

A project's op table carries only domain ops; these are guaranteed regardless.

| Op | Syntax | Reply | Notes |
|---|---|---|---|
| `V` | `V` | `=V,<product>,<platform>,<firmware>,<build>` | Identity, from the `ACON_ID_*` config strings. Enough for a host to pin exactly what it is talking to. |
| `L` | `L` | `=L,K<n>` + *n* `+` lines | List every op (builtins and the project's) with its one-line help. |
| `?` | `?` | as `L` | Alias. |
| `Z` | `Z` | `=Z` | No-op. |
| *(space)* | ` ` | `=Z` | No-op. |
| *(bare CR)* | | `=Z` | No-op that answers; the liveness probe. |
| `Q` | `Q` | `=Q` then `=~,BYE` | Quit. |
| `^C` | `0x03` | as `Q` | Quit alias. |

Skeleton's `automation_commands.c` also ships two **example ops** for command authors,
`@,a,b,c` (echo the arguments back as CSV) and `$ <text>` (echo raw text). An adopter
replaces them with its own.

---

## 4. Worked sequences

### Open, identify, list, leave

```
0xDA                =~,V1                   enter SCRIPT mode; wait for the banner
V                   =V,<product>,<platform>,<firmware>,<build>
L                   =L,K<n>                 then n '+' lines
0xA5                =~,BYE
```

### Liveness probe

Send a bare `CR`. A live console answers `=Z`; anything else means you are not in a session.

---

## 5. Driving it from a host

[`scripts/hil/acon.py`](../scripts/hil/acon.py) is the host-side driver (needs `pyserial`).
A test never touches the wire format: it calls `command("...")` and gets back a `Frame` it can
assert on.

```python
from acon import AutomationConsole
with AutomationConsole('COM3', 921600) as con:      # the context manager opens the PORT
    con.enter()                                     # ...entering the SESSION is separate
    frame = con.command('V')
    assert frame.ok
    con.leave()
```

Opening the port and entering the session are deliberately two steps: a test that wants to
observe menu-mode behaviour, or what happens when the sentinel is *not* sent, needs the port
without the session. `enter()` returns the `=~,V1` banner and raises `ProtocolError` if it
does not arrive.

`Frame` carries `sigil`, `op`, `ok`, `code`, `tokens` (a `{KEY: int}` dict), `fields` and
`payload` (the `+` lines). The two desync rules are honoured inside it: **dispatch by sigil**,
and **read a declared `K` count** rather than scanning for a terminator. `read_frame()`
returning `None` on timeout is a legitimate result, not an error.

Other methods: `leave(how=...)` (sentinel, quit or cancel), `expect_silence()`,
`stream(text, ...)` for a two-frame streaming op (ack, `*` events, terminator), and
`suspend()` / `resume()` (XOFF/XON) for ops that honour flow control.

It also runs as a one-shot command-line tool:

```bash
python scripts/hil/acon.py --port COM3 V L "@,1,2,3"
```

It enters, sends each command, prints each reply, and leaves. Add `--trace` to dump the wire
traffic.

---

## 6. Gotchas

- **Close the terminal program first** (Tera Term etc.). It holds the COM port, and the host
  driver will fail to open it.
- **Everything is hex**, in both directions. See *The trap* above.
- **Opcodes are case-sensitive.** `a` is not `A`.
- **`ACON_MAX_ARGS` caps silently.** A command sent more fields than that gets the first N with
  no error. Size it for your widest op.
- **`ACON_LINE_MAX` must stay well below the console's RX ring.** A line longer than the ring
  cannot be received at all, and the loss takes the CR with it, so the failure surfaces
  against the *next* command.
- **`*` frames can arrive at any time a streaming op is running.** Route them by sigil. A
  streaming op answers twice (ack, then terminator); `command()` reads one frame, so use
  `stream()`.
- **Anything a dead host could leave running must be undone in `ACON_ON_EXIT()`.**

---

## Documenting a project's ops

Each adopting project keeps its own `Docs/acon-reference.md` (or a section of its docs) that:

1. States the project's **wire settings** (UART, baud, connector) and its config values where
   they differ from the defaults above.
2. Lists its **domain ops**, one entry each: syntax, reply, error codes, and any safety
   behaviour (e.g. auto-stop timers).
3. Lists its **domain error codes**.
4. Links back here for the protocol rather than repeating it.

The live `L` command is always the authoritative list. If a document and the board disagree,
the board is right.
