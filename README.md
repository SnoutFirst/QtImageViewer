# QtImageViewer

[![ci](https://github.com/SnoutFirst/QtImageViewer/actions/workflows/ci.yml/badge.svg)](https://github.com/SnoutFirst/QtImageViewer/actions/workflows/ci.yml)

A small image viewer built with Qt Widgets, in the spirit of `fim` and `fbi` but
without pretending to be nearly as feature complete: give it images and/or
directories, it shows them, you step through them (or let a slideshow do it).

```sh
QtImageViewer ~/Pictures/holiday-2024
QtImageViewer --random --slideshow --timeout 8 ~/Pictures/wallpapers photo.png
```

## Features

- Any number of image files, directories, or a mix of both, on the command line.
- Directories are searched for images recursively, or only on their top level
  with `--no-recurse`. Several arguments simply add up.
- `--random` shuffles: every image is shown once before any image is shown
  again, and a new shuffle is drawn each time the whole list has been shown.
- `--slideshow` advances by itself; `--timeout` says how often.
- A window without a frame — no title bar, no borders — that is moved by
  dragging the image and resized by dragging near an edge or a corner.
- Keyboard and mouse wheel navigation, full screen. The file name and position
  in the list (`photo.png (3/12)`) go in the window title, for task bars and
  window lists; `--verbose` also draws them — with the pixel size and the file
  size — over the bottom left corner of the image.
- Honours the EXIF orientation of camera images.
- Builds against Qt 5.15 and Qt 6 from the same sources and the same
  `CMakeLists.txt`.

Not supported, on purpose: WebAssembly (there are no QWidgets there), video,
thumbnails, image editing, and every other feature you would expect from
`feh`. It is a viewer.

## Requirements

- CMake 3.21 or newer
- A C++20 compiler
- Qt 6 (recommended) or Qt 5.15, with the `Widgets` module: `qt6-base-dev` /
  `qtbase5-dev` on Debian and Ubuntu, `qt` from Homebrew, or the Qt online
  installer on Windows and macOS.

The only dependency besides Qt is [Catch2](https://github.com/catchorg/Catch2),
which CMake fetches by itself when the tests are built.

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

That takes Qt 6 if it can find it and Qt 5 otherwise. To be explicit about
which one, set `QTIMAGEVIEWER_QT_VERSION`:

```sh
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DQTIMAGEVIEWER_QT_VERSION=5
```

`CMakePresets.json` has ready made presets too (`cmake --list-presets`), for
example `unixlike-gcc-debug` and `unixlike-gcc-qt5-debug`.

The build treats warnings as errors; that can be changed with
`-DQtImageViewer_WARNINGS_AS_ERRORS=OFF`.

The address and undefined behaviour sanitizers are **not** part of an ordinary
build: an instrumented binary runs several times slower and keeps every freed
allocation in its quarantine, so a viewer built that way climbs to hundreds of
megabytes of resident memory that only looks like a leak. The `*-debug` presets
and the CI Debug job switch them on, and so does

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DQtImageViewer_ENABLE_SANITIZER_ADDRESS=ON \
  -DQtImageViewer_ENABLE_SANITIZER_UNDEFINED=ON
```

## Usage

```
QtImageViewer [options] [images...]
```

Every `images` argument is either an image file, or a directory to take images
from. With no arguments the program explains itself and exits.

| Option | Meaning |
| --- | --- |
| `-r`, `--random` | Show the images in a shuffled order instead of the order given. |
| `-s`, `--slideshow` | Advance to the next image automatically. |
| `-t`, `--timeout <seconds>` | Seconds each image is shown in a slideshow. Default: 5. Only used together with `--slideshow`; fractions are allowed (`-t 0.5`). |
| `--no-recurse` | Do not look inside the subdirectories of a directory argument. |
| `-v`, `--verbose` | Draw the file name, the position in the list, the pixel size and the file size over the bottom left corner of the image. |
| `-h`, `--help` | Usage summary. |
| `-V`, `--version` | Version of the program. |

### Controls

| Key | Action |
| --- | --- |
| `Right`, `Down`, `Space`, `Page Down`, `n` | Next image |
| `Left`, `Up`, `Backspace`, `Page Up`, `p` | Previous image |
| Mouse wheel up / down | Previous / next image |
| Drag with the left button | Move the window |
| Drag near an edge or a corner | Resize the window |
| Double click | Toggle full screen |
| `r` | Shuffle again: draw a new random order and show its first image |
| `f`, `F11` | Toggle full screen |
| `Esc` | Leave full screen, or quit when not in full screen |
| `q` | Quit |

A manual step during a slideshow restarts the countdown, so the image you just
stepped onto gets the whole interval as well.

### What "random" means here

`--random` is not "pick a random image every time": that shows some images
twice and others never. Instead a random permutation is drawn and walked
through, so every image is shown exactly once; when the list is used up a new
permutation is drawn, and it is arranged so that the last image of the old list
is never the first image of the new one. Stepping backwards behaves the same
way at the other end of a cycle.

### Details worth knowing

- A file you name on the command line is shown even if its name looks like it
  is not an image; if it cannot be decoded, the viewer says so and you can
  move on. The contents of a *directory* are filtered by the image formats Qt
  can actually read.
- Images are scaled down to fit the window, and never scaled up beyond their
  own size.
- Images are decoded at full resolution — there is no downscaled decoding for
  huge files.
- A file reachable through more than one argument is only shown once.
- Files and directories whose name starts with a dot are ignored when scanning a
  directory (as are entries the platform marks as hidden), and symlinks to
  directories are followed once, not in circles.
- The window has no title bar, so `Esc` and `q` are how it is closed (a window
  manager's own shortcut, such as `Alt+F4`, works as well). Where the platform
  can do it the *window system* performs the move and the resize
  (`QWindow::startSystemMove()` / `startSystemResize()`), which is what keeps
  snapping and tiling working; on a platform that cannot — and on an X server
  with no window manager at all — the widget moves and resizes itself.
- The `--verbose` overlay is a label floating over the image rather than part
  of the layout, so it does not take a strip of the window for itself. Without
  `--verbose` nothing is drawn over the image at all.
- The Qt platform plugin needs a display. On a headless machine, use
  `QT_QPA_PLATFORM=offscreen` for `--help`, `--version` or the test suite.

### Memory

The program holds one decoded image and one scaled copy of it; stepping through
images replaces them, so the resident size settles instead of growing. To check
that rather than trust it, build the Debug preset, which has the sanitizers on
(see `CMakePresets.json`), and run the tests or the program:

```sh
cmake --preset unixlike-gcc-debug
cmake --build --preset unixlike-gcc-debug
ctest --preset test-unixlike-gcc-debug        # every test is a leak check too
```

LeakSanitizer prints nothing for this program. It does find two allocations of
Qt's own — the X11 platform integration builds a `QGenericUnixServices` for the
desktop portal, opens a D-Bus connection on the way, and destroys neither — so
the program tells ASan to ignore those four symbols itself
(`__asan_default_suppressions()` in `src/main.cpp`): a report that survives that
is the viewer's. `valgrind --leak-check=full` agrees: "definitely lost: 0 bytes"
in this program's frames, and 1,376 bytes in Qt's X11/D-Bus start-up code.

## Tests

```sh
cmake -S . -B build && cmake --build build -j
cd build && QT_QPA_PLATFORM=offscreen ctest --output-on-failure
```

The tests cover the playlist rules (the "every image once" guarantee, both
directions, across cycle boundaries), the command line expansion (recursion,
duplicate suppression, unreadable paths), the command line interface itself,
and the widgets: that the window really is frameless, that dragging moves it and
that dragging near an edge resizes it, on `QT_QPA_PLATFORM=offscreen` and
without a window manager (the tests then exercise the widget's own move and
resize, which is also what happens on a platform without
`startSystemMove()`). Building and testing against both Qt majors is what the
CI matrix does.

## Static analysis

`clang-tidy` and `cppcheck` are both warnings-as-errors and run in CI, one job
each; the checks they are held to live in `.clang-tidy` and in
`cmake/StaticAnalyzers.cmake`. Locally:

```sh
cmake -S . -B build -DQtImageViewer_ENABLE_CLANG_TIDY=ON -DQtImageViewer_ENABLE_CPPCHECK=ON
cmake --build build       # a finding fails the build
```

Anything switched off in `.clang-tidy` is there with its reason; the clang-tidy
job pins the version the checks were tuned against so a new release cannot turn
the build red on its own.

## License

MIT — see [LICENSE](LICENSE). Copyright (c) 2026 d3fault.
