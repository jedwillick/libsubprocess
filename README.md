# libsubprocess

[![CI](https://github.com/jedwillick/subprocess/actions/workflows/ci.yml/badge.svg)](https://github.com/jedwillick/subprocess/actions/workflows/ci.yml)

A subprocess library for C that provides a higher level abstraction of fork,
exec, pipe and dup making it easy to run processes.

## Requirements

- Linux
- glibc
- gcc
- make

## Installation

First clone the repository

```bash
git clone git@github.com:jedwillick/libsubprocess.git
cd libsubprocess
```

Then you can build and install it as a shared library that will be dynamically linked

```bash
# By default installs to /usr/local so sudo is needed.
sudo make install

# You can also specify the install location:
INSTALL_PREFIX=~/.local/ make install
```

Then use the `-lsubprocess` flag when linking.

Alternatively you can statically link the library by copying all
C files to your src tree and having `include/subprocess` on your include path.
Then when compiling you must include `-D_GNU_SOURCE`

## Usage

To use the library you can `#include "subprocess/process.h"` which gives you
access to the public API.
Pleae refer to the documentation for detailed information regarding the API.

## Examples

A simple example that runs `echo Hello world!` with default options

```c
SP_Process* proc = sp_run(SP_ARGV("echo", "Hello world!"), 0);
sp_destroy(proc);
```

We can also capture that output by redirecting stdout to a pipe.

```c
SP_Process* proc = sp_run(SP_ARGV("echo", "Hello world!"), SP_OPTS(.stdout = SP_REDIR_PIPE()));
if (!proc) {
    return 1;
}
char buf[124];
while ((fgets(buf, 124, proc->stdout))) {
    printf("%s", buf);
}
sp_destroy(proc);
```

Instead of printing the output we can pass it along to another process just like
doing `echo "Hello world!" | tr [a-z] [A-Z]`

```c
SP_Process* proc = sp_run(SP_ARGV("echo", "Hello world!"), SP_OPTS(.stdout = SP_REDIR_PIPE()));
if (!proc) {
    return 1;
}
SP_Process* proc2 = sp_run(SP_ARGV("tr", "[a-z]", "[A-Z]"), SP_OPTS(.stdin = SP_REDIR_FILE(proc->stdout)));
sp_destroy(proc);
sp_destroy(proc2);
```

We could than redirect that output to a file mimicking
`echo "Hello world!" | tr [a-z] [A-Z] > sp-hello-world.txt`

```c
SP_Process* proc = sp_run(SP_ARGV("echo", "Hello world!"), SP_OPTS(.stdout = SP_REDIR_PIPE()));
if (!proc) {
    return 1;
}

SP_Opts opts = { // For style reasons I decided to not use the inline SP_OPTS macro.
    .stdin = SP_REDIR_FILE(proc->stdout),
    .stdout = SP_REDIR_PATH("sp-hello-world.txt"),
};
SP_Process* proc2 = sp_run(SP_ARGV("tr", "[a-z]", "[A-Z]"), &opts);
sp_destroy(proc);
sp_destroy(proc2);
```

For more options refer to the documentation.

## Uninstalling

You can uninstall the shared library by running one command

```bash
# If you installed as root in the default location.
sudo make uninstall

# Or if you installed to a different location
INSTALL_PREFIX=~/.local/ make uninstall
```
