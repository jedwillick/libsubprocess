#define _GNU_SOURCE  // for pipe2()

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

int sp_pipe_close(int fd[2]) {
    return !sp_fd_close(&fd[0]) && !sp_fd_close(&fd[1]) ? 0 : -1;
}

int sp_pipe_create(SP_RedirOpt* opt, bool nonBlocking) {
    if (!opt || !(opt->type == SP_REDIR_PIPE || opt->type == SP_REDIR_BYTES)) {
        return 0;
    }
    int flags = O_CLOEXEC | (nonBlocking ? O_NONBLOCK : 0);
    int fd[2];
    if (pipe2(fd, flags) < 0) {
        return -1;
    }
    if (opt->type == SP_REDIR_BYTES) {  // TODO: need better solution
        if (write(fd[1], opt->value.bytes, opt->size) < 0) {
            int tmpErrno = errno;
            sp_pipe_close(opt->value.pipeFd);
            errno = tmpErrno;
            return -1;
        }
        sp_fd_close(&fd[1]);
    }
    opt->value.pipeFd[0] = fd[0];
    opt->value.pipeFd[1] = fd[1];
    return 0;
}
