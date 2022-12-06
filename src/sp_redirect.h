#ifndef SP_REDIRECT_H
#define SP_REDIRECT_H

#include <stdbool.h>
#include <unistd.h>

#include "sp_io.h"

typedef enum sp_redirect_target {
    SP_REDIRECT_STDIN = STDIN_FILENO,
    SP_REDIRECT_STDOUT = STDOUT_FILENO,
    SP_REDIRECT_STDERR = STDERR_FILENO
} SP_RedirectTarget;

// Redirect stdin/stdout/stderr
int sp_redirect(SP_IOOptions* opts, SP_RedirectTarget target);

#endif  // SP_PIPE_H
