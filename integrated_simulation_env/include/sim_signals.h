#ifndef ISE_SIM_SIGNALS_H
#define ISE_SIM_SIGNALS_H

#include <stdint.h>
#include <stdbool.h>
#include "sim_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Catalogue of automotive signals used by the scenarios. Each signal
 * carries its ASIL, cycle time and deadline class. See
 * docs/SIGNAL_CATALOGUE.md for full traceability.
 */

typedef enum {
    SIM_ASIL_QM = 0,
    SIM_ASIL_A  = 1,
    SIM_ASIL_B  = 2,
    SIM_ASIL_C  = 3,
    SIM_ASIL_D  = 4
} SimAsil;

typedef struct {
    uint16_t   id;
    const char *name;
    uint32_t   cycle_ms;       /* nominal transmit period */
    uint32_t   payload_bytes;  /* application payload size */
    uint8_t    deadline_class; /* 1..6 */
    SimAsil    asil;
    SimBusKind preferred_bus;
    bool       tp_mode;        /* true → use SecOC_TpTransmit */
} SimSignalDef;

/* The catalogue is static; exposed for enumeration. */
const SimSignalDef *sim_signal_catalogue(size_t *out_count);

/* Lookup by id (returns NULL if unknown). */
const SimSignalDef *sim_signal_find(uint16_t id);

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_SIGNALS_H */
