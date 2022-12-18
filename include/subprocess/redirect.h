#ifndef SP_REDIRECT_H
#define SP_REDIRECT_H

#include <stdbool.h>
#include <unistd.h>

typedef enum sp_redir_target {
    SP_STDIN_FILENO = STDIN_FILENO,
    SP_STDOUT_FILENO = STDOUT_FILENO,
    SP_STDERR_FILENO = STDERR_FILENO
} SP_RedirTarget;

typedef enum sp_redir_type {
    SP_REDIR_INHERIT = 0,  // Default, inherit from parent.
    SP_REDIR_PATH,         // Redirect from/to to the file named filename
    SP_REDIR_APPEND,  // Append output. If used on stdin it is the same as SP_REDIR_PATH
    SP_REDIR_DEVNULL,  // Redirect to /dev/null
    SP_REDIR_FD,       // Redirect from/to the given fd
    SP_REDIR_PIPE,  // Redirect from/to a pipe connected to parent that will be opened as a FILE*.
    // Only valid for stdin.
    SP_REDIR_BYTES,  // Redirect from a byte stream.
    // Only valid for stdout.
    SP_REDIR_STDERR,  // Redirect stdout to stderr
    // Only valid for stderr.
    SP_REDIR_STDOUT,  // Redirect stderr to stdout.
} SP_RedirType;

typedef struct sp_redir_opt {
    SP_RedirType type;
    union {
        char* path;
        int fd;
        void* bytes;
        int pipeFd[2];
    } value;
    size_t size;
} SP_RedirOpt;

// Connect a pipe from/to the child
#define SP_REDIR_PIPE() \
    (SP_RedirOpt) { .type = SP_REDIR_PIPE }

// Redirect to/from a file path
#define SP_REDIR_PATH(_path) \
    (SP_RedirOpt) { .type = SP_REDIR_PATH, .value.path = (_path) }

// Redirect to/from a file path appending the output.
// If used on stdin it is the same as SP_REDIR_PATH
#define SP_REDIR_APPEND(_path) \
    (SP_RedirOpt) { .type = SP_REDIR_APPEND, .value.path = (_path) }

// Redirect to/from /dev/null
#define SP_REDIR_DEVNULL() \
    (SP_RedirOpt) { .type = SP_REDIR_PATH, .value.path = "/dev/null" }

// Redirect to/from the given FILE*
#define SP_REDIR_FILE(_file)                                        \
    (SP_RedirOpt) {                                                 \
        .type = SP_REDIR_FD, .value.fd = _file ? fileno(_file) : -1 \
    }

// Redirect to/from the given FD
#define SP_REDIR_FD(_fd) \
    (SP_RedirOpt) { .type = SP_REDIR_FD, .value.fd = (_fd) }

// Redirect stdin from a byte stream
#define SP_REDIR_BYTES(_bytes, _size)                                    \
    (SP_RedirOpt) {                                                      \
        .type = SP_REDIR_BYTES, .value.bytes = (_bytes), .size = (_size) \
    }

// Redirect stderr to stdout
#define SP_REDIR_STDOUT() \
    (SP_RedirOpt) { .type = SP_REDIR_STDOUT }

// Redirect stdout to stderr
#define SP_REDIR_STDERR() \
    (SP_RedirOpt) { .type = SP_REDIR_STDERR }

// Redirect stdin/stdout/stderr
int sp_redirect(SP_RedirOpt* opts, SP_RedirTarget target);

#endif  // SP_REDIRECT_H
