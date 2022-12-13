#include "subprocess/process.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <signal.h>
#include <unistd.h>

#include "subprocess/io.h"

SP_Process* proc = NULL;
SP_Options* opts = NULL;

void teardown(void) {
    sp_destroy(proc);
    free(opts);
}

void setup_force(void) {
    proc = sp_open(SP_ARGV("cat"), SP_OPTS(.stdin = SP_IO_OPTS_PIPE()));
    cr_assert(not(zero(ptr, proc)));
}

TestSuite(force, .timeout = 10, .init = setup_force, .fini = teardown);

Test(force, sp_kill) {
    cr_assert(zero(int, sp_kill(proc)));
    cr_assert(zero(int, sp_wait(proc)));
    cr_assert(eq(int, proc->exitCode, -SIGKILL));
}

Test(force, sp_terminate) {
    cr_assert(zero(int, sp_terminate(proc)));
    cr_assert(zero(int, sp_wait(proc)));
    cr_assert(eq(int, proc->exitCode, -SIGTERM));
}

Test(force, sp_signal) {
    cr_assert(zero(int, sp_signal(proc, SIGINT)));
    cr_assert(zero(int, sp_wait(proc)));
    cr_assert(eq(int, proc->exitCode, -SIGINT));

    // Should error
    cr_assert(not(zero(int, sp_signal(NULL, SIGINT))));
    cr_assert(not(zero(int, sp_signal(proc, SIGINT))));

    cr_assert(eq(int, proc->exitCode, -SIGINT));
}

Test(force, sp_poll) {
    cr_assert(not(zero(int, sp_poll(proc))));
    sp_kill(proc);
    usleep(1000);
    cr_assert(zero(int, sp_poll(proc)));
}

Test(force, sp_close) {
    cr_assert(not(zero(ptr, proc->stdin)));
    sp_close(proc);
    cr_assert(zero(ptr, proc->stdin));
    sp_wait(proc);
    cr_assert(zero(int, proc->exitCode));
}

void setup_fails(void) {
    opts = malloc(sizeof *opts);
    *opts = (SP_Options){
            .stderr = SP_IO_OPTS_DEVNULL(),
            .redirectOrder = {2, 1, 0},
    };
}

TestSuite(fails, .timeout = 10, .init = setup_fails, .fini = teardown);

Test(fails, bad_cwd) {
    opts->cwd = "NOPE";
    proc = sp_run(SP_ARGV("ls"), opts);
    cr_assert(eq(int, proc->exitCode, SP_EXIT_NOT_EXECUTE));
}

Test(fails, not_found) {
    proc = sp_run(SP_ARGV(""), opts);
    cr_assert(eq(int, proc->exitCode, SP_EXIT_NOT_FOUND));
}

Test(fails, not_executable) {
    proc = sp_run(SP_ARGV("./Makefile"), opts);
    cr_assert(eq(int, proc->exitCode, SP_EXIT_NOT_EXECUTE));
}

Test(fails, bad_fd) {
    opts->stdout = SP_IO_OPTS_FD(666);
    proc = sp_run(SP_ARGV("ls"), opts);
    cr_assert(eq(int, proc->exitCode, SP_EXIT_NOT_EXECUTE));
}

Test(fails, bad_path) {
    opts->stdin = SP_IO_OPTS_PATH("NOPE");
    proc = sp_run(SP_ARGV("ls"), opts);
    cr_assert(eq(int, proc->exitCode, SP_EXIT_NOT_EXECUTE));
}

Test(fails, null_path) {
    opts->stdin = SP_IO_OPTS_PATH(NULL);
    proc = sp_run(SP_ARGV("ls"), opts);
    cr_assert(eq(int, proc->exitCode, SP_EXIT_NOT_EXECUTE));
}

Test(fails, null_file) {
    opts->stdout = SP_IO_OPTS_FILE(NULL);
    proc = sp_run(SP_ARGV("ls"), opts);
    cr_assert(eq(int, proc->exitCode, SP_EXIT_NOT_EXECUTE));
}
