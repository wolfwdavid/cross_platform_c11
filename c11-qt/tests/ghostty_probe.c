/* Minimal libghostty init probe — no Qt.
 * Walks the same startup sequence as GhosttyRuntime::initialize, flushing after
 * each step, to localize the Windows startup fail-fast (0xC0000409) to a single
 * libghostty entry point across the gnu->MSVC link boundary. */
#include <stdio.h>
#include <string.h>
#include "ghostty.h"

#define STEP(msg) do { printf("PROBE: %s\n", msg); fflush(stdout); } while (0)

int main(void) {
    STEP("start");

    STEP("calling ghostty_init");
    int rc = ghostty_init(0, NULL);
    printf("PROBE: ghostty_init returned %d (SUCCESS=%d)\n", rc, GHOSTTY_SUCCESS);
    fflush(stdout);
    if (rc != GHOSTTY_SUCCESS) { STEP("init failed -> exit"); return 2; }

    STEP("calling ghostty_config_new");
    ghostty_config_t cfg = ghostty_config_new();
    printf("PROBE: ghostty_config_new -> %p\n", (void*)cfg);
    fflush(stdout);
    if (!cfg) { STEP("config_new null -> exit"); return 3; }

    STEP("calling ghostty_config_load_default_files");
    ghostty_config_load_default_files(cfg);
    STEP("calling ghostty_config_load_recursive_files");
    ghostty_config_load_recursive_files(cfg);
    STEP("calling ghostty_config_finalize");
    ghostty_config_finalize(cfg);
    STEP("config finalized");

    ghostty_runtime_config_s rt;
    memset(&rt, 0, sizeof(rt));
    rt.userdata = NULL;
    rt.supports_selection_clipboard = false;
    /* leave all callbacks NULL for the probe; app_new should still construct. */

    STEP("calling ghostty_app_new");
    ghostty_app_t app = ghostty_app_new(&rt, cfg);
    printf("PROBE: ghostty_app_new -> %p\n", (void*)app);
    fflush(stdout);
    if (!app) { STEP("app_new null -> exit"); return 4; }

    STEP("all init steps survived");
    ghostty_app_free(app);
    ghostty_config_free(cfg);
    STEP("done");
    return 0;
}
