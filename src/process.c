#include "subprocess/process.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Free a NULL terminated array of strings.
 *
 * @param[in,out] arr
 */
static void free_array(char** arr) {
    if (!arr) {
        return;
    }
    for (int i = 0; arr[i]; i++) {
        free(arr[i]);
    }
    free(arr);
}

/**
 * Dupe a NULL terminated array of strings.
 *
 * @param[in] arr
 * @return a copy of arr or NULL on error
 */
static char** dupe_array(char** arr) {
    char** dupeArr = malloc(sizeof(char*));
    int size = 1;
    int i;
    char** tmp;
    for (i = 0; arr[i]; i++) {
        if (i == size - 1) {
            // Double strategy with room for NULL byte at the end
            size *= 2;
            tmp = realloc(dupeArr, sizeof(char*) * size);
            if (!tmp) {
                dupeArr[i] = NULL;
                free_array(dupeArr);
                return NULL;
            }
            dupeArr = tmp;
        }
        dupeArr[i] = strdup(arr[i]);
        if (!dupeArr[i]) {
            free_array(dupeArr);
            return NULL;
        }
    }
    dupeArr[i] = NULL;
    return dupeArr;
}

/**
 * Wrapper around fclose() to first check if file is NULL.
 *
 * @param[in] file
 */
static void safe_fclose(FILE* file) {
    if (file) {
        fclose(file);
    }
}

/**
 * Creates all pipes specified in opts.
 *
 * @param[in,out] opts
 * @return 0 on success, -1 on error
 */
static int sp_create_pipes(SP_Opts* opts) {
    int cond = !sp_pipe_create(&opts->stdin, opts->nonBlockingPipes) &&
               !sp_pipe_create(&opts->stdout, opts->nonBlockingPipes) &&
               !sp_pipe_create(&opts->stderr, opts->nonBlockingPipes);
    return cond ? 0 : -1;
}

/**
 * Checks if the redirection order is valid and fixes it if not.
 * Also sets the default order.
 *
 * @param[in,out] order
 */
static void check_redirect_order(SP_RedirTarget order[3]) {
    if (order[0] == order[1] || order[0] == order[2] || order[1] == order[2]) {
        order[0] = SP_STDIN_FILENO;
        order[1] = SP_STDOUT_FILENO;
        order[2] = SP_STDERR_FILENO;
    }
}

/**
 * Converts a redirection target to the corresponding redirection options.
 *
 * @param[in] opts
 * @param[in] target
 * @return the redirection options for the target or NULL if the target is invalid
 */
static SP_RedirOpt* sp_fd_to_redir_opts(SP_Opts* opts, SP_RedirTarget target) {
    switch (target) {
    case SP_STDIN_FILENO:
        return &opts->stdin;
    case SP_STDOUT_FILENO:
        return &opts->stdout;
    case SP_STDERR_FILENO:
        return &opts->stderr;
    default:
        return NULL;
    }
}

/**
 * Redirections stdin, stdout, and stderr as specified in opts.
 *
 * @param[in,out] opts
 * @return 0 on success, -1 on error
 */
static int sp_redirect_all(SP_Opts* opts) {
    check_redirect_order(opts->redirOrder);
    for (int i = 0; i < SP_SIZE_FIXED_ARR(opts->redirOrder); i++) {
        SP_RedirOpt* redirOpt = sp_fd_to_redir_opts(opts, opts->redirOrder[i]);
        int err = sp_redirect(redirOpt, opts->redirOrder[i]);
        if (err < 0) {
            return err;
        }
    }
    return 0;
}

/**
 * Processes and applies the options for the child process.
 *
 * @param[in,out] opts
 * @return 0 on success, -1 on error
 */
static int sp_handle_child_opts(SP_Opts* opts) {
    if (!opts) {
        return 0;
    }
    if (opts->cwd && chdir(opts->cwd) < 0) {
        SP_ERROR_MSG("cwd: chdir: %s", opts->cwd);
        return -1;
    }
    if (opts->detach && setsid() < 0) {
        SP_ERROR_MSG("detach: setsid");
        return -1;
    }
    if (sp_redirect_all(opts) < 0) {
        // Error output is done in redirect.c since it has access to
        // specific details.
        return -1;
    }
    if (!opts->inheritFds) {
        int fdLimit = sysconf(_SC_OPEN_MAX);
        if (fdLimit < 0) {
            SP_ERROR_MSG(
                "inheritFds: sysconf: unable to determine max number of open "
                "file descriptors");
            return -1;
        }
        for (int i = STDERR_FILENO + 1; i < fdLimit; i++) {
            close(i);
        }

        // // Possibly a better way that only calls close() on file descriptors that are actually open
        // DIR* dir = opendir("/proc/self/fd");
        // if (!dir) {
        //     SP_ERROR_MSG("inheritFds: opendir: /proc/%d/fd", getpid());
        //     return -1;
        // }
        // int dirFd = dirfd(dir);
        // struct dirent* entry;
        // while ((entry = readdir(dir))) {
        //     int fdNum = atoi(entry->d_name);
        //     if (fdNum != dirFd && fdNum > STDERR_FILENO) {
        //         close(fdNum);
        //     }
        // }
        // closedir(dir);
    }
    return 0;
}

