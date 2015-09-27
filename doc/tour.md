#### Quick Tour

There are few really quick and simple examples in this section, just to help you to
figure out what you can do with Octaudio. See
[demos](https://github.com/antonrunov/octaudio/wiki/Demo:-dynamic-range-compression) page for more
complicated examples.

Octaudio UI consists of two parts, the data viewer and the console. Unlike typical
consoles, octaudio console uses separated areas for command editor and output log. This
allows convenient editing of multiline commands, or even simple scripts, but to edit
more complex script one should place it in a file and use their favorite text editor. To
switch focus to the console press `Ctrl+1`. `Ctrl+2` returns focus back to the data view. In
the console, use `Enter` to evaluate the command, `Shift+Enter` to insert a new line, and
`Shift+Escape` to clear the command line.

Let's create some data first.
  ```
  oca_group_setprop( "rate", 44100 );
  N = 44100*3;
  K = sin( 2*pi*0.77 / 44100 * (1:N)' .* hamming(N) ) .* hanning(N);
  oca_track_add( "K" );
  oca_track_setprop( "audible", false, "K" );
  oca_data_set( 0, K, "K" );
  ```
  ```
  oca_track_add( "signal" );
  oca_data_set( 0, 0.5*sin( 2*pi*450 / 44100 * (1:N)' .* ( 1 + 0.002*K ) ) .* K , "signal" );
  ```
We get two tracks here, one is the control signal and the other is the audio sample. The
first track is marked as non-audible, so it will not distort audio playback. Of course,
you can listen to the audio sample by clicking the playback button. To change time scale,
one can use `Ctrl+ScrollUp/ScrollDown` or `Ctrl+Up/Down` keys. But it is also possible to
control the view with octave commands.
  ```
  oca_group_setprop( struct( "view_position", -1, "view_duration", 5 ) );
  ```
In the upper left corner of a track there is a track handle. It displays track name as
well as some other track info. For example, non-audible track is marked with a '#' sign.
Right clicking on the track handle opens a context menu for this track. More options are
available in the **Track Properties** dialog.

Vertical scale for a track can be changed with `Shift+ScrollUp/ScrollDown` or
`Shift+Up/Down` keys. There are also two special scale controls, located at the right side
of a track. The upper is for vertical scale, and the lower one is for zero offset.
Scale controls display the current value of the corresponding option. Also one can alter
the value with the scrolling. Scrolling speed can be changed with the `Shift` or `Alt` key
(fast or fine scrolling correspondingly). At last, by clicking the scale control one
can enter any value directly. Press `Escape` or click outside to leave focus.

Of course, you can also change the scales with the script.
  ```
  oca_track_getprop( "scale", "signal" )
  oca_group_setprop( struct( "view_position", 0.90, "view_duration", 0.05 ) );
  oca_track_setprop( "scale", 0.1, {"K","signal"} );
  ```
  ```
  oca_group_setprop( struct( "view_position", 2.57, "view_duration", 0.4 ) );
  oca_track_setprop( "scale", 0.1, {1,2} );
  oca_track_setprop( "height", 300, "signal" );
  ```
  ```
  oca_group_setprop( struct( "view_position", -1, "view_duration", 5 ) );
  oca_track_setprop( "scale", 1, {"K","signal"} );
  oca_track_setprop( "height", 100, "signal" );
  ```
Octaudio console provides command history with incremental search. Just press `Ctrl+Up` in
the console. So by pressing `Ctrl+Up` and then "view" you will get the list of the last view
configuration commands. You can also get the list of completions for the word under the
cursor by pressing Tab. Incremental search works for the completion list as well.

Now we can calculate and display the average power of the signal.
  ```
  oca_track_add( "power", 44.1 );
  oca_track_setprop( struct( "abs_value", true, "scale", 0.25 ), "power" );
  oca_data_set( 0, meansq( buffer( oca_data_get( "all", "signal" ), 1000 ) ), "power" );
  ```
The last command uses `buffer` function from the octave signal package. Here we've created
a track with the sample rate of 44.1 Hz, so one point is 1000 samples average. Notice
the effect of the "abs_value" property, it turns on the view mode for positive data.
Zero line is at the bottom of the track now, and scale controls behave appropriately. Notice
also that Octaudio has made this track non-audible. Any track with the sample rate lower then
8000 Hz will be marked as non-audible by default. But you can always set the audible
property directly for any track.

Selecting a data region is easy.
  ```
  oca_group_setprop( "region_start", min( find( K < -0.5 ) ) / 44100 );
  oca_group_setprop( "region_end", max( find( K < -0.5 ) ) / 44100 );
  ```
Of course, you can move region boundaries with a mouse, or set them directly with
`Ctrl+Shift` and `Ctrl+Alt` click (start and end positions respectively). With a region
defined one can access the corresponding data.
  ```
  sqrt( meansq( oca_data_get( "region", "signal" ) ) )
  ```
To clear current region press `Ctrl+Shift+A` keys. One can specify time interval directly
as well.
  ```
  sqrt( meansq( oca_data_get( [0.5,0.1], "signal" ) ) )
  ```
Here we have calculated an RMS for the interval of 0.5-0.6 sec. Time interval format is
`[start_time, duration]`, and all values are in seconds.

It is also possible to display several tracks together within a smart track.
  ```
  oca_track_add( "smart track", {"K","signal"} );
  ```
Smart track displays the data of the original track, not its own data. When the original track
data is being changed, you see this changes in the smart track immediately.
  ```
  oca_data_set( 1.5, zeros(1,3000), "signal" )
  ```
One can add more subtracks to the smart track.
  ```
  oca_subtrack_add( "power", "smart track" );
  ```
And change the subtrack color.
  ```
  oca_subtrack_setprop( "color", [200,200,0], "signal", "smart track" );
  ```
Again, the smart track and its subtracks can also be edited with the **Properties** dialog.
To edit subtrack list, check on the **Edit Track List** checkbox.

If you have many tracks, it's time to make use of track groups.
  ```
  id_new_group = oca_group_add( "New Group" );
  oca_global_setprop( "active_group", id_new_group );
  ```
Finally let's load audio from a wav file.
  ```
  oca_loadwavm( "my_audio_file.wav" )
  ```
Of course, you should enter a path to the existing file here. Autocompletion can help
you here too. Actually, if you press the Tab button when the cursor is inside a string,
the file selection dialog will be opened.



