"""
acon.py -- host-side driver for the automation console (acon).

Project-neutral: speaks the SCRIPT-mode protocol described in
Docs/acon-protocol.md. Enter on 0xDA, leave on 0xA5, send one command per
CR-terminated line, read one framed response. A project's own ops are just
strings passed to command(); nothing here knows any domain opcode.

The point of this module is that a test (or any host tool) never touches the
wire format directly. It calls `command("V")` and gets back a Frame it can
assert on.

Two protocol rules are honoured here because getting them wrong is how a host
desynchronises:

  * Dispatch is by SIGIL, never by position. Any line may be an async event
    ('*') or ignorable noise ('#'); those are routed aside and the read
    continues, so an event arriving in the window between "host commits to
    send" and "device answers" cannot be mistaken for the response.

  * A response is ONE line unless its header carries K<n>, in which case
    exactly n '+' payload lines follow. Reading a declared count -- rather than
    scanning for a terminator -- is what makes a truncated frame detectable.

Command-line use (one-shot):

    python acon.py --port COM3 [--baud 921600] [--trace] V L "@,1,2,3"

enters the console, sends each command, prints each reply, and leaves.
"""

import time

try:
    import serial
except ImportError:                                     # pragma: no cover
    raise SystemExit("pyserial is required:  python -m pip install pyserial")


ENTER_SENTINEL = 0xDA
EXIT_SENTINEL  = 0xA5
CANCEL         = 0x03           # Ctrl-C, quit alias

# Flow control for streaming ops that honour it. Anywhere else they are
# ordinary bytes and would be read as an opcode.
XON            = 0x11
XOFF           = 0x13

SIG_OK      = '='
SIG_ERR     = '!'
SIG_PAYLOAD = '+'
SIG_EVENT   = '*'
SIG_NOISE   = '#'

DEFAULT_BAUD = 921600


class ProtocolError(Exception):
    """The device said something the grammar does not allow."""


class Frame:
    """One parsed device->host response.

    Attributes
      sigil    '=' or '!'
      op       the echoed opcode, in caret notation for control characters
      ok       True for '=', False for '!'
      code     error mnemonic (UNK / ARG / RNG / OVF / DUP / TMO, or a project's
               own domain code), None on success frames
      tokens   {KEY: int} for fields matching <KEY><hex>
      fields   every field after the opcode, unparsed, in order. V's identity
               response uses free text rather than tokens, so this is where
               that lands.
      payload  the '+' lines, without their sigil, when the header declared K<n>
      raw      the header line as received
    """

    def __init__(self, line, payload=None):
        self.raw = line
        self.payload = payload or []

        if not line or line[0] not in (SIG_OK, SIG_ERR):
            raise ProtocolError("not a response frame: %r" % line)

        self.sigil = line[0]
        self.ok = (self.sigil == SIG_OK)

        parts = line[1:].split(',')
        self.op = parts[0]
        rest = parts[1:]

        # Grammar: '!<op>,<CODE>[,<tok>]...' -- on an error frame the first
        # field is always the code, never a token.
        self.code = None
        if not self.ok and rest:
            self.code = rest[0]
            rest = rest[1:]

        self.fields = list(rest)
        self.tokens = {}
        for field in rest:
            if len(field) >= 2 and field[0].isalpha() and field[0].isupper():
                try:
                    self.tokens[field[0]] = int(field[1:], 16)
                except ValueError:
                    pass                # free text (V's identity strings)

    def __repr__(self):
        return "<Frame %s>" % self.raw


