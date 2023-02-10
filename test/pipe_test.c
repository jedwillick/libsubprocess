#include "subprocess/pipe.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "subprocess/redirect.h"
#include "util_test.h"

TestSuite(pipe, .timeout = 2);

Test(pipe, close_pipe) {
    SP_RedirOpt opt = SP_REDIR_PIPE();
    cr_assert(zero(sp_pipe_create(&opt, false)));
    sp_pipe_close(opt.value.pipeFd);
    cr_assert(eq(int, fcntl(opt.value.pipeFd[0], F_GETFD), -1));
    cr_assert(eq(int, fcntl(opt.value.pipeFd[1], F_GETFD), -1));
}

Test(pipe, create_pipe) {
    SP_RedirOpt opt = SP_REDIR_PIPE();
    cr_assert(zero(sp_pipe_create(&opt, false)));
    // Check for FD_CLOEXEC
    cr_assert(eq(int, fcntl(opt.value.pipeFd[0], F_GETFD), FD_CLOEXEC));
    cr_assert(eq(int, fcntl(opt.value.pipeFd[1], F_GETFD), FD_CLOEXEC));

    char* input = "sent through the pipe!";
    int size = strlen(input) + 1;  // Write null byte
    cr_assert(eq(int, write(opt.value.pipeFd[1], input, size), size));
    char output[size];
    cr_assert(eq(int, read(opt.value.pipeFd[0], output, size), size));
    cr_assert(eq(str, input, output));
}

Test(pipe, create_pipe_with_bytes) {
    char* input = "sent through the pipe!";
    int size = strlen(input) + 1;  // write null byte
    SP_RedirOpt opt = SP_REDIR_BYTES(input, size);
    cr_assert(zero(sp_pipe_create(&opt, false)));
    char output[size];
    cr_assert(eq(int, read(opt.value.pipeFd[0], output, size), size));
    cr_assert(eq(str, input, output));
}

Test(pipe, create_non_blocking) {
    SP_RedirOpt opt = SP_REDIR_PIPE();
    cr_assert(zero(int, sp_pipe_create(&opt, true)));

    char out;
    cr_assert(eq(int, read(opt.value.pipeFd[0], &out, 1), -1));
    cr_assert(any(eq(int, errno, EAGAIN), eq(int, errno, EWOULDBLOCK)),
              "read: %s", strerror(errno));
    char in = 'x';
    cr_assert(eq(int, write(opt.value.pipeFd[1], &in, 1), 1));
    cr_assert(eq(int, read(opt.value.pipeFd[0], &out, 1), 1));
    cr_assert(eq(chr, out, in));
}
