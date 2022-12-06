#include "sp_process.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "sp_io.h"
#include "sp_pipe.h"
#include "sp_redirect.h"

// Dupe a NULL terminated array
static char** dupe_array(char** arr) {
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

static void safe_fclose(FILE* file) {
    if (file) {
        fclose(file);
    }
}

static int sp_create_pipes(SP_Options* opts) {
    return sp_pipe_create(&opts->stdin) | sp_pipe_create(&opts->stdout) |
           sp_pipe_create(&opts->stderr);
}

static int sp_redirect_all(SP_Options* opts) {
    return sp_redirect(&opts->stdin, SP_REDIRECT_STDIN) |
           sp_redirect(&opts->stdout, SP_REDIRECT_STDOUT) |
           sp_redirect(&opts->stderr, SP_REDIRECT_STDERR);
}

// Child exec handler
static void sp_child_exec(SP_Options* opts) {
    if (opts->cwd && chdir(opts->cwd) < 0) {
        return;
    }
    if (sp_redirect_all(opts) < 0) {
        return;
    }
    if (opts->env) {
        execve(opts->argv[0], opts->argv, opts->env);
    } else {
        execvp(opts->argv[0], opts->argv);
    }
}

// fdopen the correct ends of the pipes and close the other ends.
// Should be called in parent after fork()
// TODO: remove dupe code
static int sp_fdopen_all(SP_Process* proc, SP_Options* opts) {
    if (opts->stdin.type == SP_IO_PIPE) {
        proc->stdin = sp_pipe_fdopen(opts->stdin.value.pipeFd, true);
        if (!proc->stdin) {
            return -1;
        }
    }
    if (opts->stdout.type == SP_IO_PIPE) {
        proc->stdout = sp_pipe_fdopen(opts->stdout.value.pipeFd, false);
        if (!proc->stdout) {
            return -1;
        }
    }
    if (opts->stderr.type == SP_IO_PIPE) {
        proc->stderr = sp_pipe_fdopen(opts->stderr.value.pipeFd, false);
        if (!proc->stderr) {
            return -1;
        }
    }
    return 0;
}

// run and wait for process to finish
SP_Process* sp_run(SP_Options* opts) {
    SP_Process* proc = sp_open(opts);
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
SP_Process* sp_open(SP_Options* opts) {
    if (!opts || !opts->argv || !opts->argv[0]) {
        errno = EINVAL;
        return NULL;
    }
    SP_Process* proc = calloc(1, sizeof *proc);
    if (sp_create_pipes(opts) < 0) {
        sp_destroy(proc);
        return NULL;
    }
    proc->pid = fork();
    switch (proc->pid) {
        case -1:  // FORK ERROR
            sp_destroy(proc);
            return NULL;
        case 0:  // CHILD
            sp_child_exec(opts);
            _exit(99);
        default:  // PARENT
            proc->status = SP_STATUS_RUNNING;
            if (sp_fdopen_all(proc, opts) < 0) {
                sp_destroy(proc);
                return NULL;
            }
            proc->argv = dupe_array(opts->argv);
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

// close stdin of process if opened with a pipe.
void sp_close(SP_Process* proc) {
    safe_fclose(proc->stdin);
    proc->stdin = NULL;
}

int sp_wait_opts(SP_Process* proc, int options) {
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
    return sp_wait_opts(proc, 0);
}

// check if a process has terminated without blocking
// and set proc->exitCode if it has
int sp_poll(SP_Process* proc) {
    return sp_wait_opts(proc, WNOHANG);
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
    if (proc->argv) {
        for (int i = 0; proc->argv[i]; i++) {
            free(proc->argv[i]);
        }
        free(proc->argv);
    }
    safe_fclose(proc->stdin);
    safe_fclose(proc->stderr);
    safe_fclose(proc->stdout);
    free(proc);
}
