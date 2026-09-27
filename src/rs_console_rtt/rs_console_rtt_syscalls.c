/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * rs_console_rtt_syscalls.c — TARGET-ONLY newlib syscall wrappers for CONSOLE_RTT.
 *
 * Under GCC/newlib (linked with --specs=nosys.specs) the C library funnels all
 * stream I/O through the POSIX syscall stubs _write() and _read(). nosys.specs
 * provides *weak* stubs that do nothing. The strong _write()/_read() defined
 * here override those weak stubs and forward to the portable RTT logic in
 * rs_console_rtt.c (rs_console_rtt_write_impl / rs_console_rtt_read_impl).
 *
 * This file is compiled ONLY into the target build. It is deliberately excluded
 * from the host unit-test build, where the host C runtime owns _write()/_read()
 * and the tests call the *_impl() functions directly.
 **********************************************************************************************************************/

#include "rs_console_rtt.h"

#include <errno.h>
#include <sys/stat.h>

/* newlib has no public prototypes for these; declare them to satisfy -Wmissing-declarations. */
int _write(int fd, const char * buf, int len);
int _read(int fd, char * buf, int len);
int _close(int fd);
int _fstat(int fd, struct stat * st);
int _isatty(int fd);
int _lseek(int fd, int offset, int whence);

/* stdin / stdout / stderr are the only descriptors backed by RTT. */
static int rs_console_rtt_is_std_fd(int fd)
{
    return (fd >= 0) && (fd <= 2);
}

/*──────────────────────────────────────────────────────────────────────────────
 * _write — stdout / stderr -> RTT up-buffer (target -> J-Link).
 *
 * @return  Number of bytes actually accepted by RTT (0..len), or -1 with errno
 *          set for an unsupported file descriptor.
 *────────────────────────────────────────────────────────────────────────────*/
int _write(int fd, const char * buf, int len)
{
    return rs_console_rtt_write_impl(fd, buf, len);
}

/*──────────────────────────────────────────────────────────────────────────────
 * _read — stdin <- RTT down-buffer (J-Link -> target).
 *
 * @return  Number of bytes read (>= 1 for len > 0), 0 for len == 0, or -1 with
 *          errno set for an unsupported file descriptor.
 *────────────────────────────────────────────────────────────────────────────*/
int _read(int fd, char * buf, int len)
{
    return rs_console_rtt_read_impl(fd, buf, len);
}

/*──────────────────────────────────────────────────────────────────────────────
 * _close / _fstat / _isatty / _lseek — minimal implementations for the RTT-backed
 * standard streams. They replace the nosys.specs stubs, which are tagged with
 * "not implemented and will always fail" link-time warnings. Reporting the
 * standard streams as character devices / TTYs also makes newlib line-buffer
 * stdout instead of fully buffering it.
 *────────────────────────────────────────────────────────────────────────────*/
int _close(int fd)
{
    (void) fd;
    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat * st)
{
    if (!rs_console_rtt_is_std_fd(fd) || (NULL == st))
    {
        errno = EBADF;
        return -1;
    }

    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    if (rs_console_rtt_is_std_fd(fd))
    {
        return 1;
    }

    errno = EBADF;
    return 0;
}

int _lseek(int fd, int offset, int whence)
{
    (void) offset;
    (void) whence;
    errno = rs_console_rtt_is_std_fd(fd) ? ESPIPE : EBADF;
    return -1;
}
