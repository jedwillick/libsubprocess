#include <string.h>

#include "sp_io.h"
#include "sp_process.h"

int main(int argc, char* argv[]) {
    // char* msg = "Hello, world!\n";
    // SP_Options opts = {
    //         .argv = argv + 1,
    //         .stdin = sp_io_options_pipe(),
    //         .stdout = sp_io_options_pipe(),
    // };
    // SP_Process* proc = sp_open(&opts);
    // if (!proc) {
    //     perror("sp_run");
    //     return 1;
    // }
    // fprintf(proc->stdin, "%s", msg);
    // sp_close(proc);
    // char buf[50];
    // if (!fgets(buf, 50, proc->stdout)) {
    //     perror("fgets");
    // } else {
    //     printf("%s\n", buf);
    // }
    //
    // printf("%d\n", proc->exitCode);
    // sp_destroy(proc);

    // cat $argv[1] | sort -r | grep proc
    SP_Options opts1 = {
            .argv = (char*[]){"cat", NULL},
            .stdin = sp_io_options_path(argv[1]),
            .stdout = sp_io_options_pipe(),
    };
    SP_Process* p1 = sp_run(&opts1);
    SP_Options opts2 = {
            .argv = (char*[]){"sort", "-r", NULL},
            .stdin = sp_io_options_file(p1->stdout),
            .stdout = sp_io_options_pipe(),
    };
    SP_Process* p2 = sp_run(&opts2);

    SP_Options opts3 = {
            .argv = (char*[]){"grep", "proc", NULL},
            .stdin = sp_io_options_file(p2->stdout),
            .stderr = sp_io_options_to_stdout(),
    };
    SP_Process* p3 = sp_run(&opts3);

    printf("%s exited with code %d\n", p1->argv[0], p1->exitCode);
    printf("%s exited with code %d\n", p2->argv[0], p2->exitCode);
    printf("%s exited with code %d\n", p3->argv[0], p3->exitCode);
    sp_destroy(p1);
    sp_destroy(p2);
    sp_destroy(p3);

    return 0;
}
