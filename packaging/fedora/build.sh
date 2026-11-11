#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
: "${VERSION:?Set VERSION to the release version, for example 0.2.0}"
command -v rpmbuild >/dev/null 2>&1 || { echo 'rpmbuild is required' >&2; exit 1; }

ARCH=${ARCH:-$(uname -m)}
case "$ARCH" in
  arm64) RPM_ARCH=aarch64 ;;
  x64) RPM_ARCH=x86_64 ;;
  x86) RPM_ARCH=i686 ;;
  *) RPM_ARCH=$ARCH ;;
esac
TMP=$(mktemp -d "${TMPDIR:-/tmp}/laser-ptr-rpm.XXXXXX")
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
STAGE="$TMP/stage"
TOP="$TMP/rpmbuild"
mkdir -p "$STAGE" "$TOP/BUILD" "$TOP/BUILDROOT" "$TOP/RPMS" "$TOP/SOURCES" "$TOP/SPECS" "$TOP/SRPMS"
make -C "$ROOT/platforms/linux" install PREFIX=/usr DESTDIR="$STAGE" ARCH="$ARCH" WITH_APPINDICATOR=0 CC="${CC:-cc}"
install -D -m 0644 "$ROOT/LICENSE" "$STAGE/usr/share/licenses/laser-ptr/LICENSE"
tar -C "$STAGE" -czf "$TOP/SOURCES/laser-ptr-files.tar.gz" .
cat > "$TOP/SPECS/laser-ptr.spec" <<EOF
Name: laser-ptr
Version: $VERSION
Release: 1%{?dist}
Summary: Keep the computer awake while the laser is active
License: MIT
Source0: laser-ptr-files.tar.gz
BuildArch: $RPM_ARCH

%description
A small desktop utility with timed keep-awake controls.

%prep

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
tar -xzf %{SOURCE0} -C %{buildroot}

%files
%license /usr/share/licenses/laser-ptr/LICENSE
%attr(0755,root,root) /usr/bin/laser-ptr
%attr(0644,root,root) /usr/share/applications/laser-ptr.desktop
%attr(0644,root,root) /usr/share/icons/hicolor/256x256/apps/laser-ptr.png
EOF
rpmbuild --target "$RPM_ARCH" --define "_topdir $TOP" -bb "$TOP/SPECS/laser-ptr.spec"
OUT="$ROOT/build/packages/fedora"
mkdir -p "$OUT"
cp "$TOP/RPMS/$RPM_ARCH/laser-ptr-$VERSION-1"*.rpm "$OUT/"
printf 'Built Fedora RPM for %s in %s\n' "$RPM_ARCH" "$OUT"
