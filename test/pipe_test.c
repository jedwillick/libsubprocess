#include "subprocess/pipe.h"

#include <criterion/criterion.h>
#include <criterion/internal/test.h>
#include <criterion/new/assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "subprocess/io.h"

TestSuite(pipe, .timeout = 2);

Test(pipe, close_pipe) {
    SP_IOOptions opts = SP_IO_OPTS_PIPE();
    cr_assert(zero(sp_pipe_create(&opts)));
    sp_pipe_close(opts.value.pipeFd);
    cr_assert(eq(int, fcntl(opts.value.pipeFd[0], F_GETFD), -1));
    cr_assert(eq(int, fcntl(opts.value.pipeFd[1], F_GETFD), -1));
}

Test(pipe, create_pipe) {
    SP_IOOptions opts = SP_IO_OPTS_PIPE();
    cr_assert(zero(sp_pipe_create(&opts)));
    // Check for FD_CLOEXEC
    cr_assert(eq(int, fcntl(opts.value.pipeFd[0], F_GETFD), FD_CLOEXEC));
    cr_assert(eq(int, fcntl(opts.value.pipeFd[1], F_GETFD), FD_CLOEXEC));

    char* input = "sent through the pipe!";
    int size = strlen(input) + 1;  // Write null byte
    cr_assert(eq(int, write(opts.value.pipeFd[1], input, size), size));
    char output[size];
    cr_assert(eq(int, read(opts.value.pipeFd[0], output, size), size));
    cr_assert(eq(str, input, output));
}

Test(pipe, create_pipe_with_bytes) {
    char* input = "sent through the pipe!";
    int size = strlen(input) + 1;  //write null byte
    SP_IOOptions opts = SP_IO_OPTS_BYTES(input, size);
    cr_assert(zero(sp_pipe_create(&opts)));
    char output[size];
    cr_assert(eq(int, read(opts.value.pipeFd[0], output, size), size));
    cr_assert(eq(str, input, output));
}
