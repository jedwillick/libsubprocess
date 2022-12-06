
#include "sp_redirect.h"

#include "sp_pipe.h"

static int sp_dup2(int oldFd, int newFd) {
    return dup2(oldFd, newFd) < 0 ? -1 : 0;
}

static int sp_dup2_close(int oldFd, int newFd) {
    int err = sp_dup2(oldFd, newFd);
    err |= close(oldFd);
    return err;
}

static int sp_dup2_pipe(int pipeFd[2], int newFd) {
    int oldFd = !newFd ? pipeFd[0] : pipeFd[1];
    int err = sp_dup2(oldFd, newFd);
    err |= sp_pipe_close(pipeFd);
    return err;
}

static int sp_freopen(char* path, SP_RedirectTarget target) {
    switch (target) {
        case SP_REDIRECT_STDIN:
            return freopen(path, "r", stdin) ? 0 : -1;
        case SP_REDIRECT_STDOUT:
            return freopen(path, "w", stdout) ? 0 : -1;
        case SP_REDIRECT_STDERR:
            return freopen(path, "w", stderr) ? 0 : -1;
        default:
            return -1;
    }
}

// Redirect stdin/stdout/stderr
// Should be called in the child
int sp_redirect(SP_IOOptions* opts, SP_RedirectTarget target) {
    if (!opts) {
        return 0;
    }
    switch (opts->type) {
        case SP_IO_INHERIT:
            return 0;
        case SP_IO_DEVNULL:
            // fallthrough
        case SP_IO_PATH:
            return sp_freopen(opts->value.path, target);
        case SP_IO_FD:
            return sp_dup2_close(opts->value.fd, target);
        case SP_IO_PIPE:
            return sp_dup2_pipe(opts->value.pipeFd, target);
        case SP_IO_BYTES:
            return sp_dup2_close(opts->value.pipeFd[0], target);
        case SP_IO_TO_STDERR:
            if (target != SP_REDIRECT_STDOUT) {
                return -1;
            }
            return sp_dup2(SP_REDIRECT_STDERR, SP_REDIRECT_STDOUT);
        case SP_IO_TO_STDOUT:
            if (target != SP_REDIRECT_STDERR) {
                return -1;
            }
            return sp_dup2(SP_REDIRECT_STDOUT, SP_REDIRECT_STDERR);
        default:
            return -1;
    }
}
