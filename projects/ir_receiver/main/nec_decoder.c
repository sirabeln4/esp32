#include "nec_decoder.h"

static bool matches(ir_pulse_t pulse, bool mark, uint32_t target)
{
    // Allow receiver distortion and approximately 25% timing tolerance.
    return pulse.mark == mark && pulse.duration_us >= target * 3 / 4 &&
           pulse.duration_us <= target * 5 / 4;
}

nec_result_t nec_decode(const ir_pulse_t *p, size_t count, nec_frame_t *frame)
{
    if (!p || !frame) return NEC_INVALID;
    // RMT may include the idle level preceding a frame.
    while (count && !p->mark) { ++p; --count; }
    if (count < 3 || !matches(p[0], true, 9000)) return NEC_INVALID;
    if (matches(p[1], false, 2250) && matches(p[2], true, 560) &&
        (count == 3 || (count == 4 && !p[3].mark))) {
        return NEC_REPEAT;
    }
    // Leader, 32 mark/space pairs, final mark, optional trailing idle.
    if ((count != 67 && count != 68) || !matches(p[1], false, 4500) ||
        !matches(p[66], true, 560) || (count == 68 && p[67].mark)) {
        return NEC_INVALID;
    }
    uint32_t raw = 0;
    for (unsigned bit = 0; bit < 32; ++bit) {
        if (!matches(p[2 + bit * 2], true, 560)) return NEC_INVALID;
        ir_pulse_t space = p[3 + bit * 2];
        if (matches(space, false, 1690)) raw |= UINT32_C(1) << bit;
        else if (!matches(space, false, 560)) return NEC_INVALID;
    }
    uint8_t address_low = raw;
    uint8_t address_high = raw >> 8;
    uint8_t command = raw >> 16;
    uint8_t command_inverse = raw >> 24;
    if ((uint8_t)(command ^ command_inverse) != 0xff) return NEC_INVALID;
    frame->raw = raw;
    frame->extended = (uint8_t)(address_low ^ address_high) != 0xff;
    frame->address = frame->extended ? (uint16_t)raw : address_low;
    frame->command = command;
    return NEC_FRAME;
}
