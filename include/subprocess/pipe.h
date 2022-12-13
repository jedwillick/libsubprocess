#ifndef SP_PIPE_H
#define SP_PIPE_H

#include <stdbool.h>

#include "subprocess/io.h"

// Create a pipe if BYTES is specified it will write them to the newly created pipe.
int sp_pipe_create(SP_IOOptions* opts);

// close pipe
int sp_pipe_close(int fd[2]);

// Return either the read or write end of the pipe
// and close the other end.
FILE* sp_pipe_fdopen(int fd[2], bool isInput);

#endif  // SP_PIPE_H
