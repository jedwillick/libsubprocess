/**
 * @file
 * @brief Error handling API
 */

#ifndef SP_ERROR_H
#define SP_ERROR_H

#include <errno.h>
#include <string.h>

/**
 * Exit code when the command invoked cannot execute.
 */
#define SP_EXIT_NOT_EXECUTE 126

/**
 * Exit code when the command invoked cannot be found.
 */
#define SP_EXIT_NOT_FOUND 127

/**
 * Print an error message to stderr, similar to perror(3).
 *
 * @param[in] fmt printf-style format string
 * @param[in] ... format string parameters
 */
#define SP_ERROR_MSG(fmt, ...) \
    fprintf(stderr, "SP: " fmt ": %s\n", ##__VA_ARGS__, strerror(errno))

#endif  // SP_ERROR_H
