.DEFAULT_GOAL := all
CC ?= cc
PKG_CONFIG ?= pkg-config
PREFIX ?= /usr/local
DESTDIR ?=
OUT ?= build/native
PACKAGES = gtk+-3.0 ayatana-appindicator3-0.1 gio-2.0 gio-unix-2.0
CFLAGS ?= -O2 -g
WARN = -std=c11 -Wall -Wextra -Wpedantic

.PHONY: all check-deps elf64 test install clean
all: $(OUT)/laser-ptr

check-deps:
	@$(PKG_CONFIG) --exists $(PACKAGES) || { echo 'Install GTK3, Ayatana AppIndicator3 and GLib development packages; see README.md.' >&2; exit 1; }

$(OUT)/laser-ptr: src/main.c src/core.c src/core.h src/inhibit.c src/inhibit.h Makefile | check-deps
	@mkdir -p "$(@D)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $$($(PKG_CONFIG) --cflags $(PACKAGES)) src/main.c src/core.c src/inhibit.c $(LDFLAGS) $$($(PKG_CONFIG) --libs $(PACKAGES)) -lm -o "$@"

# Native build on 64-bit Pi OS uses the system compiler and pkg-config.
# Cross compilation requires matching ARM64 development libraries/sysroot.
ifeq ($(shell uname -m),aarch64)
ELF64_CC ?= cc
ELF64_PKG_CONFIG ?= pkg-config
else
ELF64_CC ?= aarch64-linux-gnu-gcc
ELF64_PKG_CONFIG ?= aarch64-linux-gnu-pkg-config
endif
elf64:
	@$(ELF64_CC) -dumpmachine | grep -Eq '(aarch64|arm64).*linux' || { echo 'ELF64 target requires an AArch64 Linux compiler.' >&2; exit 1; }
	$(MAKE) CC="$(ELF64_CC)" PKG_CONFIG="$(ELF64_PKG_CONFIG)" OUT=build/arm64 all

test:
	@mkdir -p build/tests
	$(CC) $(CFLAGS) $(WARN) -Werror -Isrc tests/core_test.c src/core.c -o build/tests/core_test
	build/tests/core_test
	python3 tests/test_icon.py build/tests/core_test

install: all
	install -d "$(DESTDIR)$(PREFIX)/bin" "$(DESTDIR)$(PREFIX)/share/applications"
	install -m 755 "$(OUT)/laser-ptr" "$(DESTDIR)$(PREFIX)/bin/laser-ptr"
	install -m 644 laser-ptr.desktop "$(DESTDIR)$(PREFIX)/share/applications/laser-ptr.desktop"

clean:
	rm -rf build


.PHONY: macos
macos:
	@test "$$(uname -s)" = Darwin || { echo 'The original macOS app requires macOS.' >&2; exit 1; }
	@mkdir -p build/macos
	swiftc -parse-as-library -O -framework AppKit macos/main.swift -o build/macos/laser-ptr
