#include "test_common.h"

void assert_file_contents(FILE* file, char* expected) {
    cr_assert(not(zero(ptr, file)));
    char buf[BUF_SIZE + 1] = {0};
    size_t n = fread(buf, BUF_SIZE, 1, file);
    if (!n) cr_assert(zero(ferror(file)));
    cr_assert(not(zero(ptr, buf)));
    cr_assert(eq(str, buf, expected));
}

void diff_files(char* f1, char* f2) {
    SP_Process* diff = sp_run(SP_ARGV("diff", "-N", f1, f2), 0);
    int err = diff->exitCode;
    sp_destroy(diff);
    cr_expect(zero(int, err), "Diff failed");
}

SP_Process* assert_process_started(char* file, int line,
                                   SP_Process* (*start)(char**, SP_Opts*),
                                   char** argv, SP_Opts* opts) {
    SP_Process* p = start(argv, opts);
    if (!p) cr_fatal("[%s:%d] Process failed to start!", file, line);
    return p;
}
