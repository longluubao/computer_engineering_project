#include "sim_logger.h"
#include "sim_clock.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#if defined(_WIN32)
#  include <direct.h>
#  define MKDIR(p) _mkdir(p)
#else
#  include <sys/types.h>
#  include <unistd.h>
#  define MKDIR(p) mkdir((p), 0755)
#endif

#include <pthread.h>

static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;
static FILE *g_csv_frames  = NULL;
static FILE *g_csv_attacks = NULL;
static FILE *g_txt_console = NULL;
static char  g_out_dir[512];
static char  g_scenario[128];
static uint64_t g_seed;
static SimLogLevel g_console_level = SIM_LOG_INFO;

static void ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) return;
    (void)MKDIR(path);
}

static const char *level_name(SimLogLevel lvl)
{
    switch (lvl) {
        case SIM_LOG_TRACE: return "TRACE";
        case SIM_LOG_DEBUG: return "DEBUG";
        case SIM_LOG_INFO:  return "INFO ";
        case SIM_LOG_WARN:  return "WARN ";
        case SIM_LOG_ERROR: return "ERROR";
        default:            return "?????";
    }
}

void sim_utc_timestamp(char *out, size_t sz)
{
    time_t now = time(NULL);
    struct tm g;
#ifdef _WIN32
    gmtime_s(&g, &now);
#else
    gmtime_r(&now, &g);
#endif
    strftime(out, sz, "%Y%m%dT%H%M%SZ", &g);
}

void sim_logger_init(const SimLoggerCfg *cfg)
{
    pthread_mutex_lock(&g_mtx);
    strncpy(g_out_dir,  cfg->out_dir,  sizeof(g_out_dir) - 1);
    strncpy(g_scenario, cfg->scenario, sizeof(g_scenario) - 1);
    g_seed = cfg->seed;
    g_console_level = cfg->console_level;
    ensure_dir(cfg->out_dir);

    char path[1024];

    snprintf(path, sizeof(path), "%s/%s_frames.csv", cfg->out_dir, cfg->scenario);
    g_csv_frames = fopen(path, "w");
    if (g_csv_frames) {
        fprintf(g_csv_frames,
            "seq,tx_ns,rx_ns,e2e_ns,secoc_auth_ns,secoc_verify_ns,"
            "cantp_ns,bus_ns,pdu_bytes,auth_bytes,fragments,signal_id,"
            "bus_id,asil,deadline_class,outcome,attack_kind,protected_mode\n");
    }

    snprintf(path, sizeof(path), "%s/%s_attacks.csv", cfg->out_dir, cfg->scenario);
    g_csv_attacks = fopen(path, "w");
    if (g_csv_attacks) {
        fprintf(g_csv_attacks,
            "sim_ns,signal_id,bus_id,attack_kind,detected,extra,note\n");
    }

    snprintf(path, sizeof(path), "%s/%s_console.log", cfg->out_dir, cfg->scenario);
    g_txt_console = fopen(path, "w");

    pthread_mutex_unlock(&g_mtx);

    sim_log(SIM_LOG_INFO,
            "logger initialised: scenario=%s seed=%llu out=%s git=%s",
            cfg->scenario, (unsigned long long)cfg->seed, cfg->out_dir,
            cfg->git_hash ? cfg->git_hash : "(unknown)");
}

void sim_logger_shutdown(void)
{
    pthread_mutex_lock(&g_mtx);
    if (g_csv_frames)  { fclose(g_csv_frames);  g_csv_frames = NULL;  }
    if (g_csv_attacks) { fclose(g_csv_attacks); g_csv_attacks = NULL; }
    if (g_txt_console) { fclose(g_txt_console); g_txt_console = NULL; }
    pthread_mutex_unlock(&g_mtx);
}

void sim_log(SimLogLevel lvl, const char *fmt, ...)
{
    if (lvl < g_console_level) return;

    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    uint64_t t_us = sim_now_us();

    pthread_mutex_lock(&g_mtx);
    fprintf(stderr, "[%10llu us][%s] %s\n",
            (unsigned long long)t_us, level_name(lvl), buf);
    if (g_txt_console) {
        fprintf(g_txt_console, "[%10llu us][%s] %s\n",
                (unsigned long long)t_us, level_name(lvl), buf);
        fflush(g_txt_console);
    }
    pthread_mutex_unlock(&g_mtx);
}

void sim_log_frame(const SimFrameRecord *r)
{
    pthread_mutex_lock(&g_mtx);
    if (g_csv_frames) {
        uint64_t e2e = (r->rx_ns > r->tx_ns) ? (r->rx_ns - r->tx_ns) : 0;
        fprintf(g_csv_frames,
            "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
            (unsigned long long)r->seq,
            (unsigned long long)r->tx_ns,
            (unsigned long long)r->rx_ns,
            (unsigned long long)e2e,
            (unsigned long long)r->secoc_auth_ns,
            (unsigned long long)r->secoc_verify_ns,
            (unsigned long long)r->cantp_ns,
            (unsigned long long)r->bus_ns,
            r->pdu_bytes, r->auth_bytes, r->fragments,
            r->signal_id, r->bus_id, r->asil, r->deadline_class,
            r->outcome, r->attack_kind, r->protected_mode);
    }
    pthread_mutex_unlock(&g_mtx);
}

void sim_log_attack(const SimAttackEvent *evt)
{
    pthread_mutex_lock(&g_mtx);
    if (g_csv_attacks) {
        fprintf(g_csv_attacks, "%llu,%u,%u,%u,%u,%u,\"%s\"\n",
            (unsigned long long)evt->sim_ns, evt->signal_id,
            evt->bus_id, evt->attack_kind, evt->detected, evt->extra,
            evt->note ? evt->note : "");
    }
    pthread_mutex_unlock(&g_mtx);
}

void sim_logger_flush_summary(const char *summary_json)
{
    if (!summary_json) return;
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s_summary.json", g_out_dir, g_scenario);
    FILE *fp = fopen(path, "w");
    if (fp) {
        fputs(summary_json, fp);
        fclose(fp);
    }
}
