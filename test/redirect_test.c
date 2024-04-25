
#include "subprocess/redirect.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <stdlib.h>

#include "criterion/internal/new_asserts.h"
#include "subprocess/process.h"
#include "util_test.h"

static SP_Process* proc1;
static SP_Process* proc2;
static SP_Process* proc3;
static FILE* tmp;

static void setup(void) {
    proc1 = NULL;
    proc2 = NULL;
    proc3 = NULL;
    tmp = NULL;
}

static void teardown(void) {
    sp_destroy(proc1);
    sp_destroy(proc2);
    sp_destroy(proc3);
    if (tmp) {
        fclose(tmp);
    }
}

TestSuite(redir, .timeout = 15, .init = setup, .fini = teardown);

Test(redir, in_out_path) {
    unlink("test/txt.out");
    SP_Opts opts = {
        .spstdin = SP_REDIR_PATH("test/txt.in"),
        .spstdout = SP_REDIR_PATH("test/txt.out"),
    };
    proc1 = sp_run(SP_ARGV("cat"), &opts);
    cr_assert(zero(int, proc1->exitCode));
    diff_files("test/txt.in", "test/txt.out");
}

Test(redir, append) {
    unlink("test/append.out");
    proc1 = sp_run(SP_ARGV("echo", "Line 1"),
                   SP_OPTS(.spstdout = SP_REDIR_PATH("test/append.out")));
    cr_assert(zero(int, proc1->exitCode));
    sp_destroy(proc1);
    proc1 = sp_run(SP_ARGV("echo", "Line 2"),
                   SP_OPTS(.spstdout = SP_REDIR_APPEND("test/append.out")));
    cr_assert(zero(int, proc1->exitCode));
    diff_files("test/append.in", "test/append.out");
}

Test(redir, err_to_out_to_file) {
    tmp = tmpfile();
    SP_Opts opts = {
        .spstdout = SP_REDIR_FILE(tmp),
        .spstderr = SP_REDIR_STDOUT(),
        .redirOrder = {1, 2},
    };
    proc1 = sp_run(SP_ARGV("sh", "-c", "printf abc123 >&2"), &opts);
    cr_assert(zero(int, proc1->exitCode));
    rewind(tmp);
    assert_file_contents(tmp, "abc123");
}

Test(redir, out_to_err_to_pipe) {
    SP_Opts opts = {
        .spstdout = SP_REDIR_STDERR(),
        .spstderr = SP_REDIR_PIPE(),
        .redirOrder = {2, 1},
    };
    proc1 = sp_run(SP_ARGV("printf", "abc123"), &opts);
    cr_assert(zero(int, proc1->exitCode));
    assert_file_contents(proc1->spstderr, "abc123");
}

Test(redir, bytes_to_fd) {
    tmp = tmpfile();
    char* bytes = "abc123";
    int size = strlen(bytes) + 1;  // include NULL byte
    SP_Opts opts = {
        .spstdin = SP_REDIR_BYTES(bytes, size),
        .spstdout = SP_REDIR_FD(fileno(tmp)),
    };
    proc1 = sp_run(SP_ARGV("cat"), &opts);
    cr_assert(zero(int, proc1->exitCode));
    rewind(tmp);
    assert_file_contents(tmp, "abc123");
}

Test(redir, pipe_chain) {
    // printf "abc123\nxyz789\n" | tr a-z A-Z | sort -r
    SP_Opts opts1 = {.spstdout = SP_REDIR_PIPE()};
    proc1 = sp_run(SP_ARGV("printf", "abc123\nxyz789\n"), &opts1);
    cr_assert(zero(int, proc1->exitCode));
    SP_Opts opts2 = {
        .spstdin = SP_REDIR_FILE(proc1->spstdout),
        .spstdout = SP_REDIR_PIPE(),
    };
    proc2 = sp_run(SP_ARGV("tr", "a-z", "A-Z"), &opts2);
    cr_assert(zero(int, proc2->exitCode));
    SP_Opts opts3 = {
        .spstdin = SP_REDIR_FILE(proc2->spstdout),
        .spstdout = SP_REDIR_PIPE(),
    };
    proc3 = sp_run(SP_ARGV("sort", "-r"), &opts3);
    cr_assert(zero(int, proc3->exitCode));
    assert_file_contents(proc3->spstdout, "XYZ789\nABC123\n");
}
