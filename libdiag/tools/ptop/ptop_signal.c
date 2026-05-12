#include "ptop.h"
#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t g_exit_flag = 0;

static void handle_signal(int sig)
{
    (void)sig;
    g_exit_flag = 1;
}

int ptop_should_exit(void)
{
    return g_exit_flag != 0;
}

void ptop_terminal_hide_cursor(void)
{
    /* hide cursor */
    printf("\033[?25l");
    fflush(stdout);
}

void ptop_terminal_restore(void)
{
    /* show cursor */
    printf("\033[?25h");
    printf("\n");
    fflush(stdout);
}

static void atexit_restore(void)
{
    ptop_terminal_restore();
}

void ptop_signal_init(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    atexit(atexit_restore);
}
