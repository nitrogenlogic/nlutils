libnlutils
==========

This project contains [well-tested ![Build Status](https://travis-ci.org/nitrogenlogic/nlutils.svg?branch=master)](https://travis-ci.org/nitrogenlogic/nlutils) basic utility functions and data structures
for Nitrogen Logic C programs.  It also contains tools for cross-compiling code
and building firmware images for Nitrogen Logic controllers.

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


# Subdirectories

- `include/` -- Header files.  Read these to see what functions are available.
  - `nl_time.h` -- Time-related functions, such as arithmetic on `timespec`.
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
chroot (e.g. `~/devel/crosscompile/debian-squeeze-root-i386-build/tmp/`).


[0]: https://www.gnu.org/licenses/agpl-3.0.en.html
