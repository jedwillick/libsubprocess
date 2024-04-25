/**
 * @file
 * @brief File Redirection API
 */

#ifndef SP_REDIRECT_H
#define SP_REDIRECT_H

#include <stdbool.h>
#include <unistd.h>

/**
 * Standard redirect targets.
 * Synonymous to STDIN_FILENO, STDOUT_FILENO, and STDERR_FILENO
 */
typedef enum sp_redir_target {
    SP_STDIN_FILENO = STDIN_FILENO,
    SP_STDOUT_FILENO = STDOUT_FILENO,
    SP_STDERR_FILENO = STDERR_FILENO
} SP_RedirTarget;

/**
 * Different types of redirection.
 *
 * sp_redir_type::SP_REDIR_BYTES is only valid for stdin.
 * sp_redir_type::SP_REDIR_STDERR is only valid for stdout.
 * sp_redir_type::SP_REDIR_STDOUT is only valid for stderr.
 *
 * @see sp_redir_opt
 */
typedef enum sp_redir_type {
    SP_REDIR_INHERIT = 0,  ///< Default, inherit from parent.
    SP_REDIR_APPEND,  ///< Append output. If used on stdin it is the same as SP_REDIR_PATH
    SP_REDIR_PATH,     ///< Redirect to to the file named filename
    SP_REDIR_DEVNULL,  ///< Redirect to /dev/null
    SP_REDIR_FD,       ///< Redirect to the given fd
    SP_REDIR_PIPE,  ///< Redirect to a pipe connected to parent that will be opened as a FILE*.
    SP_REDIR_BYTES,   ///< Redirect a byte stream to stdin.
    SP_REDIR_STDERR,  ///< Redirect stdout to stderr
    SP_REDIR_STDOUT,  ///< Redirect stderr to stdout.
} SP_RedirType;

/**
 * Struct used to configure redirection.
 *
 * It is recommended to use the SP_REDIR_* macros below for configuring the struct.
 * @see redirect.h
 */
typedef struct sp_redir_opt {
    SP_RedirType type;  ///< The type of redirection.
    union {
        char* path;     ///< Redirecting to a file path.
        int fd;         ///< Redirecting to a file descriptor.
        void* bytes;    ///< Redirecting to a byte stream.
        int pipeFd[2];  ///< Redirecting to a pipe.
    } value;            ///< The value of the redirection.
    size_t size;
} SP_RedirOpt;

/**
 * Setup sp_redir_opt to redirect to a pipe.
 */
#define SP_REDIR_PIPE()       \
    (SP_RedirOpt) {           \
        .type = SP_REDIR_PIPE \
    }

/**
 * Setup sp_redir_opt to redirect to a file path.
 *
 * @param[in] _path char* holding the file path to redirect to.
 */
#define SP_REDIR_PATH(_path)                         \
    (SP_RedirOpt) {                                  \
        .type = SP_REDIR_PATH, .value.path = (_path) \
    }

/**
 * Setup sp_redir_opt to redirect to a file path appending the output.
 * If used on stdin it is the same as SP_REDIR_PATH
 *
 * @param[in] _path char* holding the file path to redirect to.
 */
#define SP_REDIR_APPEND(_path)                         \
    (SP_RedirOpt) {                                    \
        .type = SP_REDIR_APPEND, .value.path = (_path) \
    }

/**
 * Setup sp_redir_opt to redirect to /dev/null.
 */
#define SP_REDIR_DEVNULL()                               \
    (SP_RedirOpt) {                                      \
        .type = SP_REDIR_PATH, .value.path = "/dev/null" \
    }

/**
 * Setup sp_redir_opt to redirect to a FILE*
 *
 * @param[in] _file FILE*
 */
#define SP_REDIR_FILE(_file)                                        \
    (SP_RedirOpt) {                                                 \
        .type = SP_REDIR_FD, .value.fd = _file ? fileno(_file) : -1 \
    }

/**
 * Setup sp_redir_opt to redirect to a file descriptor
 *
 * @param[in] _fd int
 */
#define SP_REDIR_FD(_fd)                       \
    (SP_RedirOpt) {                            \
        .type = SP_REDIR_FD, .value.fd = (_fd) \
    }

/**
 * Setup sp_redir_opt to redirect to a byte stream pointed to by _bytes having a size of _size.
 * Only valid for stdin.
 *
 * @param[in] _bytes void* A byte stream
 * @param[in] _size size_t The size of the byte stream
 */
#define SP_REDIR_BYTES(_bytes, _size)                                    \
    (SP_RedirOpt) {                                                      \
        .type = SP_REDIR_BYTES, .value.bytes = (_bytes), .size = (_size) \
    }

/**
 * Setup sp_redir_opt to redirect stderr to stdout.
 * Only valid for stderr.
 */
#define SP_REDIR_STDOUT()       \
    (SP_RedirOpt) {             \
        .type = SP_REDIR_STDOUT \
    }

/**
 * Setup sp_redir_opt to redirect stdout to stderr.
 * Only valid for stdout.
 */
#define SP_REDIR_STDERR()       \
    (SP_RedirOpt) {             \
        .type = SP_REDIR_STDERR \
    }

/**
 * Redirect the target file descriptor using the given opts.
 * This function is intended to be used from within the child process before calling exec.
 * If an error is encountered an error message is emmitted to stderr.
 *
 * @param[in,out] opts
 * @param[in] target
 * @return 0 on success, or -1 on error and errno is set accordingly.
 * @see sp_redir_opt
 * @see sp_redir_target
 * @see sp_redir_type
 */
int sp_redirect(SP_RedirOpt* opts, SP_RedirTarget target);

#endif  // SP_REDIRECT_H
