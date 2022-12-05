#include "sp_process.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Dupe a NULL terminated array
char** dupe_array(char** arr) {
    char** dupeArr = malloc(sizeof(char*));
    int i;
    for (i = 0; arr[i]; i++) {
        // Keep room for NULL byte at the end
        dupeArr = realloc(dupeArr, sizeof(char*) * (i + 2));
        dupeArr[i] = strdup(arr[i]);
    }
    dupeArr[i] = NULL;
    return dupeArr;
}

// Child exec handler
void exec_child(SP_Options* options) {
    if (options->cwd && chdir(options->cwd) < 0) {
        return;
    }
    if (options->env) {
        execve(options->argv[0], options->argv, options->env);
    } else {
        execvp(options->argv[0], options->argv);
    }
}

// run and wait for process to finish
SP_Process* sp_run(SP_Options* options) {
    SP_Process* proc = sp_open(options);
    if (!proc) {
        return NULL;
    }
    if (sp_wait(proc) < 0) {
        sp_destroy(proc);
        return NULL;
    }
    return proc;
}

// open a new process
SP_Process* sp_open(SP_Options* options) {
    if (!options || !options->argv || !options->argv[0]) {
        errno = EINVAL;
        return NULL;
    }
    SP_Process* proc = calloc(1, sizeof *proc);
    proc->argv = dupe_array(options->argv);
    proc->pid = fork();
    switch (proc->pid) {
        case -1:  // FORK ERROR
            sp_destroy(proc);
            return NULL;
        case 0:  // CHILD
            exec_child(options);
            _exit(99);
        default:  // PARENT
            proc->status = SP_STATUS_RUNNING;
    }
    return proc;
}

// send SIGTERM to process
int sp_terminate(SP_Process* proc) {
    return sp_signal(proc, SIGTERM);
}

// send SIGKILL to process
int sp_kill(SP_Process* proc) {
    return sp_signal(proc, SIGTERM);
}
// send signal to process
int sp_signal(SP_Process* proc, int signal) {
    if (!proc || proc->status == SP_STATUS_DEAD) {
        errno = EINVAL;
        return -1;
    }
    return kill(proc->pid, signal);
}

int wait_opts(SP_Process* proc, int options) {
    if (!proc || proc->status == SP_STATUS_DEAD) {
        errno = EINVAL;
        return -1;
    }
    int stat;
    int pid = waitpid(proc->pid, &stat, options);
    if (pid <= 0) {
        return -1;
    }
    if (WIFEXITED(stat)) {
        proc->exitCode = WEXITSTATUS(stat);
    } else if (WIFSIGNALED(stat)) {
        proc->exitCode = -WTERMSIG(stat);
    }
    proc->status = SP_STATUS_DEAD;
    return 0;
}

// wait for process to exit and set proc->exitCode
int sp_wait(SP_Process* proc) {
    return wait_opts(proc, 0);
}

// check if a process has terminated without blocking
// and set proc->exitCode if it has
int sp_poll(SP_Process* proc) {
    return wait_opts(proc, WNOHANG);
}

// Free all memory allocated by process
// If process is NULL, this function does nothing
// If process is still running, it is killed and waited on.
void sp_destroy(SP_Process* proc) {
    if (!proc) {
        return;
    }
    if (proc->status == SP_STATUS_RUNNING) {
        sp_kill(proc);
        sp_wait(proc);
    }
    for (int i = 0; proc->argv[i]; i++) {
        free(proc->argv[i]);
    }
    free(proc->argv);
    free(proc);
}
