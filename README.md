Octaudio is a multitrack audio editor with embedded [GNU Octave](http://www.gnu.org/software/octave)
interpreter. It is not intended to be a general purpose audio
editor, but a tool mostly designed for audio processing development and testing. If a
simultaneous use of Octave and Audacity (or a similar audio editor) is your common
layout, Octaudio might be the tool for you. It can also be useful for the studying of the
DSP and audio processing.

The main point of Octaudio is a combination of a WYSIWYG audio editing with a command or
scripting mode. It is somewhat like the Vi for audio. You can easily navigate, view,
record and listen to audio data, select tracks and data regions, and then execute octave
commands or scripts to analyze, process, or modify selected (or any other) data. You can
also record intermediate data or metrics of the processing into separate tracks to get
a detailed picture of the process or the analysis results. Finally, you can write
complex octave scripts to fully automate the operations. A script can prepare data,
create and configure all necessary tracks, perform actual processing, analyze the
results, and display everything in a convenient form with a single command. Surely
one can use the existing octave or matlab code directly in Octaudio.

##### Features

- Cross-platform.
- Multitrack data viewer provides fast and reliable data displaying on almost arbitrary
  scale in both directions.
- Track groups that are switched with tabs.
- Double precision arithmetic for all data including time and sample rate values.
- Multichannel, arbitrary length tracks.
- Two displaying modes (Normal and Abs Value) for both zero centered (like audio data)
  and positive (amplitude, power, etc.) signals.
- Hide or Show track option.
- Full-function octave console with multiline command editor, parenthesis matching,
  command history, and autocompletion.
- Async execution of octave commands.
- Additional built-in octave commands to access all data and object properties.
- Smart Tracks to display data from multiple tracks simultaneously.
- Monitors to display multiple tracks in separate windows with independent scales.
- Audio playback and recording. Full duplex recording is supported as well.
- Standard audio mixing options: mute, solo, gain, stereo pan.
- Tracks can be marked as non-audible to be completely ignored during playback (useful
  for average values, control signals, etc.).
- 3D plotting based on Qt5DataVisualization.

##### More Resources

- [General Overview](doc/overview.md)
- [Quick Tour](doc/tour.md)
- [Command Reference](doc/commands.md)
- [Shortcuts](doc/shortcuts.txt)
- See more information and screenshots on the [Octaudio wiki](https://github.com/antonrunov/octaudio/wiki).


##### Building

Build dependencies are [Octave](http://www.gnu.org/software/octave), [Qt5](http://download.qt.io/archive/qt),
[Portaudio V19](http://www.portaudio.com), [libsamplerate](http://www.mega-nerd.com/SRC),
and [CMake](http://www.cmake.org).
See more detailed instructions in [doc/build.txt](doc/build.txt).

