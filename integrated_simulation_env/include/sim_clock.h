#ifndef ISE_SIM_CLOCK_H
#define ISE_SIM_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Monotonic clock abstraction used by every ISE component.
 *
 * - sim_now_ns() returns nanoseconds since sim_clock_init(), using
 *   CLOCK_MONOTONIC on POSIX and QueryPerformanceCounter on Windows.
 * - sim_sleep_ns() yields without drifting; short waits spin, longer
 *   waits use nanosleep/SleepEx.
 * - The "deterministic" mode advances a simulated clock in fixed
 *   increments; useful for regression / CI where wall-clock jitter
 *   would obscure results.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SIM_CLOCK_WALLTIME = 0,   /* real monotonic clock (default)  */
    SIM_CLOCK_DETERMINISTIC   /* discrete tick clock             */
} SimClockMode;

void     sim_clock_init(SimClockMode mode, uint64_t det_tick_ns);
uint64_t sim_now_ns(void);
uint64_t sim_now_us(void);
void     sim_sleep_ns(uint64_t ns);
void     sim_clock_advance_det_ns(uint64_t ns); /* only in DETERMINISTIC mode */
bool     sim_clock_is_deterministic(void);

static inline double sim_ns_to_us(uint64_t ns) { return (double)ns / 1000.0; }
static inline double sim_ns_to_ms(uint64_t ns) { return (double)ns / 1000000.0; }

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_CLOCK_H */
