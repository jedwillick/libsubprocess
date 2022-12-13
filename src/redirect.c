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

static int sp_freopen(char* path, SP_RedirectTarget target) {
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    switch (target) {
        case SP_REDIRECT_STDIN:
            return freopen(path, "r", stdin) ? 0 : -1;
        case SP_REDIRECT_STDOUT:
            return freopen(path, "w", stdout) ? 0 : -1;
        case SP_REDIRECT_STDERR:
            return freopen(path, "w", stderr) ? 0 : -1;
        default:
            errno = EINVAL;
            return -1;
    }
}

// Redirect stdin/stdout/stderr
// Should be called in the child
// On success returns 0,
// On failure outputs error information, sets errno and returns -1
int sp_redirect(SP_IOOptions* opts, SP_RedirectTarget target) {
    if (!opts) {
        errno = EINVAL;
        SP_ERROR_MSG("redirect: opts=%p, target=%d", (void*)opts, target);
        return -1;
    }
    int err = 0;
    size_t msgLen = 1024;
    char msg[msgLen];
    msg[0] = 0;
    switch (opts->type) {
        case SP_IO_INHERIT:
            err = 0;
            break;
        case SP_IO_DEVNULL:
            // fallthrough
        case SP_IO_PATH:
            err = sp_freopen(opts->value.path, target);
            snprintf(msg, msgLen, "PATH: %s", opts->value.path);
            break;
        case SP_IO_FD:
            err = sp_dup2_close(opts->value.fd, target);
            snprintf(msg, msgLen, "FD: %d", opts->value.fd);
            break;
        case SP_IO_PIPE:
            err = sp_dup2_pipe(opts->value.pipeFd, target);
            snprintf(msg, msgLen, "PIPE: [%d,%d]", opts->value.pipeFd[0],
                    opts->value.pipeFd[1]);
            break;
        case SP_IO_BYTES:
            err = sp_dup2_close(opts->value.pipeFd[0], target);
            snprintf(msg, msgLen, "BYTES: %d", opts->value.pipeFd[0]);
            break;
        case SP_IO_TO_STDERR:
            snprintf(msg, msgLen, "TO_STDERR: %d", target);
            if (target != SP_REDIRECT_STDOUT) {
                errno = EINVAL;
                err = -1;
            } else {
                err = sp_dup2(SP_REDIRECT_STDERR, SP_REDIRECT_STDOUT);
            }
            break;
        case SP_IO_TO_STDOUT:
            snprintf(msg, msgLen, "TO_STDOUT: %d", target);
            if (target != SP_REDIRECT_STDERR) {
                errno = EINVAL;
                err = -1;
            } else {
                err = sp_dup2(SP_REDIRECT_STDOUT, SP_REDIRECT_STDERR);
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
