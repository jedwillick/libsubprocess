# libsubprocess

<!-- badges: start -->

[![CI](https://github.com/jedwillick/libsubprocess/actions/workflows/ci.yml/badge.svg)](https://github.com/jedwillick/libsubprocess/actions/workflows/ci.yml)
[![Docs](https://github.com/jedwillick/libsubprocess/actions/workflows/docs.yml/badge.svg)](https://github.com/jedwillick/libsubprocess/actions/workflows/docs.yml)

<!-- badges: end -->

A subprocess library for C that provides a higher level abstraction of fork,
exec, pipe and dup2 making it easy to run processes.

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

Then you can build and install both the shared and static library.

```bash
# By default installs to /usr/local so sudo is needed.
sudo make install
sudo ldconfig

# You can also specify the install location:
INSTALL_PREFIX=~/.local make install
```

Alternatively you can compile the library with your source files by copying everything
from `src/` and `include/` to your local source tree.

```bash
curl -Lo- https://github.com/jedwillick/libsubprocess/archive/main.tar.gz \
  | tar -xzvk --strip-components=1 libsubprocess-main/src libsubprocess-main/include
```

## Usage

To use the library you can `#include "subprocess/process.h"` which gives you
access to the public API.
Please refer to the [documentation][docs] for detailed information regarding
the API.

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

Finally we can bring it all together as a complete program `example.c`

```c
#include <subprocess/process.h>

int main(void) {
    SP_Process *proc = sp_run(SP_ARGV("echo", "Hello world!"),
                              SP_OPTS(.stdout = SP_REDIR_PIPE()));
    if (!proc) {
        return 1;
    }

    SP_Opts opts = {
        // For style reasons I decided to not use the inline SP_OPTS macro.
        .stdin = SP_REDIR_FILE(proc->stdout),
        .stdout = SP_REDIR_PATH("sp-hello-world.txt"),
    };
    SP_Process *proc2 = sp_run(SP_ARGV("tr", "[a-z]", "[A-Z]"), &opts);
    sp_destroy(proc);
    sp_destroy(proc2);
}
```

Then we can compile and dynamically link the library:

```bash
gcc example.c -lsubprocess
./a.out
cat sp-hello-world.txt
```

For more options refer to the [documentation][docs].

## Uninstalling

You can uninstall the shared library by running one command

```bash
# If you installed as root in the default location.
sudo make uninstall

# Or if you installed to a different location
INSTALL_PREFIX=~/.local/ make uninstall
```

[docs]: https://jedwillick.github.io/libsubprocess/files
