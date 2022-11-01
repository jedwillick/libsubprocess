#define _GNU_SOURCE
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "processOptions.h"

typedef enum {
    STATUS_RUNNING,
    // STATUS_STOPPED,
    // STATUS_ZOMBIE,
    STATUS_DEAD,
    STATUS_UNKNOWN
} Status;

typedef struct Process {
    pid_t pid;
    Status status;
    int exitCode;
    FILE* stdout;
    FILE* stderr;
    FILE* stdin;
    char* stdoutBytes;
    char* stderrBytes;
    char** args;
    ProcessOptions* options;
} Process;

void process_destroy(Process* process);

int close_fd(int* fd) {
    int err = 0;
    if (!fd || *fd < 0) {
        return err;
    }
    err = close(*fd);
    *fd = -1;
    return err;
}

int close_pipe(int pipe[2]) {
    return close_fd(&pipe[0]) | close_fd(&pipe[1]);
}

int fclose_null(FILE** file) {
    int err = 0;
    if (!file || !*file) {
        return err;
    }
    err = fclose(*file);
    *file = NULL;
    return err;
}

int create_pipe(int pipeFds[2]) {
    int err;
    if ((err = pipe2(pipeFds, O_CLOEXEC))) {
        perror("pipe");
    }
    return err;
}

int setup_pipe(Redirect* settings) {
    return settings->type == R_PIPE ? create_pipe(settings->pipeFds) : 0;
}

int setup_pipes(ProcessOptions* options) {
    if (options->captureOutput) {
        options->stdout.type = R_PIPE;
        options->stderr.type = R_PIPE;
    }
    return setup_pipe(&options->stdin) | setup_pipe(&options->stdout) |
           setup_pipe(&options->stderr);
}

int handle_redirect(Redirect* settings, int newFd) {
    int err = 0;
    switch (settings->type) {
        case R_NONE:
            break;
        case R_PIPE:
            err = dup2(newFd == STDIN_FILENO ? settings->pipeFds[0]
                                             : settings->pipeFds[1],
                          newFd) >= 0
                          ? 0
                          : -1;
            err |= close_pipe(settings->pipeFds);
            break;
        case R_FD:
            if (settings->oldFd < 0) {
                err = -1;
            } else {
                err = dup2(settings->oldFd, newFd) >= 0 ? 0 : -1;
                err |= close(settings->oldFd);
            }
            break;
        case R_DEVNULL:
            settings->filename = "/dev/null";
            // fallthrough
        case R_FILE:
            if (!settings->filename) {
                err = -1;
                break;
            }
            switch (newFd) {
                case STDIN_FILENO:
                    err = freopen(settings->filename, "r", stdin) ? 0 : -1;
                    break;
                case STDOUT_FILENO:
                    err = freopen(settings->filename, "w", stdout) ? 0 : -1;
                    break;
                case STDERR_FILENO:
                    err = freopen(settings->filename, "w", stderr) ? 0 : -1;
                    break;
                default:
                    err = -1;
            }
            break;
        case R_TO_STDOUT:
            err = newFd == STDERR_FILENO ? dup2(STDOUT_FILENO, newFd) : -1;
            break;
        case R_TO_STDERR:
            err = newFd == STDOUT_FILENO ? dup2(STDERR_FILENO, newFd) : -1;
            break;
    }
    return err;
}

int handle_redirects(ProcessOptions* options) {
    return handle_redirect(&options->stdin, STDIN_FILENO) |
           handle_redirect(&options->stdout, STDOUT_FILENO) |
           handle_redirect(&options->stderr, STDERR_FILENO);
}

int fdopen_pipe(FILE** file, Redirect* settings, int newFd) {
    if (settings->type != R_PIPE) {
        return 0;
    }
    int err;
    if (newFd == STDIN_FILENO) {
        *file = fdopen(settings->pipeFds[1], "w");
        err = close_fd(&settings->pipeFds[0]);
    } else {
        *file = fdopen(settings->pipeFds[0], "r");
        err = close_fd(&settings->pipeFds[1]);
    }
    return *file && !err ? 0 : -1;
}

