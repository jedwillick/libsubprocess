#include "subprocess/process.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "subprocess/pipe.h"
#include "subprocess/redirect.h"

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
    int cond = !sp_pipe_create(&opts->stdin) &&
               !sp_pipe_create(&opts->stdout) &&
               !sp_pipe_create(&opts->stderr);
    return cond ? 0 : -1;
}

static void check_redirect_order(SP_RedirectTarget order[3]) {
    if (order[0] == order[1] || order[0] == order[2] || order[1] == order[2]) {
        order[0] = SP_REDIRECT_STDIN;
        order[1] = SP_REDIRECT_STDOUT;
        order[2] = SP_REDIRECT_STDERR;
    }
}

static SP_IOOptions* sp_fd_to_io_opts(SP_Options* opts,
        SP_RedirectTarget target) {
    switch (target) {
        case SP_REDIRECT_STDIN:
            return &opts->stdin;
        case SP_REDIRECT_STDOUT:
            return &opts->stdout;
        case SP_REDIRECT_STDERR:
            return &opts->stderr;
        default:
            return NULL;
    }
}

static int sp_redirect_all(SP_Options* opts) {
    check_redirect_order(opts->redirectOrder);
    for (int i = 0; i < SP_SIZE_FIXED_ARR(opts->redirectOrder); i++) {
        SP_IOOptions* ioOpts = sp_fd_to_io_opts(opts, opts->redirectOrder[i]);
        int err = sp_redirect(ioOpts, opts->redirectOrder[i]);
        if (err < 0) {
            return err;
        }
    }
    return 0;
}

// Child exec handler
static int sp_child_exec(char** argv, SP_Options* opts) {
    if (opts && sp_redirect_all(opts) < 0) {
        // Error output is done in redirect.c since it has access to
        // specific details.
        return SP_EXIT_NOT_EXECUTE;
    }
    if (opts && opts->cwd && chdir(opts->cwd) < 0) {
        SP_ERROR_MSG("chdir: %s", opts->cwd);
        return SP_EXIT_NOT_EXECUTE;
    }
    if (opts && opts->env) {
        execve(argv[0], argv, opts->env);
    } else {
        execvp(argv[0], argv);
    }
    SP_ERROR_MSG("exec: %s", argv[0]);
    switch (errno) {
        case EISDIR:
        case EACCES:
        case ELIBBAD:
        case ENOEXEC:
        case EIO:
            return SP_EXIT_NOT_EXECUTE;
        default:
            return SP_EXIT_NOT_FOUND;
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
SP_Process* sp_run(char** argv, SP_Options* opts) {
    SP_Process* proc = sp_open(argv, opts);
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
SP_Process* sp_open(char** argv, SP_Options* opts) {
    if (!argv || !argv[0]) {
        errno = EINVAL;
        return NULL;
    }
    SP_Process* proc = calloc(1, sizeof *proc);
    if (opts && sp_create_pipes(opts) < 0) {
        sp_destroy(proc);
        return NULL;
    }
    proc->pid = fork();
    switch (proc->pid) {
        case -1:  // FORK ERROR
            sp_destroy(proc);
            return NULL;
        case 0:  // CHILD
            _exit(sp_child_exec(argv, opts));
        default:  // PARENT
            proc->status = SP_STATUS_RUNNING;
            if (opts && sp_fdopen_all(proc, opts) < 0) {
                sp_destroy(proc);
                return NULL;
            }
            proc->argv = dupe_array(argv);
    }
    return proc;
}

// send SIGTERM to process
// return -1 on error, otherwise 0
int sp_terminate(SP_Process* proc) {
    return sp_signal(proc, SIGTERM);
}

// send SIGKILL to process
// return -1 on error, otherwise 0
int sp_kill(SP_Process* proc) {
    return sp_signal(proc, SIGKILL);
}
// send signal to process
// return -1 on error, otherwise 0
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

// return -1 on error,
// return 0 if successfully reaped proc,
// return 1 if still running
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
// return -1 on error, otherwise 0
int sp_wait(SP_Process* proc) {
    return sp_wait_opts(proc, 0);
}

// check if a process has terminated without blocking
// return -1 on error or still running, otherwise 0
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
