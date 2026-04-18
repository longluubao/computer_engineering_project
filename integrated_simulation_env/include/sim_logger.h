#ifndef ISE_SIM_LOGGER_H
#define ISE_SIM_LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Thread-safe logger that writes:
 *   - one CSV file with every frame (raw, for thesis appendices),
 *   - one JSON file with the final aggregate summary,
 *   - one text log ("console") for human-readable progress,
 *   - one attack-events CSV when attack scenarios are active.
 *
 * Every row carries run metadata: scenario, seed, git_hash, utc_start.
 */

typedef enum {
    SIM_LOG_TRACE = 0,
    SIM_LOG_DEBUG,
    SIM_LOG_INFO,
    SIM_LOG_WARN,
    SIM_LOG_ERROR
} SimLogLevel;

typedef struct {
    const char *out_dir;       /* output directory (created if missing) */
    const char *scenario;      /* scenario short-name (used in file names) */
    uint64_t    seed;
    const char *git_hash;      /* optional, NULL-safe */
    SimLogLevel console_level;
} SimLoggerCfg;

void sim_logger_init(const SimLoggerCfg *cfg);
void sim_logger_shutdown(void);

/* Free-form console logging. */
void sim_log(SimLogLevel lvl, const char *fmt, ...);

/*
 * Per-frame record. Every field is optional (zero = "not applicable").
 * The CSV schema is stable across versions; see docs/RESULTS_FORMAT.md.
 */
typedef struct {
    uint64_t seq;              /* monotonic sequence number              */
    uint64_t tx_ns;            /* application tx timestamp               */
    uint64_t rx_ns;            /* application rx timestamp               */
    uint64_t secoc_auth_ns;    /* Csm signature/MAC generation time      */
    uint64_t secoc_verify_ns;  /* Csm signature/MAC verification time    */
    uint64_t cantp_ns;         /* CAN-TP fragmentation/reassembly time   */
    uint64_t bus_ns;           /* physical bus transit time              */
    uint32_t pdu_bytes;        /* secured PDU total size                 */
    uint32_t auth_bytes;       /* authenticator bytes (MAC or signature) */
    uint32_t fragments;        /* CAN-TP fragment count (1 for IF mode)  */
    uint16_t signal_id;        /* application signal identifier          */
    uint8_t  bus_id;           /* which virtual bus carried it           */
    uint8_t  asil;             /* 0=QM 1=A 2=B 3=C 4=D                   */
    uint8_t  deadline_class;   /* 1..6 (see THESIS_RESEARCH.md)          */
    uint8_t  outcome;          /* 0=OK 1=DEADLINE_MISS 2=DROP 3=VERIFY_FAIL */
    uint8_t  attack_kind;      /* 0=none, else SimAttackKind              */
    uint8_t  protected_mode;   /* 0=HMAC 1=PQC 2=HYBRID                   */
} SimFrameRecord;

void sim_log_frame(const SimFrameRecord *rec);

/* Attack event log (one row per injected attack). */
typedef struct {
    uint64_t sim_ns;
    uint16_t signal_id;
    uint8_t  bus_id;
    uint8_t  attack_kind;
    uint8_t  detected;     /* 1=detected by SecOC, 0=delivered as authentic */
    uint32_t extra;        /* attack-specific payload */
    const char *note;      /* optional string */
} SimAttackEvent;

void sim_log_attack(const SimAttackEvent *evt);

/* Called once at the end of a scenario to dump JSON summary. */
void sim_logger_flush_summary(const char *summary_json);

/* Produce a UTC ISO-8601 timestamp suitable for directory names. */
void sim_utc_timestamp(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_LOGGER_H */
