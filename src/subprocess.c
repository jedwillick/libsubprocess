#include "sp_process.h"

int main(int argc, char* argv[]) {
    SP_Options opts = {.argv = argv + 1};
    SP_Process* proc = sp_run(&opts);
    if (!proc) {
        perror("sp_run");
        return 1;
    }
    printf("%d\n", proc->exitCode);
    sp_destroy(proc);
    return 0;
}
