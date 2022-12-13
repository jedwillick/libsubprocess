#ifndef SP_ERROR_H
#define SP_ERROR_H

#define SP_EXIT_NOT_EXECUTE 126
#define SP_EXIT_NOT_FOUND   127

#define SP_ERROR_MSG(fmt, ...) \
    fprintf(stderr, "SP: " fmt ": %s\n", ##__VA_ARGS__, strerror(errno))

#endif  // SP_ERROR_H