/**
 * Sets up and executes the child process.
 *
 * @paramp[in] argv NULL terminated array of arguments
 * @param[in,out] opts Options for the child process
 * @return an error code if exec fails
 */
static int sp_child_exec(char** argv, SP_Opts* opts) {
    if (sp_handle_child_opts(opts) < 0) {
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

/**
 * fdopen()'s the correct end of the pipes and closes the other end.
 * Intended to be called in the parent process after fork()
 *
 * @param proc
 * @param opts options used to setup the process
 * @return 0 on success, -1 on error
 */
static int sp_fdopen_all(SP_Process* proc, SP_Opts* opts) {
    // TODO: remove dupe code
    if (opts->stdin.type == SP_REDIR_PIPE) {
        proc->stdin = sp_pipe_fdopen(opts->stdin.value.pipeFd, true);
        if (!proc->stdin) {
            return -1;
        }
    }
    if (opts->stdout.type == SP_REDIR_PIPE) {
        proc->stdout = sp_pipe_fdopen(opts->stdout.value.pipeFd, false);
        if (!proc->stdout) {
            return -1;
        }
    }
    if (opts->stderr.type == SP_REDIR_PIPE) {
        proc->stderr = sp_pipe_fdopen(opts->stderr.value.pipeFd, false);
        if (!proc->stderr) {
            return -1;
        }
    }
    return 0;
}

SP_Process* sp_run(char** argv, SP_Opts* opts) {
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

SP_Process* sp_open(char** argv, SP_Opts* opts) {
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
    int err;
    switch (proc->pid) {
    case -1:  // FORK ERROR
        sp_destroy(proc);
        return NULL;
    case 0:  // CHILD
        err = sp_child_exec(argv, opts);
        sp_destroy(proc);
        _exit(err);
    default:  // PARENT
        proc->status = SP_STATUS_RUNNING;
        proc->exitCode = -1;
        if (opts && sp_fdopen_all(proc, opts) < 0) {
            sp_destroy(proc);
            return NULL;
        }
        proc->argv = dupe_array(argv);
    }
    return proc;
}

int sp_terminate(SP_Process* proc) {
    return sp_signal(proc, SIGTERM);
}

int sp_kill(SP_Process* proc) {
    return sp_signal(proc, SIGKILL);
}

int sp_signal(SP_Process* proc, int signal) {
    if (!proc || proc->status == SP_STATUS_DEAD) {
        errno = EINVAL;
        return -1;
    }
    return kill(proc->pid, signal);
}

void sp_close(SP_Process* proc) {
    safe_fclose(proc->stdin);
    proc->stdin = NULL;
}

/**
 * Wait for a process to finish with options passed to waitpid()
 *
 * @param proc
 * @param options passed to waitpid()
 * @return exit code of process, or -1 on error
 * @see sp_wait
 * @see sp_poll
 */
static int sp_wait_opts(SP_Process* proc, int options) {
    if (!proc) {
        errno = EINVAL;
        return -1;
    }
    if (proc->status == SP_STATUS_DEAD) {
        return proc->exitCode;
    }
    int stat;
    int pid = waitpid(proc->pid, &stat, options);
    if (pid <= 0) {
        return -1;
    }
    if (WIFEXITED(stat)) {
        proc->exitCode = WEXITSTATUS(stat);
    } else if (WIFSIGNALED(stat)) {
        proc->exitCode = WTERMSIG(stat) + SP_SIGNAL_OFFSET;
    }
    proc->status = SP_STATUS_DEAD;
    return proc->exitCode;
}

int sp_wait(SP_Process* proc) {
    return sp_wait_opts(proc, 0);
}

int sp_poll(SP_Process* proc) {
    return sp_wait_opts(proc, WNOHANG);
}

void sp_destroy(SP_Process* proc) {
    if (!proc) {
        return;
    }
    if (proc->status == SP_STATUS_RUNNING) {
        sp_kill(proc);
        sp_wait(proc);
    }
    free_array(proc->argv);
    safe_fclose(proc->stdin);
    safe_fclose(proc->stderr);
    safe_fclose(proc->stdout);
    free(proc);
}
