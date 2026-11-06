#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
APP="$ROOT/build/laser-ptr.app"
CONTENTS="$APP/Contents"

mkdir -p "$CONTENTS/MacOS"
cp "$ROOT/Resources/Info.plist" "$CONTENTS/Info.plist"
swiftc \
    -O \
    -parse-as-library \
    -target arm64-apple-macosx13.0 \
    -framework AppKit \
    -o "$CONTENTS/MacOS/laser-ptr" \
    "$ROOT/Sources/laser-ptr/main.swift"
find "$APP" -name '._*' -type f -delete
codesign --force --deep --sign - "$APP"
printf 'Built %s\n' "$APP"
