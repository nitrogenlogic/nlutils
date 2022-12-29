libnlutils
==========

This project contains [well-tested ![Tests](https://github.com/nitrogenlogic/nlutils/actions/workflows/test.yml/badge.svg)](https://github.com/nitrogenlogic/nlutils/actions/workflows/test.yml)
basic utility functions and data structures for Nitrogen Logic C programs.  It
also contains tools for cross-compiling code and building firmware images for
Nitrogen Logic controllers.

Functions include:

- Arithmetic helpers for `struct timespec` (see `include/time.h`).
- String splitting, comparison, conversion, and escaping functions (see
  `include/str.h`, `include/url.h`, and `include/escape.h`).
- A variant datatype (see `include/variant.h`).
- A key-value pair serialization format (see `include/kvp.h`).
- Variations on a `popen3()` function for working with process I/O (see
  `include/exec.h`).
- Timestamp and threadname logging functions (see `include/log.h`).
- Stream copying and similar functions (see `include/stream.h`).
- A wrapper for the `curl` command for making process-isolated network requests
  in the background (see `include/url_req.h`).
- Threading wrappers to simplify thread creation and management, especially at
  application shutdown (see `include/thread.h`).
- Functions to print a stack trace e.g. when a SIGSEGV is received (see
  `include/debug.h`).
- And others

C's standard library doesn't give you much, so for common functions and data
structures every C team either chooses an add-on library or writes their own.
One original goal of Nitrogen Logic was to license code to OEMs to allow them to
add easy support for automation and IoT, so I decided to write my own functions
instead of using something open source like glib.  That way there'd be fewer
dependencies, fewer licenses for OEMs to wrangle, and greater possibility for
customization.  The nlutils code also strives to comply to C99 and POSIX.1-2008.

Some of this code was extracted from other projects (such as the depth camera
server and the logic system for Nitrogen Logic's controllers) when the code was
found to be useful to more than just one project.

Fast forward a few years, and now we have languages like Rust that are probably
a better option for new embedded work.  This older C code is being released to
serve as a reference to anyone who might find it useful.


# Copying

Copyright 2011-2018 Mike Bourgeous, licensed under [GNU Affero GPLv3][0].  See
LICENSE for the complete text of the license.

Why AGPLv3?  Because this code probably shouldn't be used for new development.
If you want more permissive terms, just get in touch.

Includes a public domain implementation of SHA-1 by Steve Reid, et. al.  That
implementation remains public domain (see file headers for which files are
public domain).

URL escaping and unescaping code was modified from public domain code by Fred
Bulback.  Modified code is licensed under AGPLv3.  See `src/url.c`.


# Common uses

## Debugging

For applications that run on a remote server or embedded hardware where it's not
possible to attach a debugger to a running process, it's nice to get detailed
information about crashes in the application logs.  The functions in `debug.h`
make it easier to print a stack trace when an application receives an unexpected
signal (e.g. `SIGSEGV`).  These originated in the knd app and were later moved
to nlutils.

An example is the quickest way to illustrate:

```c
#include <stdio.h>
#include <stdlib.h>
#include <nlutils/nlutils.h>

// Unfortunately global state is the only option to pass data into a signal
// handler, and signal handlers apply to the entire process
static struct nl_thread_ctx *ctx;
static volatile int crashing = 0;

void handle_crash(int signum, siginfo_t *info, void *uctx)
{
  nl_print_signal(stderr, "Crashing due to", info);
  nl_print_context(stderr, (ucontext_t *)uctx);
  NL_PRINT_TRACE(stderr);

  // Pass the signal to other threads so they can also print stack traces
  // Note that this needs to check the main thread as well in practice
  if(!crashing) {
    // First thread to crash
    crashing = 1;
    nl_signal_threads(ctx, signum);
    nl_usleep(250000);
    exit(-1);
  } else {
    // Thread signalled by crashing thread
    pthread_exit(NULL);
  }
}

void *some_thread(void *data)
{
  (void)data; // unused parameter
  nl_usleep(60000000);
  return NULL;
}

int main(void)
{
  nl_set_threadname("Main thread");
  // Error handling omitted for the sake of brevity
  ctx = nl_create_thread_context();
  sigaction(SIGSEGV, &(struct sigaction){.sa_sigaction = handle_crash, .sa_flags = SA_SIGINFO}, NULL);
  nl_create_thread(ctx, NULL, some_thread, NULL, "Other thread", NULL);
  pthread_kill(pthread_self(), SIGSEGV);
  return -1; // Shouldn't be reached
}
```

Compiling and running gives this output (note the use of `-rdynamic` to include
symbol names for the backtrace):

```bash
gcc debug_example.c -o debug_example -O2 -Wall -Wextra -Werror -std=gnu99 -rdynamic -lnlutils -pthread
./debug_example
```

```
2018-08-31 20:21:43.398112 -0700 - Main thread - Crashing due to Segmentation fault (11), code -6 (tgkill-generated signal to thread), at address 0x3e800002192.
2018-08-31 20:21:43.398259 -0700 - Main thread - Originating address: 0x3e800002192 (no matching symbol found).
2018-08-31 20:21:43.398315 -0700 - Main thread - Instruction pointer: 0x7f2781b362d1 (pthread_kill@0x7f2781b362a0 from /lib/x86_64-linux-gnu/libpthread.so.0@0x7f2781b27000).
2018-08-31 20:21:43.398350 -0700 - Main thread - Stack pointer: 0x7ffddb779ca0 (no matching symbol found).
2018-08-31 20:21:43.398625 -0700 - Main thread - 6 backtrace elements:
2018-08-31 20:21:43.398659 -0700 - Main thread - 0: 0x00400c17 - ./debug_example@0x00400000 - handle_crash[0x400bd0]+0x47
2018-08-31 20:21:43.398694 -0700 - Main thread - 1: 0x7f2781b39890 - /lib/x86_64-linux-gnu/libpthread.so.0@0x7f2781b27000 - [null][0x0]+0x7f2781b39890
2018-08-31 20:21:43.398749 -0700 - Main thread - 2: 0x7f2781b362d1 - /lib/x86_64-linux-gnu/libpthread.so.0@0x7f2781b27000 - pthread_kill[0x7f2781b362a0]+0x31
2018-08-31 20:21:43.398771 -0700 - Main thread - 3: 0x00400d37 - ./debug_example@0x00400000 - main[0x400c90]+0xa7
2018-08-31 20:21:43.398843 -0700 - Main thread - 4: 0x7f2781757b97 - /lib/x86_64-linux-gnu/libc.so.6@0x7f2781736000 - __libc_start_main[0x7f2781757ab0]+0xe7
2018-08-31 20:21:43.398866 -0700 - Main thread - 5: 0x00400b0a - ./debug_example@0x00400000 - _start[0x400ae0]+0x2a
2018-08-31 20:21:43.398926 -0700 - Other thread - Crashing due to Segmentation fault (11), code -6 (tgkill-generated signal to thread), at address 0x3e800002192.
2018-08-31 20:21:43.399009 -0700 - Other thread - Originating address: 0x3e800002192 (no matching symbol found).
2018-08-31 20:21:43.399055 -0700 - Other thread - Instruction pointer: 0x7f2781b38c60 (__nanosleep@0x7f2781b38c20 from /lib/x86_64-linux-gnu/libpthread.so.0@0x7f2781b27000).
2018-08-31 20:21:43.399089 -0700 - Other thread - Stack pointer: 0x7f27810f4e80 (no matching symbol found).
2018-08-31 20:21:43.399158 -0700 - Other thread - 7 backtrace elements:
2018-08-31 20:21:43.399186 -0700 - Other thread - 0: 0x00400c17 - ./debug_example@0x00400000 - handle_crash[0x400bd0]+0x47
2018-08-31 20:21:43.399228 -0700 - Other thread - 1: 0x7f2781b39890 - /lib/x86_64-linux-gnu/libpthread.so.0@0x7f2781b27000 - [null][0x0]+0x7f2781b39890
2018-08-31 20:21:43.399274 -0700 - Other thread - 2: 0x7f2781b38c60 - /lib/x86_64-linux-gnu/libpthread.so.0@0x7f2781b27000 - __nanosleep[0x7f2781b38c20]+0x40
2018-08-31 20:21:43.399310 -0700 - Other thread - 3: 0x7f2781d580d5 - /usr/local/lib/libnlutils.so.12@0x7f2781d46000 - nl_usleep[0x7f2781d58050]+0x85
2018-08-31 20:21:43.399336 -0700 - Other thread - 4: 0x00400c8b - ./debug_example@0x00400000 - some_thread[0x400c80]+0xb
2018-08-31 20:21:43.399386 -0700 - Other thread - 5: 0x7f2781b2e6db - /lib/x86_64-linux-gnu/libpthread.so.0@0x7f2781b27000 - [null][0x0]+0x7f2781b2e6db
2018-08-31 20:21:43.399534 -0700 - Other thread - 6: 0x7f278185788f - /lib/x86_64-linux-gnu/libc.so.6@0x7f2781736000 - clone[0x7f2781857850]+0x3f
```


# Subdirectories

- `include/` -- Header files.  Read these to see what functions are available.
  - `nl_time.h` -- Time-related functions, such as arithmetic on `timespec`.
  - `debug.h` -- Debugging-related functions, such as for printing stack traces.
- `src/` - Library source code.
- `programs/` -- Programs installed on embedded and dev systems, but not included
  in Debian packages (see CMakeLists.txt files for details).
- `tools/` -- Programs installed on dev systems, but not on embedded controllers
  or in Debian packages.
- `meta/` -- Version tracking files, package build scripts.
- `meta/toolchain/` -- CMake cross-compilation toolchain definitions.
- `tests/` -- Tests for the library.  See the Testing section below.
- `embedded/` -- Files that are copied to embedded controllers (TODO: Move to
  new project).  This includes a core firmware update interface and http server.


# Dependencies

You'll need the following packages installed on your Debian/Ubuntu system to
compile and use nlutils:

```bash
sudo apt install build-essential checkinstall cmake cmake-curses-gui \
    libevent-dev curl ruby
```

You'll also need these packages for cross-compilation and making root images:

```bash
sudo apt install qemu-system-arm qemu-user-static mtd-utils \
    crossbuild-essential-armel crossbuild-essential-armhf
```


# Building

Nlutils uses CMake for its build system, with a Makefile wrapper for
convenience.  With the above dependencies installed, compilation should be as
simple as running Make.  The Makefile will create a new working directory for
CMake named for the system architecture (e.g. `./build-x86_64`), then compile
the project.  Read the Makefile itself and CMakeLists.txt for more information.

```bash
make
```

To install:

```bash
sudo make install
```

To compile and install a debug build:

```bash
make debug && sudo make -C ./build-`uname -m` install
```

## Debian/Ubuntu packages

Nlutils uses checkinstall to build Debian packages, which is not the official
Debian way but it works.  To build .deb packages for the current system (e.g.
to accompany Palace Designer):

```bash
[PKGDIR=/var/tmp] meta/make_pkg.sh
```

Package files are saved in /tmp.

*Note:* See the Multi-platform Debian Packages section below.


# Running tests

There is a fairly extensive test suite in `tests/`.  To run the tests, use the
Makefile, which will compile the library, build tests, and run the tests using
Valgrind:


```bash
make test
```

To run tests without Valgrind (for faster testing):

```bash
VALGRIND=0 make test
```

Some tests generate temporary files in /tmp.  To leave these behind after tests
finish, use the `DEBUG` environment variable:

```bash
DEBUG=1 make test
```


# Cross-compilation

Unfortunately cross-compilation is based on custom scripts and hard-coded
filesystem locations, layered on top of the Debian cross-build system.  The
long-term goal was to migrate entirely to Debian packages for updates.

## Building the base root image

Start by building a base image for the Debian distribution and architecture
being targeted (defaults to `nofp` and Debian Squeeze).  This sets up a
cross-`chroot` environment that uses QEMU to emulate an ARM CPU, compiles
nlutils into the new environment, then builds an UBIFS image for an embedded
controller.

```bash
./meta/make_root.sh # add --rebuild to start over from scratch
```

Read the comments in `meta/make_root.sh` and `tools/root_helper.sh` for more info.

## Cross-compiling for controller firmware images

The cross-compilation script, `./crosscompile.sh`, supports three architecture
targets, `neon` (OMAP3 from older Beagle Board processors), `nofp` (Marvell
Sheeva), and `armhf` (newer ARM CPUs).  The `armhf` target isn't fully
implemented.  By default, the script will compile `neon` and `nofp`.

There are four output directories for each architecture.  These will normally
be placed in `~/devel/crosscompile/`, but this can be customized by modifying
variables at the top of the script:

- For Automation Controller firmware images:
  - `cross-libs-arm-[target]`
  - `cross-root-arm-[target]`
- For Depth Camera Controller firmware images:
  - `cross-libs-arm-[target]-depth`
  - `cross-root-arm-[target]-depth`

To cross-compile nlutils for inclusion in Depth Camera Controller or Automation
Controller firmware images (note that these firmware images are defined and
documented elsewhere and may not be publicly available):

```bash
./crosscompile.sh
```

To cross-compile with debug symbols:

```bash
BUILD=RelWithDebInfo ./crosscompile.sh # With optimizations
BUILD=Debug ./crosscompile.sh # Without optimizations
```

## Multi-platform Debian Packages

To build Debian packages for all cross-built architectures, separated into /tmp
by architecture:

```bash
TESTS=0 meta/all_arch_pkgs.sh
```

To build Debian packages for a specific architecture, using a debootstrap
Debian chroot:

```bash
# Use TESTS=0 because tests do not work in the cross-build environment
TESTS=0 ARCH=i386 meta/cross_pkg.sh
TESTS=0 ARCH=amd64 meta/cross_pkg.sh
TESTS=0 ARCH=armel meta/cross_pkg.sh
```

This should make packages like these in /tmp:

```
libnlutils_0.12.0-2_amd64.deb
libnlutils_0.12.0-3_i386.deb
libnlutils_0.12.0-4_armel.deb
```

If something goes wrong, you can find the source code in /tmp in the specific
chroot (e.g. `~/devel/crosscompile/debian-buster-root-armel-build/tmp/`).


[0]: https://www.gnu.org/licenses/agpl-3.0.en.html
