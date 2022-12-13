#include "subprocess/pipe.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int sp_fd_close(int* fd) {
    if (!fd || *fd < 0) {
        return 0;
    }
    int err = close(*fd);
    *fd = -1;
    return err;
}

// Return either the read or write end of the pipe
// and close the other end.
FILE* sp_pipe_fdopen(int fd[2], bool isInput) {
    FILE* file;
    if (isInput) {
        file = fdopen(fd[1], "w");
        if (file) {
            sp_fd_close(&fd[0]);
        }
    } else {
        file = fdopen(fd[0], "r");
        if (file) {
            sp_fd_close(&fd[1]);
        }
    }
    return file;
}

// close pipe
int sp_pipe_close(int fd[2]) {
    return !sp_fd_close(&fd[0]) && !sp_fd_close(&fd[1]) ? 0 : -1;
}

// Create a pipe if BYTES is specified it will write them to the newly created pipe.
// and close the end.
// Called BEFORE fork()
// If opts is not setup correctly silently ignore. TODO: fix this
int sp_pipe_create(SP_IOOptions* opts) {
    if (!opts || !(opts->type == SP_IO_PIPE || opts->type == SP_IO_BYTES)) {
        return 0;
    }
    int fd[2];
    if (pipe(fd) < 0) {
        return -1;
    }
    fcntl(fd[0], F_SETFD, FD_CLOEXEC);
    fcntl(fd[1], F_SETFD, FD_CLOEXEC);
    if (opts->type == SP_IO_BYTES) {  // TODO: need better solution
        if (write(fd[1], opts->value.bytes, opts->size) < 0) {
            int tmpErrno = errno;
            sp_pipe_close(opts->value.pipeFd);
            errno = tmpErrno;
            return -1;
        }
        sp_fd_close(&fd[1]);
    }
    opts->value.pipeFd[0] = fd[0];
    opts->value.pipeFd[1] = fd[1];
    return 0;
}
