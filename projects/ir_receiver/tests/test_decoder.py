"""Exercise the real C decoder via a host-built shared library.

Usage: python test_decoder.py path/to/nec_decoder_test.dll
Build nec_decoder.c as a shared library exporting nec_decode (no ESP-IDF needed).
"""
import ctypes as c
import sys


class Pulse(c.Structure):
    _fields_ = [("duration_us", c.c_uint32), ("mark", c.c_bool)]


class Frame(c.Structure):
    _fields_ = [("raw", c.c_uint32), ("address", c.c_uint16),
                ("command", c.c_uint8), ("extended", c.c_bool)]


lib = c.CDLL(sys.argv[1])
lib.nec_decode.argtypes = [c.POINTER(Pulse), c.c_size_t, c.POINTER(Frame)]
lib.nec_decode.restype = c.c_int


def decode(pulses):
    frame = Frame()
    array = (Pulse * len(pulses))(*(Pulse(*p) for p in pulses))
    return lib.nec_decode(array, len(array), c.byref(frame)), frame


def encode(raw):
    pulses = [(9000, True), (4500, False)]
    for bit in range(32):
        pulses.extend([(560, True), (1690 if raw & (1 << bit) else 560, False)])
    return pulses + [(560, True)]


for address in (0, 0x12, 0xff):
    for command in range(256):
        raw = address | ((address ^ 0xff) << 8) | (command << 16) | ((command ^ 0xff) << 24)
        result, frame = decode(encode(raw))
        assert result == 1 and frame.raw == raw
        assert frame.address == address and frame.command == command and not frame.extended

result, frame = decode(encode(0xCB341234))
assert result == 1 and frame.extended and frame.address == 0x1234 and frame.command == 0x34
assert decode([(12000, False)] + encode(0xBA45FF00) + [(12000, False)])[0] == 1
assert decode([(9000, True), (2250, False), (560, True)])[0] == 2
assert decode([(9000, True), (2250, False), (560, True), (12000, False)])[0] == 2
assert decode(encode(0xBB45FF00))[0] == 0  # Bad command inverse.
assert decode(encode(0xBA45FF00)[:-1])[0] == 0  # Missing stop mark.
assert decode(encode(0xBA45FF00) + [(560, True)])[0] == 0
bad = encode(0xBA45FF00)
bad[2] = (560, False)
assert decode(bad)[0] == 0  # Wrong polarity.
bad = encode(0xBA45FF00)
bad[3] = (3000, False)
assert decode(bad)[0] == 0  # Invalid bit timing.
assert decode([(round(d * 1.2), m) for d, m in encode(0xBA45FF00)])[0] == 1
assert decode([(round(d * 1.4), m) for d, m in encode(0xBA45FF00)])[0] == 0
for count in range(67):
    assert decode(encode(0xBA45FF00)[:count])[0] == 0
print("Passed: 768 NEC address/command combinations, extended NEC, repeats, idle, tolerance, and malformed/truncated frames.")
