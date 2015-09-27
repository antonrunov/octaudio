#### General Overview

The main objects in Octaudio are tracks and track groups. Track is a container for data
blocks at the specific sample rate. Track data is always a double precision floating
point. There are no limits for data values, any scale can be used.

Group is a container for tracks. Groups appear as tabs in the main window. They help
organize and navigate the number of tracks. Also, groups hold all time related
data, that is time cursor, time scale, view position, and selected time region. Audio
playback and recording work with groups as well.

Currently visible group is the active group. Similarly, each group has the active track
indicated with a frame. Most operations are applied to the active group or its
active track by default.

Each track belongs to exactly one group, so you cannot add the same track into several
groups. But there are other types of objects, which are smart tracks and monitors, to
display the track data in several groups, or view tracks from different groups
simultaneously.

Smart track is a special type of track. It does not contain data, but the list of
subtracks, which are the links to the data of the other tracks. The subtracks are
displayed the same way as the original track data, so one can view several tracks
combined within the smart track. A subtrack can link any data track from any group, so
this is also a way to display tracks from other groups. Any number of smart tracks can
display the same data track.

Monitor is similar to track in that it contains and displays subtracks. But unlike smart
track, a monitor is not a member of a track group, and thus it has its own time data.
Generally, its view position is synced with the group's time cursor, but the time scale is
independent. It allows viewing the track (or several tracks) at different time scales.
One can use a monitor as a magnifying glass or, reversed, as an overview map.
In the UI a monitor is represented as a dock window, so one can arrange their data
views. When a monitor is synced with a group, it is visible only when the group is
active, and will be hidden otherwise. But it is also possible to disconnect the monitor
from any group, in this case both its scale and view position will be independent, and the
monitor will be shown permanently.

Although Octaudio has some common audio editor functionality, WYSIWYG editing is not the
way it is intended to be used. It is the octave console that gives Octaudio the power
and flexibility. Octaudio is designed in a way that almost any operation can be
implemented with an octave script.

There is a number of special built-in commands to access objects and their
properties in Octaudio. These commands are grouped by the type of the operand, and are
named by "oca_TYPE_OPNAME" pattern. For example oca_track_add, oca_data_get, etc. See
the detailed list of the types and commands in the
[commands](commands.md) page.

The operand object for a command can be specified by one of the following ways.

- By default, the active object of the appropriate type is used.

- Using the object ID. Every octaudio object has an unique ID, represented as a numeric
  array in octave. Object ID should be considered as opaque data, so one needs to get
  it first. For example, with the `find` or `list` command for appropriate object type.

- Using the object name. String argument is interpreted as an object name.
  Octaudio will search for objects with the specified name in the active container.
  One can also specify another container object with additional argument. Each command
  type implies the type of the container to be used. For example, container for track
  operations is the track group. Top level objects, which are groups and monitors,
  always have the single global container, so it cannot be altered.

  Object names do not have to be unique, so when addressing an object with a name, a
  collision may occur. In this case, behavior depends on the context. When addressing
  a single object, the command will fail in the case of the collision. When addressing
  multiple objects (see later), all objects with the specified name will be processed.

- Addressing the object with its index in the container. Numeric argument is interpreted
  as an index. The rules for specifying the container object are the same as for the names.
  The objects in a container are numbered sequentially from 1 up to the number of
  objects. The object's index is not permanent though, and can be changed when another
  object is deleted or moved. One can use appropriate `list` command to get the
  list of objects in the container. The indices of the tracks and groups are displayed
  in the UI before their names.

  There are two special indices, 0 and -1. Zero index refers to the current active
  object of the container, and this is the default argument. -1 means non-existing
  (null) object.

- Addressing multiple objects. Some commands can operate with several objects at once.
  These commands additionally accept cell array as an object address. The cell array can
  contain any number of elements listed above, namely object IDs, names, and indices.
  When using a name in the cell array, in the case of name collision, all objects
  with that name will be addressed.

  For example, the command
```
  oca_track_remove( {"foo"} )
```
  deletes all tracks with the name "foo" from the active group, while the command
```
  oca_track_remove( "foo" )
```
  fails if the active group contains more then one track with the name "foo".

The following notation will be used below for specifying command arguments.
  - `id`  - single objects address, which can be
    - `ID`      - unique object ID, numeric array
    - `name`    - object name, string
    - `idx`     - object index, integer

  - `idn` - multiple objects address, which can be
    - `id`               - single address
    - `{ id1, id2, ... }` - cell array of single addresses

  - `IDn` - `{ ID1, ID1, ... }` - cell array of object IDs

Each octaudio object type has a number of named properties. For example, a track group has
the "cursor" and "view_position" properties, and a track has the "scale" property. One
can control all aspects of the object behavior and view with its properties.

Another common object option is its context. All octaudio objects have an associated
scalar structure called context. A context is empty by default and is intended to be used by
scripts, not by Octaudio itself. It is just a way to associate arbitrary data with the
object. For example, one can save track metadata in its context. Another example is that
specific testing parameters can be saved in the group context, so you can switch the
test conditions easily by switching the group. The context of the object will be deleted
automatically when the object is deleted.



