#include "ptop.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static volatile sig_atomic_t g_exit_flag = 0;
static volatile sig_atomic_t g_resize_flag = 0;
static volatile sig_atomic_t g_cursor_hidden = 0;

static void handle_signal(int sig)
{
    (void)sig;
    g_exit_flag = 1;
}

static void handle_winch(int sig)
{
    (void)sig;
    g_resize_flag = 1;
}

int ptop_should_exit(void)
{
    return g_exit_flag != 0;
}

int ptop_terminal_resized(void)
{
    return g_resize_flag != 0;
}

void ptop_clear_resize_flag(void)
{
    g_resize_flag = 0;
}

void ptop_terminal_hide_cursor(void)
{
    /* hide cursor */
    printf("\033[?25l");
    fflush(stdout);
    g_cursor_hidden = 1;
}

void ptop_terminal_restore(void)
{
    if (!g_cursor_hidden)
        return;

    /* show cursor */
    printf("\033[?25h");
    printf("\n");
    fflush(stdout);
    g_cursor_hidden = 0;
}

static void atexit_restore(void)
{
    ptop_terminal_restore();
}

void ptop_signal_init(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGWINCH, handle_winch);

    atexit(atexit_restore);
}
