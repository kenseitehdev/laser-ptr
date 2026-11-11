# laser-ptr

A small menu-bar/tray utility that keeps the computer awake while its laser is lit. The current macOS release is **v0.2**. The Linux, FreeBSD, and Windows backends are being prepared for cross-platform release.

## macOS

Requires macOS 13 or newer and Xcode Command Line Tools. The build supports Apple Silicon (`arm64`) and Intel (`x64`); 32-bit macOS is unsupported.

```sh
make build
make run
```

Build for Intel from Apple Silicon with `ARCH=x64 make build`, or explicitly build Apple Silicon with `ARCH=arm64 make build`.

The menu-bar menu toggles the keep-awake activity and offers indefinite, 30-minute, 1-hour, and 2-hour durations. While active, the app disables idle system and display sleep.

## Linux

Requires GTK 3, GLib/GIO, and a desktop session bus that implements the freedesktop ScreenSaver or GNOME SessionManager inhibit API. Ayatana AppIndicator is optional; when present, the app adds a tray icon. If there is no status-notifier host or AppIndicator support, the app opens a small control window.

```sh
make -C platforms/linux
./platforms/linux/build/$(uname -m)/laser-ptr
sudo make -C platforms/linux install PREFIX=/usr
```

Linux builds can target `aarch64`, `x86_64`, or `i686` by setting `ARCH` and a matching `CC`; cross builds also require target GTK/GLib libraries and matching `PKG_CONFIG_LIBDIR`/`PKG_CONFIG_SYSROOT_DIR`. FreeBSD builds use the same GTK source and accept the target compiler and architecture through `CC` and `ARCH`.

On Fedora, install `gcc make gtk3-devel glib2-devel pkgconf-pkg-config rpm-build`, then build an RPM with:

```sh
VERSION=0.2.0 packaging/fedora/build.sh
```

The Fedora builder accepts `ARCH=aarch64`, `ARCH=x86_64`, or `ARCH=i686`; for cross builds, set `CC` to a matching compiler and provide matching GTK/GLib target libraries through `PKG_CONFIG_LIBDIR` and `PKG_CONFIG_SYSROOT_DIR`.

The Debian builder accepts `ARCH=x64`, `ARCH=x86`, or `ARCH=arm64` and forwards `CC` to the Linux Makefile. Cross builds need matching GTK/GLib libraries and target pkg-config paths.

## FreeBSD

Requires GTK 3, GLib/GIO, and a desktop session bus with the freedesktop ScreenSaver or GNOME SessionManager inhibit API. Ayatana AppIndicator is optional; when present, the app adds a tray icon. If there is no status-notifier host or AppIndicator support, the app opens a small control window.

```sh
make -C platforms/freebsd
./platforms/freebsd/build/$(uname -m)/laser-ptr
sudo make -C platforms/freebsd install
```

## Windows

Build from a Visual Studio Native Tools PowerShell session configured for the target architecture (x64, x86, or ARM64):

```powershell
./platforms/windows/build.ps1 -Arch x64
# Repeat from the matching x86 and ARM64 Native Tools shells.
./platforms/windows/build.ps1 -Arch x86
./platforms/windows/build.ps1 -Arch arm64
```

The Windows version uses the notification area and the native execution-state API. Cross builds need matching target toolchains and GTK libraries. Linux/FreeBSD and Windows ports still need target-system builds and runtime checks before release. See [CHANGELOG.md](CHANGELOG.md) for macOS release notes.
