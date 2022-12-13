#ifndef SP_IO_H
#define SP_IO_H

#include <stdio.h>

typedef enum sp_io_type {
    SP_IO_INHERIT = 0,  // Default, inherit from parent.
    SP_IO_PATH,         // Redirect from/to to the file named filename
    SP_IO_DEVNULL,      // Redirect to /dev/null
    SP_IO_FD,           // Redirect from/to the given fd
    SP_IO_PIPE,  // Redirect from/to a pipe connected to parent that will be opened as a FILE*.
    // Only valid for stdin.
    SP_IO_BYTES,  // Redirect from a byte stream.
    // Only valid for stdout.
    SP_IO_TO_STDERR,  // Redirect stdout to stderr
    // Only valid for stderr.
    SP_IO_TO_STDOUT,  // Redirect stderr to stdout.
} SP_IOType;

typedef struct sp_io_options {
    SP_IOType type;
    union {
        char* path;
        int fd;
        void* bytes;
        int pipeFd[2];
    } value;
    size_t size;
} SP_IOOptions;

// Connect a pipe from/to the child
#define SP_IO_OPTS_PIPE() \
    (SP_IOOptions) { .type = SP_IO_PIPE }

// Redirect to/from a file path
#define SP_IO_OPTS_PATH(_path) \
    (SP_IOOptions) { .type = SP_IO_PATH, .value.path = (_path) }

// Redirect to/from /dev/null
#define SP_IO_OPTS_DEVNULL() \
    (SP_IOOptions) { .type = SP_IO_PATH, .value.path = "/dev/null" }

// Redirect to/from the given FILE*
#define SP_IO_OPTS_FILE(_file) \
    (SP_IOOptions) { .type = SP_IO_FD, .value.fd = _file ? fileno(_file) : -1 }

// Redirect to/from the given FD
#define SP_IO_OPTS_FD(_fd) \
    (SP_IOOptions) { .type = SP_IO_FD, .value.fd = (_fd) }

// Redirect stdin from a byte stream
#define SP_IO_OPTS_BYTES(_bytes, _size)                               \
    (SP_IOOptions) {                                                  \
        .type = SP_IO_BYTES, .value.bytes = (_bytes), .size = (_size) \
    }

// Redirect stderr to stdout
#define SP_IO_OPTS_TO_STDOUT() \
    (SP_IOOptions) { .type = SP_IO_TO_STDOUT }

// Redirect stdout to stderr
#define SP_IO_OPTS_TO_STDERR() \
    (SP_IOOptions) { .type = SP_IO_TO_STDERR }

#endif  // SP_IO_H
