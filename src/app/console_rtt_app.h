/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * console_rtt_app.h — public interface of the app_console_rtt use-case.
 **********************************************************************************************************************/

#ifndef CONSOLE_RTT_APP_H
#define CONSOLE_RTT_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Run the console demo: print a banner, then echo each input line. Never returns. */
void console_rtt_app_run(void);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_RTT_APP_H */
