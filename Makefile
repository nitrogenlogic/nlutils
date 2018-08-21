.PHONY: all build debug clean install init init-debug distclean test test-debug

ncpus := $(shell grep -i 'processor.*:' /proc/cpuinfo | wc -l | sed -e 's/^0/8/')

all: build

build: init
	sh -c "cd build-$$(uname -m) && make -j$(ncpus)"

debug: init-debug
	sh -c "cd build-$$(uname -m) && make -j$(ncpus)"

clean: init
	sh -c "cd build-$$(uname -m) && make -j$(ncpus) clean"

install: init
	sh -c "cd build-$$(uname -m) && make -j$(ncpus) install && ldconfig"

init:
	sh -c "mkdir -p build-$$(uname -m)"
	sh -c "cd build-$$(uname -m) && cmake -DCMAKE_BUILD_TYPE=Release $(CMAKE_DEFS) .."

init-debug:
	sh -c "mkdir -p build-$$(uname -m)"
	sh -c "cd build-$$(uname -m) && cmake -DCMAKE_BUILD_TYPE=Debug $(CMAKE_DEFS) .."

distclean:
	sh -c "rm -rf build-$$(uname -m) build-pkg-*"

test: build
	printf "\n\033[36mRun with \e[1mVALGRIND=0 make test\e[0;36m to disable Valgrind for faster testing\e[0m\n"
	sh -c "cd build-$$(uname -m)/tests && ./tests.sh"

test-debug: debug
	printf "\n\033[35mRun with \e[1mVALGRIND=0 make test-debug\e[0;35m to disable Valgrind for faster testing\e[0m\n"
	sh -c "cd build-$$(uname -m)/tests && ./tests.sh"
