# laser-ptr — Linux C port

Tray keep-awake utility ported from the supplied Swift/AppKit source. Keeps the
left-facing laser pointer, red beam when active, on/off toggle, indefinite / 30
minute / one hour / two hour choices, status countdown and Quit. The icon changes
between on/off states, matching the original; it is not a continuously animated
icon. The original macOS source is preserved in `macos/main.swift`.

## Build on the Pi's 64-bit Linux desktop

```sh
sudo apt install build-essential pkg-config libgtk-3-dev libayatana-appindicator3-dev
make
./build/native/laser-ptr
```

Run in a graphical login with a panel that supports AppIndicator / StatusNotifier
icons. A headless session cannot show the tray icon. Some desktops require an
indicator panel plugin; Wayland alone does not provide one. The default pointer
is white for dark panels; choose `--dark-outline` for a black pointer.

```sh
make install PREFIX="$HOME/.local"
./build/native/laser-ptr --dark-outline
```

Installation adds an application launcher, without enabling autostart.
The original Swift version builds on macOS with `make macos`.

## ARM64 ELF64

```sh
make elf64
```

On AArch64 Linux this uses the native tools, producing `build/arm64/laser-ptr`.
Elsewhere install an AArch64 Linux cross compiler, Linux sysroot, target GTK3 /
Ayatana / GLib libraries, and a pkg-config wrapper pointing at those libraries:

```sh
make elf64 ELF64_CC=/path/to/aarch64-linux-gnu-gcc ELF64_PKG_CONFIG=/path/to/target-pkg-config
```

A macOS ARM64 compiler produces Mach-O, not Linux ELF. Native compilation on the
Pi is simpler. This is a dynamically linked executable, not a fully static one.

## Keep-awake implementation

Activation runs on a worker thread and requires **both**:

- A desktop screen-idle inhibitor: freedesktop ScreenSaver, with GNOME and MATE
  SessionManager fallbacks.
- A systemd-logind `idle:sleep` block inhibitor, held by an open file descriptor.

The app stays Off (or retains the previous active duration) if acquisition fails.
An unavailable service, unsupported desktop or denied authorization produces an
error. No fake keyboard/mouse events, sudo invocation, or persistent power-setting
changes are used. PolicyKit handles any required authorization through the system.

The logind inhibitor may also block ordinary explicit suspend requests, not just
idle suspend. Privileged/forced suspend can override inhibitors. Actual screen
blanking behavior depends on the desktop honoring its inhibition API. Systems
without logind or any of the supported screen-inhibition services are unsupported;
the program reports failure rather than claiming it is active. In particular,
check the services available on the selected Raspberry Pi desktop/compositor.

Expiry, Turn Off, Quit and handled SIGINT/SIGTERM close the system descriptor and
request release of the screen inhibitor. Session inhibitors are also scoped to
the D-Bus client, so they are released on process disconnect. Service owner changes
turn the mode off and prompt reacquisition. Switching duration acquires replacement
locks before releasing the existing ones; stale asynchronous results are discarded.
The countdown uses boot time on Linux so suspend time counts toward the duration.

## Verification

```sh
make test
ASAN_OPTIONS=detect_leaks=0 make test CFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'
```

Passed in the delivery environment:

- Timer tests: all durations, indefinite operation, exact expiry, replacement,
  stop, invalid inputs, and integer-overflow rejection.
- Four valid SVG variants: on/off, light/dark pointer, and left-facing transform.
- The same tests under AddressSanitizer and UBSan. LeakSanitizer was disabled
  because this environment's process tracing prevents it from running.

The GTK/Ayatana/GIO development headers and ARM64 compiler were unavailable here.
The complete GUI/inhibitor code has **not been compiled or run**. ARM64 execution,
visual appearance, service restart handling, authorization and real sleep/display
inhibition remain to be checked on the target desktop.

Target checklist: build; verify the tray icon and menu; activate a timed option;
inspect `systemd-inhibit --list`; verify the screen stays on past its idle timeout;
turn Off and confirm release; verify duration expiry; test denied/unavailable
services; quit while active and confirm the inhibitor disappears.

API references:
- https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html
- https://ebassi.pages.freedesktop.org/xdg-specs/idle-inhibit-spec/0.1/
