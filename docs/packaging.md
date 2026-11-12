# Packaging

GTK made Qt follows the same installation standard as Motif on Qt, so one
convention covers both (for example in the qtify package collection).

## The library package: `gtk-made-qt`

A build-time package. Installed layout (`libdir` is `/usr/lib` on Arch,
`/usr/lib/<triplet>` on Debian-style multiarch):

```
/usr/bin/gtk-made-qt-env                    sets PKG_CONFIG_PATH, LDFLAGS, PATH
/usr/include/gtk-made-qt/{gtk,gdk}/         the GTK 1.2/2/3/4 headers
<libdir>/gtk-made-qt/libgtk-made-qt.a       the library (static)
<libdir>/gtk-made-qt/libgtk.so, libgdk.so, libgtk-x11-2.0.so, libgdk-x11-2.0.so,
                     libgtk-3.so, libgdk-3.so, libgtk-4.so
                                            GNU ld scripts: the archive + Qt
<libdir>/gtk-made-qt/pkgconfig/             gtk+.pc, gtk+-2.0.pc, gtk+-3.0.pc, gtk4.pc ...
<libdir>/gtk-made-qt/bin/                   gtk-config, glib-config (GTK 1.2 programs)
<libdir>/gtk-made-qt/shared/                the same for the shared library
                                            (gtk-made-qt-env --shared)
<libdir>/libgtk-made-qt.so.0                the shared library (programs with plugins)
<libdir>/pkgconfig/gtk-made-qt.pc           found without extra settings
/usr/share/doc/gtk-made-qt/
```

Everything sits in its own directories, so the package installs next to the
real GTK without conflicts, and a build only sees it when asked to
(`gtk-made-qt-env`).

Rules for the library package:

- Depends on `qt6-base`, `glib2`, `cairo`, `pango` (programs link them).
- Programs linked statically need nothing of the package at run time.
  Programs built with `--shared` need `libgtk-made-qt.so.0`; a distribution
  may split it into a small runtime package (`gtk-made-qt-libs`), or the
  application package depends on `gtk-made-qt`.
- Keep the static library: on Arch `options=(staticlibs !lto)`.
- Do not strip the `.so` link scripts (they are text).

## Ported application packages

- Name: `<app>-qtify` (for example `leafpad-qtify`), with
  `provides=(<app>)` and `conflicts=(<app>)` when it replaces the original.
- `makedepends`: `gtk-made-qt` plus the program's own build tools;
  `depends`: `qt6-base`, `glib2`, `cairo`, `pango` and the program's other
  libraries - never `gtk`/`gtk2`/`gtk3`/`gtk4`. Programs built with
  `gtk-made-qt-env --shared` (plugins calling GTK) also depend on the
  runtime library.
- Build with the program's own build system wrapped in `gtk-made-qt-env`;
  do not patch the sources unless needed for unrelated reasons.
- Install where the original package would.
- Run something headless in `check()` when possible:
  `QT_QPA_PLATFORM=offscreen GTKQT_AUTOQUIT=1000 ./prog`.

## Example: PKGBUILD for the library

```bash
pkgname=gtk-made-qt-git
pkgver=r1
pkgrel=1
pkgdesc="The GTK 2/3/4 C API on Qt 6: build GTK programs that run without GTK"
arch=(x86_64 aarch64)
url="https://github.com/staalmannen/gtk-made-qt"
license=(LGPL-2.1-or-later)
depends=(qt6-base glib2 cairo pango)
makedepends=(cmake git)
provides=(gtk-made-qt)
conflicts=(gtk-made-qt)
options=(staticlibs !lto)
source=("git+https://github.com/staalmannen/gtk-made-qt.git")
sha256sums=(SKIP)

pkgver() {
  cd gtk-made-qt
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cmake -B build -S gtk-made-qt -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release \
    -DGTKQT_BUILD_EXAMPLES=OFF -DGTKQT_BUILD_TESTS=OFF
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
```

## Example: PKGBUILD for Leafpad on Qt

```bash
pkgname=leafpad-qtify
pkgver=0.8.19
pkgrel=1
_commit=3b1a2c2a1fad10f7f03e9b240bb6fa425aed5417
pkgdesc="Leafpad (the GTK 2 text editor) built on Qt with GTK made Qt"
arch=(x86_64 aarch64)
url="https://github.com/tarot231/leafpad"
license=(GPL-2.0-or-later)
depends=(qt6-base glib2 cairo pango)
makedepends=(gtk-made-qt git autoconf automake intltool pkgconf)
provides=(leafpad)
conflicts=(leafpad)
source=("git+https://github.com/tarot231/leafpad.git#commit=${_commit}")
sha256sums=(SKIP)

build() {
  cd leafpad
  intltoolize --force
  autoreconf -fi
  gtk-made-qt-env ./configure --prefix=/usr
  gtk-made-qt-env make
}

check() {
  cd leafpad
  QT_QPA_PLATFORM=offscreen GTKQT_AUTOQUIT=1000 ./src/leafpad
}

package() {
  cd leafpad
  make DESTDIR="$pkgdir" install
}
```

## Example: PKGBUILD for XMMS 1.2 (GTK 1.2) on Qt - draft

XMMS is the GTK 1.2 proof of concept: its plugins (output, input, visual)
and `libxmms` call GTK themselves, so it is built against the shared
library. **Status: not yet built** - the GTK 1.2 API level runs the GTK 1.2
tutorial programs; building XMMS itself is the next step, so expect this
recipe to change. The checksum has to be filled in from the release.

```bash
pkgname=xmms-qtify
pkgver=1.2.11
pkgrel=1
pkgdesc="XMMS, the GTK 1.2 audio player, built on Qt with GTK made Qt"
arch=(x86_64 aarch64)
url="https://www.xmms.org"
license=(GPL-2.0-or-later)
depends=(qt6-base glib2 cairo pango gtk-made-qt alsa-lib libmad)
makedepends=(pkgconf)
provides=(xmms)
conflicts=(xmms)
source=("https://www.xmms.org/files/1.2.x/xmms-${pkgver}.tar.bz2")
sha256sums=('SKIP')   # fill in from the release

build() {
  cd "xmms-${pkgver}"
  gtk-made-qt-env --shared ./configure --prefix=/usr --disable-esd --disable-opengl --disable-mikmod
  gtk-made-qt-env --shared make
}

check() {
  cd "xmms-${pkgver}"
  QT_QPA_PLATFORM=offscreen GTKQT_AUTOQUIT=1500 ./xmms/xmms
}

package() {
  cd "xmms-${pkgver}"
  make DESTDIR="$pkgdir" install
}
```

