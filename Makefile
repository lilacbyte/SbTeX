BUILD_DIR ?= build
PREFIX ?= /usr/local
BUILD_TYPE ?= Release
JOBS ?= 2
.PHONY: all build configure sbtex dev test install clean
all: sbtex
configure:
	cmake -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" -DCMAKE_INSTALL_PREFIX="$(PREFIX)" -DBUILD_TESTING=ON
build: configure
	cmake --build "$(BUILD_DIR)" --parallel "$(JOBS)"
sbtex: build
	cp "$(BUILD_DIR)/sbtex" sbtex
dev:
	$(MAKE) sbtex BUILD_TYPE=Debug
test: sbtex
	ctest --test-dir "$(BUILD_DIR)" --output-on-failure
install: sbtex
	cmake --install "$(BUILD_DIR)"
clean:
	$(RM) -r "$(BUILD_DIR)"
	$(RM) *.o sbtex
