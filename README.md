# laser-ptr

A small macOS menu-bar utility that keeps the Mac awake while its laser beam is lit. Current release: **v0.2**.

## Build and run

Requires macOS 13 or newer and Xcode Command Line Tools.

```sh
make build
make run
```

The menu-bar menu toggles the keep-awake activity and offers indefinite, 30-minute,
1-hour, and 2-hour durations. While active, the app disables idle system and display
sleep. The request ends when its timer expires, when you turn it off, or when you quit.

The app is built for Apple silicon and macOS 13 or newer. It runs as a menu-bar accessory without a Dock icon. See [CHANGELOG.md](CHANGELOG.md) for release notes.
