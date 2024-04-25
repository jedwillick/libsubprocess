#include <assert.h>
#include <string.h>

#include "subprocess/process.h"

int main(void) {
    SP_Opts opts = {
        .spstdin = SP_REDIR_PIPE(),
        .spstdout = SP_REDIR_PIPE(),
        .spstderr = SP_REDIR_PIPE(),
    };
    SP_Process* p1 = sp_open(SP_ARGV("sort", "-r"), &opts);
    fprintf(p1->spstdin, "abc\n");
    fprintf(p1->spstdin, "xyz\n");
    sp_close(p1);
    sp_wait(p1);
    assert(p1->exitCode == 0);
    opts.spstdin = SP_REDIR_FILE(p1->spstdout);
    SP_Process* p2 = sp_run(SP_ARGV("cat"), &opts);
    assert(p2->exitCode == 0);
    sp_destroy(p1);

    char buf[24];
    assert(fgets(buf, 24, p2->spstdout));
    assert(!strcmp(buf, "xyz\n"));
    assert(fgets(buf, 24, p2->spstdout));
    assert(!strcmp(buf, "abc\n"));
    sp_destroy(p2);
}
