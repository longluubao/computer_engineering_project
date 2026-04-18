#include "sim_clock.h"

#include <stdatomic.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#  include <errno.h>
#endif

static SimClockMode g_mode = SIM_CLOCK_WALLTIME;
static uint64_t     g_start_ns;
static uint64_t     g_det_tick_ns;
static _Atomic uint64_t g_det_now_ns = 0;

#ifdef _WIN32
static LARGE_INTEGER g_freq;
static uint64_t wall_now_ns(void)
{
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (uint64_t)((c.QuadPart * 1000000000ULL) / (uint64_t)g_freq.QuadPart);
}
#else
static uint64_t wall_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
#endif

void sim_clock_init(SimClockMode mode, uint64_t det_tick_ns)
{
    g_mode         = mode;
    g_det_tick_ns  = (det_tick_ns > 0) ? det_tick_ns : 1000ULL; /* 1 µs default */
    atomic_store(&g_det_now_ns, 0);

#ifdef _WIN32
    QueryPerformanceFrequency(&g_freq);
#endif
    g_start_ns = wall_now_ns();
}

bool sim_clock_is_deterministic(void)
{
    return g_mode == SIM_CLOCK_DETERMINISTIC;
}

uint64_t sim_now_ns(void)
{
    if (g_mode == SIM_CLOCK_DETERMINISTIC) {
        return atomic_load(&g_det_now_ns);
    }
    return wall_now_ns() - g_start_ns;
}

uint64_t sim_now_us(void)
{
    return sim_now_ns() / 1000ULL;
}

void sim_sleep_ns(uint64_t ns)
{
    if (g_mode == SIM_CLOCK_DETERMINISTIC) {
        sim_clock_advance_det_ns(ns);
        return;
    }
#ifdef _WIN32
    if (ns < 1000000ULL) {
        /* Windows Sleep rounds to millisecond; spin for small waits. */
        uint64_t start = wall_now_ns();
        while (wall_now_ns() - start < ns) { /* spin */ }
    } else {
        Sleep((DWORD)(ns / 1000000ULL));
    }
#else
    struct timespec req;
    req.tv_sec  = (time_t)(ns / 1000000000ULL);
    req.tv_nsec = (long)(ns % 1000000000ULL);
    while (nanosleep(&req, &req) == -1 && errno == EINTR) { }
#endif
}

void sim_clock_advance_det_ns(uint64_t ns)
{
    if (g_mode == SIM_CLOCK_DETERMINISTIC) {
        atomic_fetch_add(&g_det_now_ns, ns);
    }
}
