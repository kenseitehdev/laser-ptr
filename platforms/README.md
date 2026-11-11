# Platform backends

- `gtk/` contains the GTK 3 control app shared by Linux and FreeBSD, with an optional Ayatana tray indicator and a visible-window fallback.
- `linux/` and `freebsd/` hold target-specific build rules.
- `windows/` contains the Win32 notification-area implementation and MSVC build script.
- The shipping macOS implementation remains in `Sources/laser-ptr/` and is packaged by `scripts/build-app.sh`.

The Linux/FreeBSD and Windows builds are development ports. Windows x86 and x64 cross-compiles have been checked with MinGW; each target still needs native runtime validation before release.
