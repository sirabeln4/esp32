#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t duration_us;
    bool mark; // Active-low output from the demodulating receiver.
} ir_pulse_t;

typedef enum { NEC_INVALID, NEC_FRAME, NEC_REPEAT } nec_result_t;
typedef struct {
    uint32_t raw;
    uint16_t address;
    uint8_t command;
    bool extended;
} nec_frame_t;

nec_result_t nec_decode(const ir_pulse_t *pulses, size_t count, nec_frame_t *frame);
