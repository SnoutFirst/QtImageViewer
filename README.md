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
- Keyboard and mouse wheel navigation, full screen, window title showing the
  file name and position in the list (`photo.png (3/12)`).
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

The build treats warnings as errors and turns on the address and undefined
behaviour sanitizers by default; both can be changed with the usual
`-DQtImageViewer_*` options (`-DQtImageViewer_WARNINGS_AS_ERRORS=OFF`,
`-DQtImageViewer_ENABLE_SANITIZER_ADDRESS=OFF`, ...).

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
| `-h`, `--help` | Usage summary. |
| `-v`, `--version` | Version of the program. |

### Controls

| Key | Action |
| --- | --- |
| `Right`, `Down`, `Space`, `Page Down`, `n` | Next image |
| `Left`, `Up`, `Backspace`, `Page Up`, `p` | Previous image |
| Mouse wheel up / down | Previous / next image |
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
- Hidden files and directories are ignored when scanning a directory, and
  symlinks to directories are followed once, not in circles.
- The Qt platform plugin needs a display. On a headless machine, use
  `QT_QPA_PLATFORM=offscreen` for `--help`, `--version` or the test suite.

## Tests

```sh
cmake -S . -B build && cmake --build build -j
cd build && QT_QPA_PLATFORM=offscreen ctest --output-on-failure
```

The tests cover the playlist rules (the "every image once" guarantee, both
directions, across cycle boundaries), the command line expansion (recursion,
duplicate suppression, unreadable paths), and the command line interface
itself. Building and testing against both Qt majors is what the CI matrix
does.

## License

MIT — see [LICENSE](LICENSE). Copyright (c) 2026 d3fault.
