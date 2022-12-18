#ifndef SP_TEST_COMMON_H
#define SP_TEST_COMMON_H

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <stdlib.h>

#include "subprocess/process.h"

#define sp_run(argv, opts)                                 \
    __extension__({                                        \
        SP_Process* __proc;                                \
        __proc = sp_run(argv, opts);                       \
        if (!__proc)                                       \
            cr_fatal("[sp_run] Process failed to start!"); \
        __proc;                                            \
    })

#define sp_open(argv, opts)                                 \
    __extension__({                                         \
        SP_Process* __proc;                                 \
        __proc = sp_open(argv, opts);                       \
        if (!__proc)                                        \
            cr_fatal("[sp_open] Process failed to start!"); \
        __proc;                                             \
    })

#define BUF_SIZE 1024

#define assert_file_contents(file, expected)          \
    do {                                              \
        cr_assert(not(zero(ptr, file)));              \
        char buf[BUF_SIZE];                           \
        size_t n = fread(buf, BUF_SIZE - 1, 1, file); \
        if (!n)                                       \
            cr_assert(zero(ferror(file)));            \
        buf[BUF_SIZE - 1] = 0;                        \
        cr_assert(not(zero(ptr, buf)));               \
        cr_assert(eq(str, buf, expected));            \
    } while (0)

#define diff_files(f1, f2)                                           \
    do {                                                             \
        SP_Process* diff = sp_run(SP_ARGV("diff", "-N", f1, f2), 0); \
        cr_expect(zero(int, diff->exitCode), "Diff failed");         \
        sp_destroy(diff);                                            \
    } while (0)

#endif  // SP_TEST_COMMON_H
