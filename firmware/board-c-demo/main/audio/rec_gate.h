#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Edge + dwell gate for GPIO9 / hand-held record.
 * Start and stop are rising/falling edges with a minimum capture and a
 * cooldown so ToF chatter cannot restart or emit BUSY every scan.
 * A rising edge while ASR is busy cancels the upload instead of failing.
 */

enum {
    VENTURED_REC_MIN_MS = 350U,
    VENTURED_REC_COOLDOWN_MS = 400U,
};

typedef enum {
    VENTURED_REC_GATE_NONE = 0,
    VENTURED_REC_GATE_START,
    VENTURED_REC_GATE_STOP,
    VENTURED_REC_GATE_CANCEL_ASR,
} ventured_rec_gate_action_t;

typedef struct {
    bool prev_want;
    bool recording;
    bool hold_off;
    uint32_t rec_start_ms;
    uint32_t last_stop_ms;
} ventured_rec_gate_t;

void ventured_rec_gate_init(ventured_rec_gate_t *gate);
void ventured_rec_gate_abort(ventured_rec_gate_t *gate, uint32_t now_ms);
ventured_rec_gate_action_t ventured_rec_gate_update(ventured_rec_gate_t *gate, bool want,
                                                    bool asr_busy, bool force_stop, uint32_t now_ms);
