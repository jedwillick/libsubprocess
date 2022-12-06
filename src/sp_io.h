#ifndef SP_IO_H
#define SP_IO_H

#include <stdio.h>

typedef enum sp_io_type {
    SP_IO_INHERIT = 0,  // Default, inherit from parent.
    SP_IO_PATH,         // Redirect from/to to the file named filename
    SP_IO_DEVNULL,      // Redirect to /dev/null
    SP_IO_FD,           // Redirect from/to the given fd
    SP_IO_PIPE,         // Redirect from/to a pipe connected to parent that will be opened as a FILE*.
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
#define sp_io_options_pipe() \
    (SP_IOOptions) { .type = SP_IO_PIPE }

// Redirect to/from a file path
#define sp_io_options_path(_path) \
    (SP_IOOptions) { .type = SP_IO_PATH, .value.path = (_path) }

// Redirect to/from /dev/null
#define sp_io_options_devnull() \
    (SP_IOOptions) { .type = SP_IO_PATH, .value.path = "/dev/null" }

// Redirect to/from the given FILE*
#define sp_io_options_file(_file) \
    (SP_IOOptions) { .type = SP_IO_FD, .value.fd = fileno((_file)) }

// Redirect to/from the given FD
#define sp_io_options_fd(_fd) \
    (SP_IOOptions) { .type = SP_IO_FD, .value.fd = (_fd) }

// Redirect stdin from a byte stream
#define sp_io_options_bytes(_bytes, _size)                            \
    (SP_IOOptions) {                                                  \
        .type = SP_IO_BYTES, .value.bytes = (_bytes), .size = (_size) \
    }

// Redirect stderr to stdout
#define sp_io_options_to_stdout() \
    (SP_IOOptions) { .type = SP_IO_TO_STDOUT }

// Redirect stdout to stderr
#define sp_io_options_to_stderr() \
    (SP_IOOptions) { .type = SP_IO_TO_STDERR }

#endif  // SP_IO_H
