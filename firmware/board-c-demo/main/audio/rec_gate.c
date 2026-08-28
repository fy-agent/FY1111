#include "rec_gate.h"

#include <stddef.h>

void ventured_rec_gate_init(ventured_rec_gate_t *gate) {
    if (gate == NULL) return;
    gate->prev_want = false;
    gate->recording = false;
    gate->hold_off = false;
    gate->rec_start_ms = 0;
    gate->last_stop_ms = 0;
}

void ventured_rec_gate_abort(ventured_rec_gate_t *gate, uint32_t now_ms) {
    if (gate == NULL) return;
    gate->recording = false;
    gate->hold_off = true;
    gate->last_stop_ms = now_ms;
}

ventured_rec_gate_action_t ventured_rec_gate_update(ventured_rec_gate_t *gate, bool want,
                                                    bool asr_busy, bool force_stop, uint32_t now_ms) {
    if (gate == NULL) return VENTURED_REC_GATE_NONE;
    bool rising = want && !gate->prev_want;
    gate->prev_want = want;

    if (gate->hold_off) {
        if (!want) gate->hold_off = false;
        else return VENTURED_REC_GATE_NONE;
    }

    if (gate->recording) {
        if (force_stop || (!want && (now_ms - gate->rec_start_ms) >= VENTURED_REC_MIN_MS)) {
            gate->recording = false;
            gate->last_stop_ms = now_ms;
            if (force_stop) gate->hold_off = true;
            return VENTURED_REC_GATE_STOP;
        }
        return VENTURED_REC_GATE_NONE;
    }

    if (!want) return VENTURED_REC_GATE_NONE;
    if (asr_busy) {
        if (rising) {
            gate->hold_off = true;
            return VENTURED_REC_GATE_CANCEL_ASR;
        }
        return VENTURED_REC_GATE_NONE;
    }
    if (!rising) return VENTURED_REC_GATE_NONE;
    if (gate->last_stop_ms != 0U && (now_ms - gate->last_stop_ms) < VENTURED_REC_COOLDOWN_MS) {
        return VENTURED_REC_GATE_NONE;
    }
    gate->recording = true;
    gate->rec_start_ms = now_ms;
    return VENTURED_REC_GATE_START;
}
