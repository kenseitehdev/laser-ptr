CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
PKG_CONFIG ?= pkg-config
PKGS := gtk+-3.0 gio-2.0
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKGS))
GTK_LIBS := $(shell $(PKG_CONFIG) --libs $(PKGS))
ifeq ($(WITH_APPINDICATOR),0)
APPINDICATOR_CFLAGS :=
APPINDICATOR_LIBS :=
else
ifeq ($(WITH_APPINDICATOR),0)
APPINDICATOR_CFLAGS :=
APPINDICATOR_LIBS :=
else
APPINDICATOR_CFLAGS := $(shell $(PKG_CONFIG) --exists ayatana-appindicator3-0.1 && echo -DHAVE_AYATANA_APPINDICATOR)
APPINDICATOR_LIBS := $(shell $(PKG_CONFIG) --exists ayatana-appindicator3-0.1 && $(PKG_CONFIG) --libs ayatana-appindicator3-0.1)
endif
endif
ROOT := ../..
PREFIX ?= /usr/local
DESTDIR ?=
ARCH ?= $(shell uname -m)
OUT := build/$(ARCH)/laser-ptr
.PHONY: all install uninstall clean
all: $(OUT)
$(OUT): $(ROOT)/platforms/gtk/main.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(APPINDICATOR_CFLAGS) $(GTK_CFLAGS) $< $(GTK_LIBS) $(APPINDICATOR_LIBS) -o $@
install: $(OUT)
	mkdir -p $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/share/applications $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps
	install -m 0755 $(OUT) $(DESTDIR)$(PREFIX)/bin/laser-ptr
	install -m 0644 laser-ptr.desktop $(DESTDIR)$(PREFIX)/share/applications/laser-ptr.desktop
	install -m 0644 laser-ptr.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/laser-ptr.png
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/laser-ptr $(DESTDIR)$(PREFIX)/share/applications/laser-ptr.desktop $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/laser-ptr.png
clean:
	rm -rf build
