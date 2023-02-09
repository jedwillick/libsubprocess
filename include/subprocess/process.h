#ifndef SP_PROCESS_H
#define SP_PROCESS_H

#include <stdio.h>
#include <unistd.h>

#include "subprocess/error.h"
#include "subprocess/redirect.h"

#define SP_SIGNAL_OFFSET 128

typedef enum sp_status {
    SP_STATUS_SPAWNING = 0,
    SP_STATUS_RUNNING,
    SP_STATUS_DEAD,
} SP_Status;

typedef struct sp_process {
    pid_t pid;         // process id
    char** argv;       // argv copied
    SP_Status status;  // status of process
    // exitCode only valid if status == SP_STATUS_DEAD
    int exitCode;  // exit code of process
    // If RedirOpt specifies SP_REDIR_PIPE than this will
    // be opened from the corresponding pipe ends
    FILE* stdin;   // stdin of process
    FILE* stdout;  // stdout of process
    FILE* stderr;  // stderr of process
} SP_Process;

typedef struct sp_opts {
    char* cwd;    // current working directory
    char** env;   // environment passed to execve
    bool detach;  // detach process from parent
    // Don't attempt to close other open file descriptors.
    bool inheritFds;

    SP_RedirOpt stdin;
    SP_RedirOpt stdout;
    SP_RedirOpt stderr;
    // Specifies the order to attempt redirection.
    // Default order is {0, 1, 2}. stdin then stdout then stderr.
    // If given you must fully specify the unqiue order.
    // e.g. .redirOrder = {2, 1, 0}
    SP_RedirTarget redirOrder[3];
    // Make pipes non-blocking.
    bool nonBlockingPipes;
} SP_Opts;

#define SP_ARGV(...) \
    (char*[]) { __VA_ARGS__, NULL }

#define SP_OPTS(...) &((SP_Opts){__VA_ARGS__})

#define SP_SIZE_FIXED_ARR(fixedArray) \
    (sizeof(fixedArray) / sizeof(fixedArray[0]))

// run and wait for process to finish
SP_Process* sp_run(char** argv, SP_Opts* options);

// open a new process
SP_Process* sp_open(char** argv, SP_Opts* options);

// send SIGTERM to process
// return -1 on error, otherwise 0
int sp_terminate(SP_Process* process);

// send SIGKILL to process
// return -1 on error, otherwise 0
int sp_kill(SP_Process* process);

// send signal to process
// return -1 on error, otherwise 0
int sp_signal(SP_Process* process, int signal);

// close stdin of process if opened with a pipe.
void sp_close(SP_Process* process);

// wait for process to exit and set proc->exitCode
// return -1 on error, otherwise the exit code
int sp_wait(SP_Process* process);

// check if a process has terminated without blocking
// and set proc->exitCode if it has
// return -1 on error or still running, otherwise the exit code
int sp_poll(SP_Process* process);

// Free all memory allocated by process
// If process is NULL, this function does nothing
// If process is still running, it is killed and waited on.
void sp_destroy(SP_Process* process);

#endif  // SP_PROCESS_H
