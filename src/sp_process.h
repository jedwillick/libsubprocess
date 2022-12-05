#ifndef SP_PROCESS_H
#define SP_PROCESS_H

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

typedef enum sp_status {
    SP_STATUS_RUNNING,
    SP_STATUS_DEAD,
} SP_Status;

typedef struct sp_process {
    pid_t pid;    // process id
    char** argv;  // argv copied from options
    // FILE* stdin;       // stdin of process
    // FILE* stdout;      // stdout of process
    // FILE* stderr;      // stderr of process
    SP_Status status;  // status of process
    // exitCode only valid if status == SP_STATUS_DEAD
    int exitCode;  // exit code of process (-N indicates terminated with signal N)
} SP_Process;

typedef struct sp_options {
    char** argv;  // argv passed to process
    char* cwd;    // current working directory
    char** env;   // environment passed to execve
} SP_Options;

// run and wait for process to finish
SP_Process* sp_run(SP_Options* options);

// open a new process
SP_Process* sp_open(SP_Options* options);

// send SIGTERM to process
int sp_terminate(SP_Process* process);

// send SIGKILL to process
int sp_kill(SP_Process* process);

// send signal to process
int sp_signal(SP_Process* process, int signal);

// close stdin of process
// int sp_close(SP_Process* process);

// wait for process to exit and set proc->exitCode
int sp_wait(SP_Process* process);

// check if a process has terminated without blocking
// and set proc->exitCode if it has
int sp_poll(SP_Process* process);

// Free all memory allocated by process
// If process is NULL, this function does nothing
// If process is still running, it is killed and waited on.
void sp_destroy(SP_Process* process);

#endif  // SP_PROCESS_H
