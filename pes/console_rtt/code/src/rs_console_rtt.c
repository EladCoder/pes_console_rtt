/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * rs_console_rtt.c — CONSOLE_RTT PES: standard C stdio over SEGGER RTT.
 *
 * Job-to-be-done
 * --------------
 * Make the standard C library console (printf / fprintf / puts / putchar /
 * getchar / scanf / fgets on stdin / stdout / stderr) work over SEGGER RTT
 * instead of UART, WITHOUT changing any application code.
 *
 * How it works
 * ------------
 * Under GCC/newlib (linked with --specs=nosys.specs) the C library funnels all
 * stream I/O through the POSIX syscall stubs _write() and _read(). nosys.specs
 * provides *weak* stubs that do nothing. The strong _write()/_read() that
 * override those stubs live in rs_console_rtt_syscalls.c (target-only) and
 * forward to the portable logic in THIS file, which routes every libc stream to
 * SEGGER RTT channel 0:
 *
 *     printf / fputs / putchar  ->  _write(1|2, ...)  ->  rs_console_rtt_write_impl  ->  SEGGER_RTT_Write(0, ...)
 *     getchar / fgets / scanf   ->  _read (0, ...)    ->  rs_console_rtt_read_impl   ->  SEGGER_RTT_GetKey(0)
 *
 * No UART, no Picolibc, no logger framework, no dynamic allocation, and no
 * mandatory RTOS. This is the ONLY source file of the PES; the SEGGER RTT ring
 * buffer is the single buffer in the path (no duplicate buffering).
 *
 * Partial-write policy (truthful)
 * -------------------------------
 * Channel 0 runs in NO_BLOCK_TRIM mode, so SEGGER_RTT_Write() may accept fewer
 * bytes than requested (or zero, when no J-Link/Viewer is draining the buffer).
 * _write() returns the ACTUAL number of bytes accepted (0..len); it never
 * fabricates a full-length success and never silently discards output. If a
 * write iteration makes no progress, _write() stops and returns the truthful
 * count so far, so the caller (and newlib) observe a short write rather than a
 * spin or a lie.
 *
 * Read semantics (newlib-correct)
 * -------------------------------
 * _read() blocks until at least one byte is available, then returns all
 * currently-available bytes up to len. It does NOT wait to fill the full
 * requested length — matching POSIX read() and the getchar()/fgets() contract.
 **********************************************************************************************************************/

#include "rs_console_rtt.h"
#include "SEGGER_RTT.h"

#include <errno.h>
#include <stddef.h>

/*──────────────────────────────────────────────────────────────────────────────
 * Configuration
 *────────────────────────────────────────────────────────────────────────────*/
#define RS_CONSOLE_RTT_CHANNEL     (0U)   /* single "Terminal" channel */

#define RS_CONSOLE_RTT_FD_STDIN    (0)
#define RS_CONSOLE_RTT_FD_STDOUT   (1)
#define RS_CONSOLE_RTT_FD_STDERR   (2)

/*──────────────────────────────────────────────────────────────────────────────
 * rs_console_rtt_write_impl — stdout / stderr -> RTT up-buffer (target -> J-Link).
 *
 * Portable logic behind the target _write() syscall (rs_console_rtt_syscalls.c)
 * and directly exercised by the host unit tests.
 *
 * @return  Number of bytes actually accepted by RTT (0..len), or -1 with errno
 *          set for an unsupported file descriptor.
 *────────────────────────────────────────────────────────────────────────────*/
int rs_console_rtt_write_impl(int fd, const char * buf, int len)
{
    const char * p;
    unsigned     remaining;
    unsigned     total;
    unsigned     n;

    if ((RS_CONSOLE_RTT_FD_STDOUT != fd) && (RS_CONSOLE_RTT_FD_STDERR != fd))
    {
        errno = EBADF;
        return -1;
    }
    if (NULL == buf)
    {
        errno = EFAULT;
        return -1;
    }
    if (len <= 0)
    {
        return 0;
    }

    /* No explicit init: SEGGER_RTT_Write() self-initialises the control block on
     * first use via the library's guarded INIT() (it inits only once, when the
     * "SEGGER RTT" ID is absent). Calling SEGGER_RTT_Init() here would be wrong
     * with the official engine — that entry point re-runs _DoInit() every call
     * and would wipe WrOff/RdOff, discarding buffered data on every write. */

    /* Cast is safe here: the len <= 0 guard above guarantees len >= 1. */
    p         = buf;
    remaining = (unsigned)len;
    total     = 0U;

    while (remaining > 0U)
    {
        n = SEGGER_RTT_Write(RS_CONSOLE_RTT_CHANNEL, p, remaining);
        if (0U == n)
        {
            /* Buffer full / no host draining: report the truthful count so far
             * (may be 0). Do not spin and do not claim bytes were written. */
            break;
        }
        p         += n;
        total     += n;
        remaining -= n;
    }

    return (int)total;
}

/*──────────────────────────────────────────────────────────────────────────────
 * rs_console_rtt_read_impl — stdin <- RTT down-buffer (J-Link -> target).
 *
 * Portable logic behind the target _read() syscall (rs_console_rtt_syscalls.c)
 * and directly exercised by the host unit tests.
 *
 * Blocks until at least one byte is available, then returns all currently
 * available bytes up to len (never waits to fill the whole request).
 *
 * @return  Number of bytes read (>= 1 for len > 0), 0 for len == 0, or -1 with
 *          errno set for an unsupported file descriptor.
 *────────────────────────────────────────────────────────────────────────────*/
int rs_console_rtt_read_impl(int fd, char * buf, int len)
{
    int count;
    int c;

    if (RS_CONSOLE_RTT_FD_STDIN != fd)
    {
        errno = EBADF;
        return -1;
    }
    if (NULL == buf)
    {
        errno = EFAULT;
        return -1;
    }
    if (len <= 0)
    {
        return 0;
    }

    /* No explicit init: SEGGER_RTT_HasKey()/_GetKey() self-initialise via the
     * library's guarded INIT(). SEGGER_RTT_Init() must NOT be called here — the
     * official entry point re-runs _DoInit() unconditionally and would reset the
     * down-buffer offsets, dropping keystrokes already sent by the host. */

    /* Block for the first byte (matches getchar() / blocking read()). */
    while (0 == SEGGER_RTT_HasKey())
    {
        /* spin — bare-metal, no RTOS yield */
    }

    /* Drain what is currently available, up to len. Do not wait for more. */
    count = 0;
    while (count < len)
    {
        c = SEGGER_RTT_GetKey();
        if (c < 0)
        {
            break;  /* nothing more available right now */
        }
        buf[count] = (char)c;
        ++count;
    }

    return count;
}