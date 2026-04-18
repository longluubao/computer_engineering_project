#include "sim_signals.h"

#include <stddef.h>

/*
 * A representative automotive signal catalogue covering the ASIL range
 * and both short / medium / large payload sizes. This is the set used
 * by the thesis scenarios; scenarios pick a subset by id.
 */
static const SimSignalDef g_catalogue[] = {
    /* id,  name,                     cycle, bytes, dl, asil,      bus,                 tp   */
    { 0x01, "BrakeCmd",                  5,    8,  1, SIM_ASIL_D, SIM_BUS_CAN_FD,       false },
    { 0x02, "SteeringTorque",            5,   16,  1, SIM_ASIL_D, SIM_BUS_CAN_FD,       false },
    { 0x03, "AirbagTrigger",             5,    4,  1, SIM_ASIL_D, SIM_BUS_CAN_20,       false },
    { 0x04, "Throttle",                 10,    8,  2, SIM_ASIL_C, SIM_BUS_CAN_FD,       false },
    { 0x05, "AbsWheelSpeed",            10,   32,  2, SIM_ASIL_C, SIM_BUS_CAN_FD,       false },
    { 0x06, "InverterTorque",           10,   16,  2, SIM_ASIL_C, SIM_BUS_FLEXRAY,      false },
    { 0x07, "BodyMotion",               20,   24,  3, SIM_ASIL_B, SIM_BUS_CAN_FD,       false },
    { 0x08, "DoorLockState",            20,    8,  3, SIM_ASIL_B, SIM_BUS_CAN_20,       false },
    { 0x09, "Speedometer",              50,   16,  4, SIM_ASIL_B, SIM_BUS_CAN_FD,       false },
    { 0x0A, "HVAC",                     50,    8,  4, SIM_ASIL_QM, SIM_BUS_CAN_20,      false },
    { 0x0B, "InfotainmentGateway",     100,  512,  5, SIM_ASIL_QM, SIM_BUS_ETH_100,     true  },
    { 0x0C, "ADAS_Camera_Meta",         20,  256,  3, SIM_ASIL_C, SIM_BUS_ETH_100,      true  },
    { 0x0D, "V2X_AuthenticatedMsg",    100, 1024,  5, SIM_ASIL_B, SIM_BUS_ETH_1000,     true  },
    { 0x0E, "OBD_Diagnostic",          500,  128,  6, SIM_ASIL_QM, SIM_BUS_ETH_100,     true  },
};

const SimSignalDef *sim_signal_catalogue(size_t *out_count)
{
    if (out_count) *out_count = sizeof(g_catalogue) / sizeof(g_catalogue[0]);
    return g_catalogue;
}

const SimSignalDef *sim_signal_find(uint16_t id)
{
    size_t n = sizeof(g_catalogue) / sizeof(g_catalogue[0]);
    for (size_t i = 0; i < n; ++i) {
        if (g_catalogue[i].id == id) return &g_catalogue[i];
    }
    return NULL;
}
