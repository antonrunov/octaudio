Octaudio is a multitrack audio editor with embedded [GNU Octave](http://www.gnu.org/software/octave)
interpreter. It is not intended to be a general purpose audio
editor, but a tool mostly designed for audio processing development and testing. If a
simultaneous use of Octave and Audacity (or a similar audio editor) is your common
layout, Octaudio might be a tool for you. It can also be useful for the studying of the
DSP and audio processing.

##### Features

- Runs on GNU/Linux, Mac OS X, and MS Windows, and can be compiled for the other
  platforms.
- Multitrack data viewer provides fast and reliable data displaying on almost arbitrary
  scale in both directions.
- Track groups that are switched with tabs.
- Double precision arithmetics for all data including time and sample rate values.
- Two displaying modes (Normal and Abs Value) for both zero centered (like audio data)
  and positive (amplitude, power, etc.) signals.
- Hide or Show track option.
- Full-function octave console with multiline command editor, parenthesis matching,
  command history, and autocompletion.
- Async execution of octave commands.
- Special built-in octave commands to access all data and object properties.
- Smart Tracks to display data from multiple tracks simultaneously.
- Monitors to display multiple tracks in separate windows with independent scales.
- Audio playback and recording. Full duplex recording is supported as well.
- Standard audio mixing options: mute, solo, gain, stereo pan.
- Tracks can be marked as non-audible to be completely ignored during playback (useful
  for average values, control signals, etc.).

For more information see [doc/octaudio_readme.txt](doc/octaudio_readme.txt).

##### Installing

First of all, Octaudio requires Octave to be installed. Pre-built binaries for Debian
GNU/Linux, Windows (official MinGW Octave 3.6.4 build), and Mac OS X (macports Octave
installation) are available at the [releases](https://github.com/antonrunov/octaudio/releases) page.

Installation instructions are in [doc/install.txt](doc/install.txt).

##### Building

Build dependencies are [Octave](http://www.gnu.org/software/octave), [Qt4](http://download.qt.io/archive/qt),
[Portaudio V19](http://www.portaudio.com), [libsamplerate](http://www.mega-nerd.com/SRC),
and [CMake](http://www.cmake.org).
See more detailed instructions in [doc/build.txt](doc/build.txt).

##### Known issues and limitations

Octaudio is in beta state now. While it is quite functional and relatively stable,
some functionality is not implemented yet, or implemented partially. Below is incomplete
list of the main issues.

- Track data length is limited by memory. Octaudio is designed to work with arbitrary
  length data, but current implementation of data blocks uses memory storage. This
  limitation will be removed in next releases.
- Missing undo/redo functions.
- Missing data block operations. Split, join, move, cut functions are not implemented yet.
- UI functionality is insufficient sometimes, especially for monitors and multiple track
  selection.
