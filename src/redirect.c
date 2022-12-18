#include "subprocess/redirect.h"

#include <errno.h>
#include <string.h>

#include "subprocess/error.h"
#include "subprocess/pipe.h"

static int sp_dup2(int oldFd, int newFd) {
    return dup2(oldFd, newFd) < 0 ? -1 : 0;
}

static int sp_dup2_close(int oldFd, int newFd) {
    return !sp_dup2(oldFd, newFd) && !close(oldFd) ? 0 : -1;
}

// dup2 the one end of the pipe and close them both.
static int sp_dup2_pipe(int pipeFd[2], int newFd) {
    int oldFd = !newFd ? pipeFd[0] : pipeFd[1];
    return !sp_dup2(oldFd, newFd) && !sp_pipe_close(pipeFd) ? 0 : -1;
}

static int sp_freopen(char* path, SP_RedirTarget target, bool append) {
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    switch (target) {
        case SP_STDIN_FILENO:
            return freopen(path, "r", stdin) ? 0 : -1;
        case SP_STDOUT_FILENO:
            return freopen(path, append ? "a" : "w", stdout) ? 0 : -1;
        case SP_STDERR_FILENO:
            return freopen(path, append ? "a" : "w", stderr) ? 0 : -1;
        default:
            errno = EINVAL;
            return -1;
    }
}

// Redirect stdin/stdout/stderr
// Should be called in the child
// On success returns 0,
// On failure outputs error information, sets errno and returns -1
int sp_redirect(SP_RedirOpt* opts, SP_RedirTarget target) {
    if (!opts) {
        errno = EINVAL;
        SP_ERROR_MSG("redirect: opts=%p, target=%d", (void*)opts, target);
        return -1;
    }
    bool append = false;
    int err = 0;
    size_t msgLen = 1024;
    char msg[msgLen];
    msg[0] = 0;
    switch (opts->type) {
        case SP_REDIR_INHERIT:
            err = 0;
            break;
        case SP_REDIR_APPEND:
            append = true;
            // fallthrough
        case SP_REDIR_DEVNULL:
            // fallthrough
        case SP_REDIR_PATH:
            err = sp_freopen(opts->value.path, target, append);
            snprintf(msg, msgLen, "PATH: %s", opts->value.path);
            break;
        case SP_REDIR_FD:
            err = sp_dup2_close(opts->value.fd, target);
            snprintf(msg, msgLen, "FD: %d", opts->value.fd);
            break;
        case SP_REDIR_PIPE:
            err = sp_dup2_pipe(opts->value.pipeFd, target);
            snprintf(msg, msgLen, "PIPE: [%d,%d]", opts->value.pipeFd[0],
                opts->value.pipeFd[1]);
            break;
        case SP_REDIR_BYTES:
            err = sp_dup2_close(opts->value.pipeFd[0], target);
            snprintf(msg, msgLen, "BYTES: %d", opts->value.pipeFd[0]);
            break;
        case SP_REDIR_STDERR:
            snprintf(msg, msgLen, "STDERR: %d", target);
            if (target != SP_STDOUT_FILENO) {
                errno = EINVAL;
                err = -1;
            } else {
                err = sp_dup2(SP_STDERR_FILENO, SP_STDOUT_FILENO);
            }
            break;
        case SP_REDIR_STDOUT:
            snprintf(msg, msgLen, "STDOUT: %d", target);
            if (target != SP_STDERR_FILENO) {
                errno = EINVAL;
                err = -1;
            } else {
                err = sp_dup2(SP_STDOUT_FILENO, SP_STDERR_FILENO);
            }
            break;
        default:
            errno = EINVAL;
            err = -1;
    }
    if (err) {
        SP_ERROR_MSG("redirect: %s", msg);
    }
    return err;
}
