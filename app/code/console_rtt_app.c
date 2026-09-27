/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * console_rtt_app.c — app_console_rtt use-case.
 *
 * Demonstrates that ordinary application code using the STANDARD C CONSOLE
 * (printf / fgets) works unchanged over SEGGER RTT — no UART, no Picolibc, no
 * logger and no RTOS. The CONSOLE_RTT PES (rs_console_rtt) supplies the newlib
 * _write()/_read() retarget that routes stdio to SEGGER RTT channel 0.
 *
 * Behaviour (per specification):
 *   1. Print a banner.
 *   2. Read a complete, bounded input line from the RTT Viewer.
 *   3. Echo it back as:  "You printed: <input>"
 *
 * USE-CASE RULES (same as any AUC in this repo):
 *   - The use-case calls the (standard C) console API. It never touches the
 *     transport directly — the PES owns _write()/_read().
 *   - No dynamic allocation: the input line uses a fixed, bounded buffer.
 **********************************************************************************************************************/

#include <stdio.h>
#include <string.h>

/* Maximum bounded input line length, including the NUL terminator. */
#define APP_CONSOLE_LINE_MAX   (128)

void console_rtt_app_run(void)
{
    char line[APP_CONSOLE_LINE_MAX];

    /* newlib streams to a non-tty are fully buffered by default. Make stdout
     * unbuffered so the banner and each echo reach the RTT Viewer immediately,
     * without needing an explicit fflush(). */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("System Init\r\n");
    printf("CONSOLE_RTT ready. Type a line and press Enter.\r\n");

    for (;;)
    {
        /* Read one complete, bounded line. fgets() stops at newline or when the
         * buffer is full, and always NUL-terminates — no overflow. */
        if (NULL != fgets(line, (int)sizeof line, stdin))
        {
            /* Strip the trailing CR/LF the terminal appends. */
            line[strcspn(line, "\r\n")] = '\0';

            printf("You printed: %s\r\n", line);
        }
    }
}
