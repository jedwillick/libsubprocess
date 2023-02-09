/**
 * @file
 * @brief Process API
 */

#ifndef SP_PROCESS_H
#define SP_PROCESS_H

#include <stdio.h>
#include <unistd.h>

#include "subprocess/error.h"
#include "subprocess/pipe.h"
#include "subprocess/redirect.h"

/**
 * Offset added to signal numbers when setting the exit code.
 */
#define SP_SIGNAL_OFFSET 128

/**
 * The statuses a process can have.
 */
typedef enum sp_status {
    SP_STATUS_SPAWNING = 0,  ///< process is still being spawned.
    SP_STATUS_RUNNING,       ///< process is still running.
    SP_STATUS_DEAD,          ///< process has terminated and has been waited on.
} SP_Status;

/**
 * A struct containing data pertintent to a process.
 * sp_process::stdin, sp_process::stdout, and sp_process::stderr
 * are only opened if the corresponding option in sp_opts
 * specifies sp_redir_type::SP_REDIR_PIPE.
 *
 * @see sp_destroy
 */
typedef struct sp_process {
    pid_t pid;         ///< process id
    char** argv;       ///< deep clone of argv
    SP_Status status;  ///< status of process
    int exitCode;  ///< exit code of process or -1 if status != SP_STATUS_DEAD
    FILE* stdin;   ///< stdin of process
    FILE* stdout;  ///< stdout of process
    FILE* stderr;  ///< stderr of process
} SP_Process;

/**
 * A struct containing options when spawning a process.
 *
 * @see sp_run
 * @see sp_open
 */
typedef struct sp_opts {
    char* cwd;        ///< change the working directory of the process
    char** env;       ///< environment passed to execve
    bool detach;      ///< detach process from parent
    bool inheritFds;  ///< don't attempt to close other open file descriptors.
    bool nonBlockingPipes;  ///< Make pipes non-blocking.
    SP_RedirOpt stdin;      ///< options for stdin
    SP_RedirOpt stdout;     ///< options for stdout
    SP_RedirOpt stderr;     ///< options for stderr
    /**
    * Specifies the order to attempt redirection.
    * Default order is {0, 1, 2}. stdin then stdout then stderr.
    * If given you must fully specify the unqiue order.
    * e.g. .redirOrder = {2, 1, 0}
    */
    SP_RedirTarget redirOrder[3];
} SP_Opts;

#define SP_ARGV(...) \
    (char*[]) { __VA_ARGS__, NULL }

#define SP_OPTS(...) &((SP_Opts){__VA_ARGS__})

#define SP_SIZE_FIXED_ARR(fixedArray) \
    (sizeof(fixedArray) / sizeof(fixedArray[0]))

/**
 * Run a process with the given options and wait for it to finish.
 * If successful, memory is allocated for the sp_process and must be freed with sp_destroy()
 *
 * @param[in] argv array of arguments to pass to execve. The last element must be NULL.
 * @param[in,out] options options used when spawning the process. See sp_opts
 * @return a pointer to a new sp_process or NULL on error and errno is set accordingly.
 */
SP_Process* sp_run(char** argv, SP_Opts* options);

/**
 * Open a process with the given options and return a pointer to it.
 * If successful, memory is allocated for the sp_process and must be freed with sp_destroy()
 *
 * @param[in] argv array of arguments to pass to execve. The last element must be NULL.
 * @param[in,out] options options used when spawning the process. See sp_opts
 * @return a pointer to a new sp_process or NULL on error and errno is set accordingly.
 */
SP_Process* sp_open(char** argv, SP_Opts* options);

/**
 * Send SIGTERM to a running process.
 *
 * @param[in] process
 * @return 0 on success, -1 on error and errno is set accordingly.
 * @see sp_signal
 */
int sp_terminate(SP_Process* process);

/**
 * Send SIGKILL to a running process.
 *
 * @param[in] process
 * @return 0 on success, -1 on error and errno is set accordingly.
 * @see sp_signal
 */
int sp_kill(SP_Process* process);

/**
 * Send a signal to a running process.
 *
 * @param[in] process
 * @param[in] signal
 * @return 1 on success, -1 on error and errno is set accordingly.
 */
int sp_signal(SP_Process* process, int signal);

// close stdin of process if opened with a pipe.

/**
 * Close stdin of a process if it was opened with a pipe and set it to NULL.
 * Can be called multiple times, but only the first call has affect.
 *
 * @param[in,out] process
 */
void sp_close(SP_Process* process);

/**
 * Wait for a process to exit and set process->exitCode accordingly.
 *
 * @param[in,out] process
 * @return the exit code of the process or -1 on error and errno is set accordingly.
 */
int sp_wait(SP_Process* process);

/**
 * Check if a process has terminated without blocking and set process->exitCode if it has.
 *
 * @param[in,out] process
 * @return the exit code of the process, or -1 if the process is still running or an error occured.
 */
int sp_poll(SP_Process* process);

/**
 * Free all memory allocated to an sp_process.
 * If the process is NULL, this function does nothing.
 * If the process is still running, it is killed and waited on.
 *
 * @param[in,out] process the process being freed
 */
void sp_destroy(SP_Process* process);

#endif  // SP_PROCESS_H
