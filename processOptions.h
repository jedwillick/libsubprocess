#ifndef PROCESS_OPTIONS_H
#define PROCESS_OPTIONS_H

typedef enum RedirectType {
    R_NONE,
    R_PIPE,
    R_FD,
    R_FILE,
    R_DEVNULL,
    R_TO_STDOUT,  // Redirect stderr to stdout
    R_TO_STDERR,  // Redirect stdout to stderr
} RedirectType;

typedef struct Redirect {
    RedirectType type;
    char* filename;
    int oldFd;
    int newFd;
    int pipeFds[2];
} Redirect;

typedef struct ProcessOptions {
    char* cwd;
    char** env;
    Redirect stdin;
    Redirect stdout;
    Redirect stderr;
    bool captureOutput;
} ProcessOptions;

#endif  // PROCESS_OPTIONS_H