class AutomationConsole:
    """Session against one device's automation console over a serial port."""

    def __init__(self, port, baud=DEFAULT_BAUD, timeout=1.0, trace=False):
        self.port_name = port
        self.baud = baud
        self.timeout = timeout
        self.trace = trace
        self.ser = None
        self.events = []            # '*' frames seen, in arrival order
        self.noise = []             # '#' lines seen

    # ---------------------------------------------------------------- port --

    def open(self):
        self.ser = serial.Serial(self.port_name, self.baud, timeout=self.timeout)
        time.sleep(0.15)
        self.drain()
        return self

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    def __enter__(self):
        return self.open()

    def __exit__(self, *exc):
        self.close()

    def drain(self):
        """Discard anything already buffered. Used between test phases so one
        test's leftovers cannot be read as the next one's response."""
        self.ser.reset_input_buffer()
        time.sleep(0.05)
        while self.ser.in_waiting:
            self.ser.read(self.ser.in_waiting)
            time.sleep(0.05)

    # ------------------------------------------------------------- raw i/o --

    def write_raw(self, data):
        if isinstance(data, str):
            data = data.encode('ascii')
        if self.trace:
            print("    >> %r" % data)
        self.ser.write(data)
        self.ser.flush()

    def read_line(self):
        """One line, CR/LF stripped. Returns None on timeout."""
        raw = self.ser.readline()
        if not raw:
            return None
        line = raw.decode('ascii', errors='replace').rstrip('\r\n')
        if self.trace and line:
            print("    << %r" % line)
        return line

    # ------------------------------------------------------------- framing --

    def read_frame(self, timeout=None):
        """Read until a response frame arrives, routing events and noise aside.

        Returns None if nothing arrives before the deadline -- which is a
        legitimate result, since 'the device must stay silent' is something
        tests assert.
        """
        deadline = time.time() + (timeout if timeout is not None else self.timeout * 3)

        while time.time() < deadline:
            line = self.read_line()
            if line is None or line == '':
                continue

            sigil = line[0]

            if sigil == SIG_EVENT:
                self.events.append(line)
                continue
            if sigil == SIG_NOISE:
                self.noise.append(line)
                continue
            if sigil == SIG_PAYLOAD:
                # A payload line with no header preceding it: the host lost
                # sync, or the device emitted a frame it did not declare.
                raise ProtocolError("orphan payload line: %r" % line)
            if sigil not in (SIG_OK, SIG_ERR):
                raise ProtocolError("unknown sigil in %r" % line)

            frame = Frame(line)

            # K<n> in the header means exactly n '+' lines follow.
            count = frame.tokens.get('K')
            if count:
                payload = []
                for _ in range(count):
                    nxt = self.read_line()
                    if nxt is None:
                        raise ProtocolError(
                            "declared K%d but only %d payload lines arrived"
                            % (count, len(payload)))
                    if not nxt.startswith(SIG_PAYLOAD):
                        raise ProtocolError("expected '+' payload, got %r" % nxt)
                    payload.append(nxt[1:])
                frame.payload = payload

            return frame

        return None

    def stream(self, text, duration_s, cancel_after=None, slack=1.0):
        """Run a two-frame streaming op and collect what it streams.

        A streaming op answers immediately with an ack, emits '*' events, then
        answers again with a terminator -- so it cannot be driven with
        command(), which reads exactly one frame.

        text: the full command line (no CR). duration_s: how long the op is
        expected to run. cancel_after: seconds to wait before sending a cancel
        byte, or None to let the device's own timeout end it. slack: extra
        seconds to wait for the terminator.

        Returns (ack, events, terminator). `events` are the '*' lines emitted
        during THIS call, not the session-long self.events list.
        """
        mark = len(self.events)
        self.write_raw(text + '\r')

        ack = self.read_frame()
        if ack is None:
            raise ProtocolError("no ack from %r" % text)
        if not ack.ok:
            return ack, [], None            # refused; nothing streamed

        if cancel_after is not None:
            time.sleep(cancel_after)
            self.write_raw('x')             # 'any byte' -- consumed as the cancel

        term = self.read_frame(timeout=duration_s + slack)
        return ack, self.events[mark:], term

    def suspend(self):
        """XOFF -- pause a streaming op's emission without leaving it."""
        self.write_raw(bytes([XOFF]))

    def resume(self):
        """XON -- resume emission."""
        self.write_raw(bytes([XON]))

    def expect_silence(self, seconds=0.5):
        """Assert the device says nothing for a while. Returns what it said
        anyway, so a failure can report the offending line."""
        end = time.time() + seconds
        heard = []
        while time.time() < end:
            line = self.read_line()
            if line:
                heard.append(line)
        return heard

    # ------------------------------------------------------------ protocol --

    def command(self, text, timeout=None):
        """Send one command line and return its response frame."""
        self.write_raw(text + '\r')
        return self.read_frame(timeout=timeout)

    def enter(self):
        """Enter the console in SCRIPT mode. Returns the ready frame."""
        self.write_raw(bytes([ENTER_SENTINEL]))
        frame = self.read_frame()
        if frame is None or frame.op != '~':
            raise ProtocolError("no session banner after ENTER, got %r" % frame)
        return frame

    def leave(self, how='sentinel'):
        """Leave the console. `how` is 'sentinel', 'quit' or 'cancel' -- all
        three are supposed to produce the same BYE frame."""
        if how == 'sentinel':
            self.write_raw(bytes([EXIT_SENTINEL]))
        elif how == 'quit':
            self.write_raw('Q\r')
        elif how == 'cancel':
            self.write_raw(bytes([CANCEL]) + b'\r')
        else:
            raise ValueError(how)
        return self.read_frame()


def main(argv=None):
    """One-shot CLI: enter, send each command, print each reply, leave."""
    import argparse

    ap = argparse.ArgumentParser(description="Send commands to an automation console.")
    ap.add_argument('--port', required=True, help="serial port, e.g. COM3")
    ap.add_argument('--baud', type=int, default=DEFAULT_BAUD)
    ap.add_argument('--timeout', type=float, default=1.0, help="per-line read timeout, s")
    ap.add_argument('--trace', action='store_true', help="dump raw wire traffic")
    ap.add_argument('commands', nargs='*', help="command lines, e.g. V  L  \"@,1,2\"")
    args = ap.parse_args(argv)

    with AutomationConsole(args.port, args.baud, args.timeout, args.trace) as con:
        print(con.enter().raw)
        failed = 0
        try:
            for text in args.commands:
                frame = con.command(text)
                if frame is None:
                    print("%-12s (no reply)" % text)
                    failed += 1
                    continue
                print("%-12s %s" % (text, frame.raw))
                for line in frame.payload:
                    print("%-12s +%s" % ('', line))
                if not frame.ok:
                    failed += 1
        finally:
            bye = con.leave()
            print(bye.raw if bye else "(no BYE)")
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())
