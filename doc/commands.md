#### Octaudio Command Reference

See [overview](overview.md) page for the main concepts explanations.


##### Object properties handling

There is a number of commands to work with the object
properties. Although these commands will be listed in the later sections, they have
common arguments explained here. These commands have one of the following forms.
```
  vals = oca_TYPE_getprop( [""], [id], ... )
  vals = oca_TYPE_getprop( "name", [idn], ... )
  vals = oca_TYPE_getprop( {"name1", "name2", ... }, [id], ... )
  oca_TYPE_setprop( "name", value, [idn] )
  oca_TYPE_setprop( struct( "name1", val1, "name2", val2, ... ), [idn], ... )
```
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


##### Data commands.

Track data samples are addressed by the time in Octaudio, not by index.
Most commands below also take the `t_spec` argument, that specifies a time
interval for the data. t_spec can be either an explicit interval in the form
`[ t0, duration ]`, or one of the strings "cursor", "region", "all", "".
- "cursor", refers to a single sample at the time of the track group cursor.
- "region", is a selected time region of the track group. The command will fail
  if no region is defined in the group.
- "all", refers to full track data and is equal to the `[ -inf, inf ]` interval.
- "", default interval, that is either a selected time region if it is 
  defined, or a cursor time point.

Below is a full list of data commands.
```
  [data, t0] = oca_data_get( [t_spec], [id], [group_id] )
```
Read the track data as a numeric array with the channels represented as rows.
`id` is the track id, and `group_id` specifies the container (group) for the
track. The second returned value, `t0` is the actual time of the first returned
sample.

This command always returns a single data block. It means that all samples in the
returned array are equally spaced and their times are `t0 + [0:n-1] / sample_rate`.
If there are several data blocks in the specified region, `oca_data_get` will
return only the first one. Use `oca_data_getblocks` to get all blocks in the region.
```
  t_next = oca_data_set( t, data, [id], [group_id] )
```
Write the numeric array `data` to the track, starting with the time t.  If the
track has a single channel, the data must be a vector. For multichannel tracks
the data must be a matrix with the rows corresponding to the track channels.
If the track already contains a data block at the time t, the nearest existing
sample will be overwritten with data(1) and so on. Otherwise the new data block
starting with time t will be created. The command returns the time of the
sample (not necessary existing) following the last written sample. Thus the
sequential commands
```
    t = oca_data_set( t, data1 );
    t = oca_data_set( t, data2 );
    ...
```
will always write a continuous data block with concatenated data.

```
  t_next = oca_data_fill( pattern, [t_spec], [id], [group_id] )
```
Fill the region with the pattern.

```
  ret = oca_data_clear( [idn], [group_id] )
```
Remove all data from the specified track or tracks.

```
  oca_data_delete( [t_spec], [id], [group_id] )
  [blocks, starts] = oca_data_delete( [t_spec], [id], [group_id] )
```
Delete data within the specified interval `t_spec`. Returns deleted data blocks
as a cell array of numeric arrays. The second returned value `starts` is an array
of start positions of the data blocks;

```
  list = oca_data_listblocks( [t_spec], [id], [group_id] )
```
List data blocks  within the specified interval `t_spec`.
Returns 2 x N matrix. Each column is a vector of the form `[time; duration]`, thus
`list(1,:)` gives the starting times of the blocks, and `list(2,:)` gives the
durations.

```
  [blocks, starts] = oca_data_getblocks( [t_spec], [id], [group_id] )
```
Get all data blocks within the specified interval `t_spec`. Returned values are the
same as for `oca_data_delete` function.

```
  dt = oca_data_moveblocks( dt, [t_spec], [id], [group_id] )
```
Shift the data blocks on `dt` or up to the next block. Returns the actual shift
value.

```
  t_split = oca_data_split( ["cursor"], [id], [group_id] )
  t_split = oca_data_split( t, [id], [group_id] )
```
Split data block at the time t or the cursor position. Returns start time of the
new block, or `nan` if there is no data block at the specified position.

```
  ret = oca_data_join( [t_spec], [id], [group_id] )
```
Join all data blocks within the specified interval into single block. Returns the
number of joined blocks.


##### Track commands

For track operations, the operand `id` should be a track or a smart track, and container is a group.

```
  [ID, idx ] = oca_track_add( name, [rate], [groip_id] )
```
Add a new track to the group. `name` is a new track name, and `rate` is a sample
rate (in Hz). By default, the group's rate will be used. The command returns the ID
of the new track and its index in the group. The last is actually a new number of
tracks.

