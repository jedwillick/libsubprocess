/**
 * @file
 * @brief Pipe API
 */

#ifndef SP_PIPE_H
#define SP_PIPE_H

#include <stdbool.h>
#include <stdio.h>

#include "subprocess/redirect.h"

/**
 * Close the file descriptor and set it to -1.
 * If the fd is already closed (-1) then nothing is done.
 *
 * @param[in,out] fd the file descriptor being closed.
 * @return 0 on success, -1 on error and errno is set by close(2).
 */
int sp_fd_close(int* fd);

/**
 * Create a pipe at opt->value.pipeFd with the O_CLOEXEC flag.
 * If nonBlocking is true the O_NONBLOCK flag is also applied.
 * If opt->type is SP_REDIR_BYTES the data is written to the pipe as soon as it is created,
 * and the write end is closed.
 *
 * @param[in,out] opt the redirect option being changed.
 * @param[in] nonBlocking if true the pipe will be non-blocking.
 * @return 0 on success, -1 on error and errno is set accordingly.
 */
int sp_pipe_create(SP_RedirOpt* opt, bool nonBlocking);

/**
 * Close both ends of the pipe and sets them to -1.
 * If the pipe is already closed silently returns without error.
 *
 * @param[in,out] fd the file descriptors of the pipe being closed.
 * @return 0 on success, -1 on error and errno is set by close(2).
 */
int sp_pipe_close(int fd[2]);

/**
 * Opens a FILE* to either the read or write end of the pipe,
 * and closes the other end.
 *
 * @param[in,out] fd the file descriptors of the pipe being used.
 * @param[in] isInput if true the write end is opened, otherwise the read end is opened.
 * @return a FILE* to the open end of the pipe, or NULL on error and errno is set accordingly.
 */
FILE* sp_pipe_fdopen(int fd[2], bool isInput);

#endif  // SP_PIPE_H
