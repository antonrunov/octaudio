  Octaudio-0.0.6 readme
  =====================

  Introduction
  ------------

Octaudio is a multitrack audio editor with embedded GNU Octave interpreter
(http://www.gnu.org/software/octave). It is not intended to be a general purpose audio
editor, but a tool mostly designed for audio processing development and testing. If a
simultaneous use of Octave and Audacity (or a similar audio editor) is your common layout,
Octaudio might be a tool for you. It can also be useful for the studying of the DSP and
audio processing.

The main point of Octaudio is a combination of a WYSIWYG audio editing with a command or
scripting mode. It is somewhat like the Vi for audio. You can easily navigate, view,
record and listen to audio data, select tracks and data regions, and then execute octave
commands or scripts to analyze, process, or modify selected (or any other) data. You can
also record intermediate data or metrics of the processing into separate tracks to get
the detailed picture of the process or the analysis results. Finally, you can write
complex octave scripts to fully automate the operations. A script can prepare data,
create and configure all necessary tracks, perform actual processing, analyze the
results, and display all that stuff in a convenient form with a single command. Surely
one can use the existing octave or matlab code directly in Octaudio.


  Simple examples
  ---------------

There are few really quick and simple examples in this section, just to help you to
figure out what you can do with Octaudio.

Octaudio UI consists of two parts, the data viewer and the console. Unlike typical
consoles, octaudio console uses separated areas for command editor and output log. This
allows convenient editing of multiline commands, or even simple scripts, but to edit
more complex script one should place it in a file and use their favorite text editor. To
switch focus to the console press Ctrl+1. Ctrl+2 returns focus back to the data view. In
the console, use Enter to evaluate the command, Shift+Enter to insert a new line, and
Shift+Escape to clear the command line.

Let's create some data first.
  >
  oca_group_setprop( "rate", 44100 );
  N = 44100*3;
  K = sin( 2*pi*0.77 / 44100 * (1:N)' .* hamming(N) ) .* hanning(N);
  oca_track_add( "K" );
  oca_track_setprop( "audible", false, "K" );
  oca_data_set( 0, K, "K" );

  >
  oca_track_add( "signal" );
  oca_data_set( 0, 0.5*sin( 2*pi*450 / 44100 * (1:N)' .* ( 1 + 0.002*K ) ) .* K , "signal" );

We get two tracks here, one is the control signal and the other is the audio sample. The
first track is marked as non-audible, so it will not distort audio playback. Of course,
you can listen to the audio sample by clicking the playback button. To change time scale,
one can use Ctrl+ScrollUp/ScrollDown or Ctrl+Up/Down keys. But it is also possible to
control the view with octave commands.
  >
  oca_group_setprop( struct( "view_position", -1, "view_duration", 5 ) );

In the upper left corner of a track there is a track handle. It displays track name as
well as some other track info. For example, non-audible track is marked with a '#' sign.
Right clicking on the track handle opens a context menu for this track. More options are
available in the Track Properties dialog.

Vertical scale for a track can be changed with Shift+ScrollUp/ScrollDown or
Shift+Up/Down keys. There are also two special scale controls, located at the right side
of a track. The upper is for vertical scale, and the lower one is for zero offset.
Scale controls display the current value of the corresponding option. Also one can alter
the value with the scrolling. Scrolling speed can be changed with the Shift or Alt key
(fast or fine scrolling correspondingly). At last, by clicking the scale control one
can enter any value directly. Press Escape or click outside to leave focus.

Of course, you can also change the scales with the script.
  >
  oca_track_getprop( "scale", "signal" )
  oca_group_setprop( struct( "view_position", 0.90, "view_duration", 0.05 ) );
  oca_track_setprop( "scale", 0.1, {"K","signal"} );

  >
  oca_group_setprop( struct( "view_position", 2.57, "view_duration", 0.4 ) );
  oca_track_setprop( "scale", 0.1, {1,2} );
  oca_track_setprop( "height", 300, "signal" );

  >
  oca_group_setprop( struct( "view_position", -1, "view_duration", 5 ) );
  oca_track_setprop( "scale", 1, {"K","signal"} );
  oca_track_setprop( "height", 100, "signal" );

Octaudio console provides command history with incremental search. Just press Ctrl+Up in
the console. So by pressing Ctrl+Up and then "view" you will get the list of the last view
configuration commands. You can also get the list of completions for the word under the
cursor by pressing Tab. Incremental search works for the completion list as well.

Now we can calculate and display the average power of the signal.
  >
  oca_track_add( "power", 44.1 );
  oca_track_setprop( struct( "abs_value", true, "scale", 0.25 ), "power" );
  oca_data_set( 0, meansq( buffer( oca_data_get( "all", "signal" ), 1000 ) ), "power" );

The last command uses 'buffer' function from the octave signal package. Here we've created
a track with the sample rate of 44.1 Hz, so one point is 1000 samples average. Notice
the effect of the "abs_value" property, it turns on the view mode for positive data.
Zero line is at the bottom of the track now, and scale controls behave appropriately. Notice
also that Octaudio has made this track non-audible. Any track with the sample rate lower then
8000 Hz will be marked as non-audible by default. But you can always set the audible
property directly for any track.

Selecting a data region is easy.
  >
  oca_group_setprop( "region_start", min( find( K < -0.5 ) ) / 44100 );
  oca_group_setprop( "region_end", max( find( K < -0.5 ) ) / 44100 );

Of course, you can move region boundaries with a mouse, or set them directly with
Ctrl+Shift and Ctrl+Alt click (start and end positions respectively). With a region
defined one can access the corresponding data.
  >
  sqrt( meansq( oca_data_get( "region", "signal" ) ) )

To clear current region press Ctrl+Shift+A keys. One can specify time interval directly
as well.
  >
  sqrt( meansq( oca_data_get( [0.5,0.1], "signal" ) ) )

Here we have calculated an RMS for the interval of 0.5-0.6 sec. Time interval format is
[start_time, duration], and all values are in seconds.

It is also possible to display several tracks together within a smart track.
  >
  oca_track_add( "smart track", {"K","signal"} );

Smart track displays the data of the original track, not its own data. When the original track
data is being changed, you see this changes in the smart track immediately.
  >
  oca_data_set( 1.5, zeros(1,3000), "signal" )

One can add more subtracks to the smart track.
  >
  oca_subtrack_add( "power", "smart track" );

And change the subtrack color.
  >
  oca_subtrack_setprop( "color", [200,200,0], "signal", "smart track" );

Again, the smart track and its subtracks can also be edited with the Properties dialog.
To edit subtrack list, check on the 'Edit Track List' checkbox.

If you have many tracks, it's time to make use of track groups.
  >
  id_new_group = oca_group_add( "New Group" );
  oca_global_setprop( "active_group", id_new_group );

Finally let's load audio from a wav file.
  >
  oca_loadwavm( "my_audio_file.wav" )

Of course, you should enter a path to the existing file here. Autocompletion can help
you here too. Actually, if you press the Tab button when the cursor is inside a string,
the file selection dialog will be opened.


  General Overview
  ----------------

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
the detailed list of the types and commands later.

The operand object for a command can be specified by one of the following ways.

- By default, the active object of the appropriate type is used.

- Using the object ID. Every octaudio object has an unique ID, represented as a numeric
  array in octave. Object ID should be considered as opaque data, so one needs to get
  it first. For example, with the 'find' or 'list' command for appropriate object type.

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
  object is deleted or moved. One can use appropriate 'list' command to get the
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

      oca_track_remove( {"foo"} )

  deletes all tracks with the name "foo" from the active group, while the command

      oca_track_remove( "foo" )

  fails if the active group contains more then one track with the name "foo".

The following notation will be used below for specifying command arguments.
  id  - single objects address, which can be
        ID      - unique object ID, numeric array
        name    - object name, string
        idx     - object index, integer

  idn - multiple objects address, which can be
        id                - single address
        { id1, id2, ... } - cell array of single addresses

  IDn - { ID1, ID1, ... } - cell array of object IDs

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


  Octaudio octave commands
  ------------------------

- Object properties handling. There is a number of commands to work with the object
  properties. Although these commands will be listed in the later sections, they have
  common arguments explained here. These commands have one of the following forms.

    vals = oca_TYPE_getprop( [""], [id], ... )
    vals = oca_TYPE_getprop( "name", [idn], ... )
    vals = oca_TYPE_getprop( {"name1", "name2", ... }, [id], ... )
    oca_TYPE_setprop( "name", value, [idn] )
    oca_TYPE_setprop( struct( "name1", val1, "name2", val2, ... ), [idn], ... )

  Here and later the square brackets denote optional arguments. The first form of the
  getprop command returns a scalar structure with the all available object properties.
  The second form returns either a single value if the idn addresses a single object, or
  a cell array. The third form always returns a cell array. The setprop command accepts
  either property name and value arguments, or a scalar structure with property
  names as keys. The second form allows to set multiple properties for multiple
  objects. The function returns the total number of modified properties.

  The next properties are defined for all octaudio objects.
  - "name", the name of the object, should not be empty.
  - "display_name", if not empty, this string will be displayed in the UI instead of
    the object name. Empty by default.
  - "display_text" (read only), the actual string that is displayed in the UI. That is
    either display_name or the name.


- Data commands. Track data samples are addressed by the time in Octaudio, not by index.
  Most commands below also take the 't_spec' argument, that specifies a time
  interval for the data. t_spec can be either an explicit interval in the form
  [ t0, duration ], or one of the strings "cursor", "region", "all", "".
    "cursor"  - refers to a single sample at the time of the track group cursor.
    "region"  - is a selected time region of the track group. The command will fail if
                no region is defined in the group.
    "all"     - refers to full track data and is equal to the [ -inf, inf ] interval.
    ""        - default interval, that is either a selected time region if it is
                defined, or a cursor time point.

  Below is a full list of data commands.

  [data, t0] = oca_data_get( [t_spec], [id], [group_id] )
      Read the track data as a numeric array. 'id' is the track id, and 'group_id'
      specifies the container (group) for the track. The second returned value, 't0' is
      the actual time of the first returned sample.

      This command always returns a single data block. It means that all samples in the
      returned array are equally spaced and their times are t0 + [0:n-1] / sample_rate.
      If there are several data blocks in the specified region, 'oca_data_get' will
      return only the first one.

  t_next = oca_data_set( t, data, [id], [group_id] )

      Write numeric array data to the track, starting with the time t. If the track
      already contains a data block at the time t, the nearest existing sample will be
      overwritten with data(1) and so on. Otherwise the new data block starting with
      time t will be created. The command returns the time of the sample (not necessary
      existing) following the last written sample. Thus the sequential commands
        t = oca_data_set( t, data1 );
        t = oca_data_set( t, data2 );
        ...
      will always write a continuous data block with concatenated data.

  ret = oca_data_clear( [idn], [group_id] )
      Remove all data from the specified track or tracks.

  list = oca_data_listblocks( [t_spec], [id], [group_id] )
      List data blocks  within the specified interval 't_spec'.
      Returns 2 x N matrix. Each column is a vector of the form [time; duration], thus
      list(1,:) gives the starting times of the blocks, and list(2,:) gives the
      durations.


- Track commands. For track operations, the operand id should be a track or a smart
  track, and container is a group.

  [ID, idx ] = oca_track_add( name, [rate], [groip_id] )
      Add a new track to the group. 'name' is a new track name, and 'rate' is a sample
      rate (in Hz). By default, the group's rate will be used. The command returns the ID
      of the new track and its index in the group. The last is actually a new number of
      tracks.

  [ID, idx ] = oca_track_add( name, {subtracks},  [group_id]  )
      Add a new smart track to the group. Subtracks must be a cell array of data track
      ids. One can specify an empty cell array to create empty smart track.

  ret = oca_track_remove( [idn], [group_id] )
      Remove track(s). Returns the number of deleted tracks.

  idx = oca_track_move( idx, [id], [target_group_id], [group_id] )
      Move track within its group or to another group (with 'target_group_id'). Returns
      the previous track index, or -1 if the move was not performed.

  IDn = oca_track_list( [group_id] )
      Get IDs of all tracks in the group.

  count = oca_track_count( [group_id] )
      Get the number of tracks in the group.

  ID  = oca_track_find( id, [group_id] )
  IDn = oca_track_find( idn, [group_id] )
      Get the ID(s) of the specified track(s).

  vals = oca_track_getprop( [names], [idn], [group_id] )
  ret = oca_track_setprop( name, val, [idn], [group_id] )
  ret = oca_track_setprop( props, [idn], [group_id] )
      Handle track properties. The following properties are defined for all tracks:
      - "start" (read only), start time of track data, in seconds
      - "end" (read only), end time of track data, in seconds
      - "duration" (read only), it is just equal to end - start
      - "scale", vertical scale
      - "zero", zero offset
      - "abs_value", boolean flag
      - "selected", boolean flag
      - "hidden", boolean flag
      - "index" (read only), the track index in the group
      - "height", the track height in pixels
      Properties, specific for the data tracks:
      - "muted", boolean flag
      - "readonly", boolean flag
      - "rate", track data sample rate in Hz, double
      - "audible", boolean flag
      - "gain", gain for audio mixing, double
      - "stereo_pan", pan for audio mixing, valid range is from -1.0 (left) to 1.0 (right)
      Properties, specific for the smart tracks:
      - "common_scale", boolean, true if all subtracks are displayed with the same scale
      - "active_subtrack", active subtrack ID
      - "aux_transparency", controls the visibility of inactive subtracks, 0.0 - 1.0

  [ val, ret ] = oca_track_getcontext( field, [id], [group_id] )
  [ val, ret ] = oca_track_getcontext( { field, default_value }, [id], [group_id] )
      Read track context field. Default value can be specified. The second return value
      'ret' will be 1 if the specified field exists and 0 otherwise.

  oca_track_setcontext( field, value, [id], [group_id] )
      Write the value to the track context.


- Group commands. The operand object is a group. Container is not specified.

  [ID, idx] = oca_group_add( name )

  ret = oca_group_remove( [idn] )

  idx = oca_group_move( idx, [id] )

  [IDn] = oca_group_list()

  count = oca_group_count()

  IDn = oca_group_find( idn )

  vals = oca_group_getprop( [names], [idn] )
  ret = oca_group_setprop( name, val, [idn] )
  ret = oca_group_setprop( props, [idn] )
      Group properties are:
      - "rate", default sample rate for new tracks
      - "start" (read only), start time (the minimum start time of the group's tracks)
      - "end" (read only), end time
      - "duration" (read only), duration time
      - "cursor", time cursor position, seconds
      - "region_start", selected time region start, seconds
      - "region_end", selected time region end, seconds
      - "region_duration" (read only), selected time region duration, seconds
      - "view_position", start time of the currently displayed area, seconds
      - "view_duration", duration of the currently displayed area, seconds
      - "active_track", active track ID
      - "solo_track", solo track ID, can be null ID if no solo track selected
      - "rec_track1", recording track for the left channel, can be null ID
      - "rec_track2", recording track for the right channel, can be null ID

  [ val, ret ] = oca_group_getcontext( field, [id] )
  [ val, ret ] = oca_group_getcontext( { field, default_value }, [id] )
  oca_group_setcontext( field, value, [id] )


- Monitor commands. The operand object is a monitor. Container is not specified.

  [ID, idx] = oca_monitor_add( name, [group_id] )
      Create monitor. The view position of the monitor will be synced with the time
      cursor of the specified group (or the active group). Use group_id = -1 to create
      monitor without a group.

  ret = oca_monitor_remove( [idn] )

  [IDn] = oca_monitor_list()

  count = oca_monitor_count()

  IDn = oca_monitor_find( idn )

  vals = oca_monitor_getprop( [names], [idn] )
  ret = oca_monitor_setprop( name, val, [idn] )
  ret = oca_monitor_setprop( props, [idn] )
      Monitors have the same properties as smart tracks. Additional ones are:
      - "group", the synced group ID, can be null ID
      - "cursor", time cursor position, seconds
      - "time_scale", visible area duration, seconds


- Subtrack commands. The operand for the subtrack commands is its linked data track id.
  Container can be either a smart track or a monitor. To specify a monitor as a
  container, one should use group_id = -1, otherwise smart tracks will be used.

  idx = oca_subtrack_add( idn, [container_id], [group_id] )
      Add subtracks to the smart track or monitor. 'idn' is id(s) of original data
      track(s).

  idx = oca_subtrack_remove( [idn], [container_id], [group_id] )

  idx = oca_subtrack_move( idx, [id], [container_id], [group_id] )

  [IDn] = oca_subtrack_list( [container_id], [group_id] )
      List the subtracks in the smart track or monitor. The returned values are the IDs
      of the data tracks, linked with the subtracks.

  count = oca_subtrack_count( [container_id], [group_id] )

  IDn = oca_subtrack_find( idn, [container_id], [group_id] )

  vals = oca_subtrack_getprop( [names], [idn], [container_id], [group_id] )
  ret = oca_subtrack_setprop( [name], [val], [idn], [container_id], [group_id] )
  ret = oca_subtrack_setprop( [props], [idn], [container_id], [group_id] )
      The subtrack has the most properties of the linked data track, and additionally:
      - "color", the display color for subtrack data, in the form [r,g,b]
      - "index", the subtrack index in its container (smart track or monitor)


- Context commands. These commands work with objects of any type, and accept only the
  unique ID as an object address.

  [val, ret] = oca_context_get( ID, name, [default_value] )
      Get the context field 'name' for the object 'ID'. The second return value 'ret'
      will be 1 if the specified field exists and 0 otherwise.

  context = oca_context_get( ID )
      Return the whole context for the object 'ID' as a scalar structure.

  oca_context_set( ID, name, val )
      Set the conext field 'name'.

  oca_context_set( ID, context )
      Set the whole context for the object. 'context' must be a scalar structure.

  ret = oca_context_remove( ID, name )
      Delete field 'name' from the object's context.


- Global commands. These commands have no operand object and perform operations in a
  global scope.

  oca_global_getinfo()
    Get octaudio build and version information.

  oca_global_listaudiodevs( dev_type )
    List available audio devices. 'dev_type' should be either "input" or "output".

  vals = oca_global_getprop( [names] )
  ret = oca_global_setprop( name, val )
  ret = oca_global_setprop( props )"  )
    Global properties are:
    - "name", "display_name", "display_text", control the name of octaudio main window
    - "audio_samplerate", the sample rate for audio playback and recording device
    - "default_rate", the default sample rate for new groups
    - "active_group", active group ID
    - "output_device", output audio device, string
    - "input_device", input audio device, string


- Utility commands. There are a few useful utility scripts, bundled with Octaudio. These
  scripts are located at OCTAUDIO_PREFIX/share/octaudio/m/utils/ directory.

  oca_loadwav( filename, [track] )
      Load a data from the wav file to the specified track, or create a new one if the
      track is not specified. Multichannel audio is mixed into a single channel.

  oca_loadwavm( filename, [basename] )
      Load a data from the wav file. Each channel is loaded into a new track. Track names
      are generated from the basename or from the filename.

  oca_savewav( filename, track=0 )
      Save the track data to wav file.

  oca_spectrum( w=[], track=0, limit=-120, linfreq=false )
      Display spectrum for the selected region of the track. This command uses 'pwelch'
      function from the octave signal packet. 'w' is a window length or a window data,
      and is passed as is to the 'pwelch' function. 'limit' is a lower limit for data
      plotting in dB.


  Shortcuts
  ---------

- Global
    Ctrl+T                  - add group
    Ctrl+Shift+T            - open new group dialog
    Ctrl+W                  - delete active group
    Ctrl+G                  - open group properties
    Ctrl+PgDown, Ctrl+PgUp  - next / previous group
    Ctrl+Shift+A            - clear time region
    Ctrl+`                  - hide console
    Ctrl+1                  - show and focus console
    Ctrl+2                  - focus data view
    Alt+A                   - center view on audio position
    Alt+Shift+M             - open new monitor dialog
    Ctrl+N                  - add track
    Ctrl+Shift+N            - open new track dialog
    Alt+Shift+S             - open new smart track dialog
    Ctrl+K                  - delete active track
    Ctrl+Shift+K            - delete selected tracks
    Alt+Shift+H             - hide active track
    Ctrl+Shift+H            - hide selected tracks
    Ctrl+I                  - toggle track selection
    Ctrl+P                  - open active track properties dialog
    Ctrl+Shift+P            - start playback
    Ctrl+Shift+R            - start recording
    Ctrl+Shift+Space        - pause audio
    Ctrl+Shift+S            - stop audio

- Console
    Enter, Return           - evaluate command
    Shift+Enter             - new line
    Shift+Esc               - clear command editor
    Ctrl+Up                 - open command history
    Tab                     - show completions
    Ctrl+Z                  - undo
    Ctrl+Shift+Z            - redo
    Ctrl+A                  - select all
    Ctrl+C                  - copy
    Ctrl+V                  - paste
    Ctrl+X                  - cut

- Data View
    Ctrl+U, Ctrl+D                    - move track up / down
    Up, Down                          - next / previous track
    Home                              - set cursor and view to the group start
    End                               - set cursor and view to the group end
    Ctrl+Up, Ctrl+Down                - change time scale
    Left, Right                       - move cursor
    Ctrl+Left, Ctrl+Right             - move cursor (fine)
    Shift+Left, Shift+Right           - move view position
    Alt+Shift+Left, Alt+Shift+Right   - move view position (fast)
    Ctrl+Shift+Left, Ctrl+Shift+Right - move view position (fine)
    Ctrl+[, Ctrl+]                    - move cursor by 1 sample
    Alt+[, Alt+]                      - move cursor and view
    Alt+Shift+[, Alt+Shift+]          - move cursor and view (fast)

    ScrollUp, ScrollDown              - vertical scroll
    Ctrl+ScrollUp, Ctrl+ScrollDown    - change time scale
    Shift+ScrollUp, Shift+ScrollDown  - change vertical scale

    ScrollLeft, ScrollRight                       - move view position
    Shift+ScrollLeft, Shift+ScrollRight           - move view position (fast)
    Alt+Shift+ScrollLeft, Alt+Shift+ScrollRight   - move view position (fine)

    Ctrl+ScrollLeft, Ctrl+ScrollRight             - move cursor and view
    Ctrl+Shift+ScrollLeft, Ctrl+Shift+ScrollRight - move cursor and view (fast)
    Ctrl+Alt+ScrollLeft, Ctrl+Alt+ScrollRight     - move cursor and view (fine)

    Shift+Up, Shift+Down              - change vertical scale
    Alt+Up, Alt+Down                  - change zero offset

    Alt+N, Alt+P                      - next / previous subtrack (in smart track)

- Scale Control
    Click                             - take focus
    Esc                               - abandon focus
    Up, Down                          - change value
    Shift+Up, Shift+Down              - change value (fast)
    Ctrl+Up, Ctrl+Down                - change value (fine)
    ScrollUp, ScrollDown              - change value
    Sift+ScrollUp, Shift+ScrollDown   - change value (fast)
    Ctrl+ScrollUp, Ctrl+ScrollDown    - change value (fine)

- Monitor
    ScrollUp, ScrollDown              - move cursor and view
    Ctrl+ScrollUp, Ctrl+ScrollDown    - change time scale
    Shift+ScrollUp, Shift+ScrollDonw  - change vertical scale
