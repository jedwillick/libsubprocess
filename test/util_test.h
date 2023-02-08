#ifndef UTIL_TEST_H
#define UTIL_TEST_H

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <stdlib.h>

#include "subprocess/process.h"

#define BUF_SIZE 1024

SP_Process* assert_process_started(char* file, int line,
                                   SP_Process* (*start)(char**, SP_Opts*),
                                   char** argv, SP_Opts* opts);

#define sp_run(argv, opts) \
    assert_process_started(__FILE__, __LINE__, sp_run, argv, opts)
#define sp_open(argv, opts) \
    assert_process_started(__FILE__, __LINE__, sp_open, argv, opts)

void assert_file_contents(FILE* file, char* expected);
void diff_files(char* f1, char* f2);

#endif  // UTIL_TEST_H