int fdopen_pipes(Process* process) {
    return fdopen_pipe(&process->stdout, &process->options->stdout,
                   STDOUT_FILENO) |
           fdopen_pipe(&process->stderr, &process->options->stderr,
                   STDERR_FILENO) |
           fdopen_pipe(&process->stdin, &process->options->stdin,
                   STDIN_FILENO);
}

Process* process_exec(char** args, ProcessOptions* options) {
    Process* process = calloc(1, sizeof(Process));
    process->exitCode = -1;
    process->args = args;
    process->status = STATUS_RUNNING;
    process->options = options;
    if (setup_pipes(options)) {
        perror("setup pipes");
        process_destroy(process);
        return NULL;
    }
    process->pid = fork();
    if (!process->pid) {
        if (handle_redirects(options)) {
            _exit(99);
        }
        // Child
        if (options->cwd) {
            chdir(options->cwd);
        }
        if (options->env) {
            execve(args[0], args, options->env);
        } else {
            execvp(args[0], args);
        }
        _exit(99);
    }
    fdopen_pipes(process);
    return process;
}

char* read_file(FILE* file) {
    size_t readSize = 16;
    char* buffer = malloc(readSize);
    size_t size = readSize + 1;
    char* content = calloc(1, size);
    while (true) {
        size_t bytesRead = fread(buffer, 1, readSize, file);
        if (bytesRead <= 0) {
            break;
        }
        strncat(content, buffer, bytesRead);
        if (bytesRead != readSize) {
            break;
        }
        size += readSize;
        content = realloc(content, size);
    }
    free(buffer);
    content[size - 1] = '\0';
    return content;
}

Process* process_run(char** args, ProcessOptions* options) {
    Process* process = process_exec(args, options);
    // Parent
    int status;
    waitpid(process->pid, &status, 0);
    if (options->captureOutput) {
        process->stdoutBytes = read_file(process->stdout);
        process->stderrBytes = read_file(process->stderr);
        fclose_null(&process->stdout);
        fclose_null(&process->stderr);
    }
    if (WIFEXITED(status)) {
        process->status = STATUS_DEAD;
        process->exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        process->status = STATUS_DEAD;
        process->exitCode = WTERMSIG(status) + 128;
    } else {
        process->status = STATUS_UNKNOWN;
    }
    return process;
}

void process_destroy(Process* process) {
    if (!process) {
        return;
    }
    fclose_null(&process->stdin);
    fclose_null(&process->stdout);
    fclose_null(&process->stderr);
    free(process->stdoutBytes);
    free(process->stderrBytes);
    free(process);
}

int main(int argc, char** argv) {
    // ProcessOptions options = {0};
    // options.captureOutput = true;
    // Process* process = process_run(argv + 1, &options);
    // printf("Process exited with code %d\n", process->exitCode);
    // printf("Stdout '%s'\n", process->stdoutBytes);
    // printf("Stderr '%s'\n", process->stderrBytes);
    // process_destroy(process);

    // cat $argv[1] | sort -r | grep proc
    ProcessOptions opts1 = {0};
    opts1.stdin.type = R_FILE;
    opts1.stdin.filename = argv[1];
    opts1.stdout.type = R_PIPE;
    char* args1[] = {"cat", 0};
    Process* p1 = process_run(args1, &opts1);
    ProcessOptions opts2 = {0};
    opts2.stdout.type = R_PIPE;
    opts2.stdin.type = R_FD;
    opts2.stdin.oldFd = fileno(p1->stdout);
    char* args2[] = {"sort", "-r", 0};
    Process* p2 = process_run(args2, &opts2);
    ProcessOptions opts3 = {0};
    opts3.stdin.type = R_FD;
    opts3.stdin.oldFd = fileno(p2->stdout);
    opts3.captureOutput = true;
    char* args3[] = {"grep", "proc", 0};
    Process* p3 = process_run(args3, &opts3);
    printf("%s exited with code %d\n", p1->args[0], p1->exitCode);
    printf("%s exited with code %d\n", p2->args[0], p2->exitCode);
    printf("%s exited with code %d\n", p3->args[0], p3->exitCode);
    printf("Stdout '%s'\n", p3->stdoutBytes);
    printf("Stderr '%s'\n", p3->stderrBytes);
    process_destroy(p1);
    process_destroy(p2);
    process_destroy(p3);
}