```
  [ID, idx ] = oca_track_add( name, {subtracks},  [group_id]  )
```
Add a new smart track to the group. Subtracks must be a cell array of data track
ids. One can specify an empty cell array to create empty smart track.

```
  ret = oca_track_remove( [idn], [group_id] )
```
Remove track(s). Returns the number of deleted tracks.

```
  idx = oca_track_move( idx, [id], [target_group_id], [group_id] )
```
Move track within its group or to another group (with 'target_group_id'). Returns
the previous track index, or -1 if the move was not performed.

```
  IDn = oca_track_list( [group_id] )
```
Get IDs of all tracks in the group.

```
  count = oca_track_count( [group_id] )
```
Get the number of tracks in the group.

```
  ID  = oca_track_find( id, [group_id] )
  IDn = oca_track_find( idn, [group_id] )
```
Get the ID(s) of the specified track(s).

```
  vals = oca_track_getprop( [names], [idn], [group_id] )
  ret = oca_track_setprop( name, val, [idn], [group_id] )
  ret = oca_track_setprop( props, [idn], [group_id] )
```
Handle track properties. The following properties are defined for all tracks:
- "start", start time of track data, in seconds
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
- "channels", the number of channels in the track; this property can be changed
  only when the track is empty
- "audible", boolean flag
- "gain", gain for audio mixing, double
- "stereo_pan", pan for audio mixing, valid range is from -1.0 (left) to 1.0 (right)

Properties, specific for the smart tracks:
- "common_scale", boolean, true if all subtracks are displayed with the same scale
- "active_subtrack", active subtrack ID
- "aux_transparency", controls the visibility of inactive subtracks, 0.0 - 1.0

```
  [ val, ret ] = oca_track_getcontext( field, [id], [group_id] )
  [ val, ret ] = oca_track_getcontext( { field, default_value }, [id], [group_id] )
```
Read track context field. Default value can be specified. The second return value
`ret` will be 1 if the specified field exists and 0 otherwise.

```
  oca_track_setcontext( field, value, [id], [group_id] )
```
Write the value to the track context.


##### Group commands

The operand object is a group. Container is not specified.

```
  [ID, idx] = oca_group_add( name )
```
```
  ret = oca_group_remove( [idn] )
```
```
  idx = oca_group_move( idx, [id] )
```
```
  [IDn] = oca_group_list()
```
```
  count = oca_group_count()
```
```
  IDn = oca_group_find( idn )
```
```
  vals = oca_group_getprop( [names], [idn] )
  ret = oca_group_setprop( name, val, [idn] )
  ret = oca_group_setprop( props, [idn] )
```
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

```
  [ val, ret ] = oca_group_getcontext( field, [id] )
  [ val, ret ] = oca_group_getcontext( { field, default_value }, [id] )
  oca_group_setcontext( field, value, [id] )
```


##### Monitor commands

The operand object is a monitor. Container is not specified.

```
  [ID, idx] = oca_monitor_add( name, [group_id] )
```
Create monitor. The view position of the monitor will be synced with the time
cursor of the specified group (or the active group). Use group_id = -1 to create
monitor without a group.

```
  ret = oca_monitor_remove( [idn] )
```
```
  [IDn] = oca_monitor_list()
```
```
  count = oca_monitor_count()
```
```
  IDn = oca_monitor_find( idn )
```
```
  vals = oca_monitor_getprop( [names], [idn] )
  ret = oca_monitor_setprop( name, val, [idn] )
  ret = oca_monitor_setprop( props, [idn] )
```
Monitors have the same properties as smart tracks. Additional ones are:
- "group", the synced group ID, can be null ID
- "cursor", time cursor position, seconds
- "time_scale", visible area duration, seconds


##### Subtrack commands

The operand for the subtrack commands is its linked data track id.  Container can be
either a smart track or a monitor. To specify a monitor as a container, one should use
`group_id` = 1, otherwise smart tracks will be used.

```
  idx = oca_subtrack_add( idn, [container_id], [group_id] )
```
Add subtracks to the smart track or monitor. `idn` is id(s) of original data
track(s).

```
  idx = oca_subtrack_remove( [idn], [container_id], [group_id] )
```
```
  idx = oca_subtrack_move( idx, [id], [container_id], [group_id] )
```
```
  [IDn] = oca_subtrack_list( [container_id], [group_id] )
```
List the subtracks in the smart track or monitor. The returned values are the IDs
of the data tracks, linked with the subtracks.

