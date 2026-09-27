/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * rs_console_rtt.h — INTERNAL (private) declarations for the CONSOLE_RTT PES.
 *
 * NOT a public API. The public "API" of this PES is the standard C library
 * console (printf / fgets / ...). These two functions are the portable RTT
 * implementation, shared by:
 *   - the target syscall wrappers  (rs_console_rtt_syscalls.c: _write / _read)
 *   - the host unit tests          (test/test_console_rtt.c)
 *
 * The header lives in code/src/ (a PRIVATE include dir of the rs_console_rtt
 * module) and is NOT exposed via PUBLIC_HEADERS, so application code cannot
 * include it or call these functions directly.
 **********************************************************************************************************************/
#ifndef RS_CONSOLE_RTT_H
#define RS_CONSOLE_RTT_H

/* Portable RTT console logic. The target syscall wrappers (_write/_read) and
 * the host unit tests both call these; nothing else should. */
int rs_console_rtt_write_impl(int fd, const char * buf, int len);
int rs_console_rtt_read_impl(int fd, char * buf, int len);

#endif /* RS_CONSOLE_RTT_H */