```
  count = oca_subtrack_count( [container_id], [group_id] )
```
```
  IDn = oca_subtrack_find( idn, [container_id], [group_id] )
```
```
  vals = oca_subtrack_getprop( [names], [idn], [container_id], [group_id] )
  ret = oca_subtrack_setprop( [name], [val], [idn], [container_id], [group_id] )
  ret = oca_subtrack_setprop( [props], [idn], [container_id], [group_id] )
```
The subtrack has the most properties of the linked data track, and additionally:
- "color", the display color for subtrack data, in the form `[r,g,b]`
- "index", the subtrack index in its container (smart track or monitor)


##### 3D plotting commands

The operand object is a plot3d instance. Container is not specified.

```
  [ID, idx] = oca_plot3d_add( name )
```
Create plot3d.
```
  oca_plot3d_set( data, [id] )
```
Set plot data. `data` should be a matrix. The corresponding values will be displayed
as a 3D surface.
```
  oca_plot3d_settexture( texture, [id] )
```
Set a texture to the existing data. `texture` should be a matrix of the same size as the data.
Values are colors in RGB format (0xff0000 for red, etc.). Value `nan` allows the default coloring for
the corresponding surface points.
```
  ret = oca_plot3d_remove( [idn] )
```
```
  [IDn] = oca_plot3d_list()
```
```
  count = oca_plot3d_count()
```
```
  IDn = oca_plot3d_find( idn )
```
```
  vals = oca_plot3d_getprop( [names], [idn] )
  ret = oca_plot3d_setprop( name, val, [idn] )
  ret = oca_plot3d_setprop( props, [idn] )
```
Plot3d specific properties are:
- "x_len", the number of rows in the current data
- "y_len", the number of columns in the current data
- "x_origin", the x-coordinate of the first data value
- "x_step", the distance between data points along the x axis
- "y_origin", the y-coordinate of the first data value
- "y_step", the distance between data points along the y axis
- "x_pos", the x-coordinate of the center of displayed area
- "x_scale", the scale of the displayed area
- "y_pos", the y-coordinate of the center of displayed area
- "y_scale", the scale of the displayed area
- "z_min", the minimum displayed value
- "z_scale", the vertical scale
- "x_sel_pos" (read only), the x-coordinate of the selected point or `nan`
- "y_sel_pos" (read only), the y-coordinate of the selected point or `nan`
- "aspect_ratio", the aspect ratio
- "horiz_ratio", the horizontal aspect ratio


##### Context commands

These commands work with objects of any type, and accept only the unique ID as an object
address.

```
  [val, ret] = oca_context_get( ID, name, [default_value] )
```
Get the context field `name` for the object `ID`. The second return value `ret`
will be 1 if the specified field exists and 0 otherwise.

```
  context = oca_context_get( ID )
```
Return the whole context for the object `ID` as a scalar structure.

```
  oca_context_set( ID, name, val )
```
Set the context field `name`.

```
  oca_context_set( ID, context )
```
Set the whole context for the object. `context` must be a scalar structure.

```
  ret = oca_context_remove( ID, name )
```
Delete field `name` from the object's context.


##### Global commands

These commands have no operand object and perform operations in a global scope.

```
  oca_global_getinfo()
```
Get octaudio build and version information.

```
  oca_global_listaudiodevs( dev_type )
```
List available audio devices. `dev_type` should be either "input" or "output".

```
  vals = oca_global_getprop( [names] )
  ret = oca_global_setprop( name, val )
  ret = oca_global_setprop( props )
```
Global properties are:
- "name", "display_name", "display_text", control the name of octaudio main window
- "audio_samplerate", the sample rate for audio playback and recording device
- "default_rate", the default sample rate for new groups
- "active_group", active group ID
- "output_device", output audio device, string
- "input_device", input audio device, string
- "cache_dir", data cache directory (make shure you have enough space there)


##### Utility commands

There are a few useful utility scripts, bundled with Octaudio. These
scripts are located at `OCTAUDIO_PREFIX/share/octaudio/m/utils/` directory.

```
  oca_loadwav( filename, [track], [mono] )
```
Load a data from the wav file to the specified track, or create a new one if the
track is not specified. When `mono=true` is specified, multichannel
audio is mixed into a single channel.

```
  oca_loadwavm( filename, [basename] )
```
Load a data from the wav file. Each channel is loaded into a new track. Track names
are generated from the basename or from the filename.

```
  oca_savewav( filename, track=0 )
```
Save the track data to wav file.

```
  oca_spectrum( w=[], track=0, limit=-120, linfreq=false )
```
Display spectrum for the selected region of the track. This command uses `pwelch`
function from the octave signal packet. `w` is a window length or a window data,
and is passed as is to the `pwelch` function. `limit` is a lower limit for data
plotting in dB.

