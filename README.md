# loud_video_detector

This is a project that will detect if a video contains painfully loud audio or not.
The program can be passed a url or a file path to perform this check, and has various customizable parameters.

### You must have ffmpeg in order to use this program.

ffmpeg may be either in the system path or stored as an executable in the same directory as the program.
This program relies on ffmpeg to fetch the audio data for analysis.

### How does it work?

This program works by detecting 3 different types of loudness through simple audio analysis.
This can be customized to meet the user's needs and preferences. Each may be toggled on or off and detection thresholds modified.

- peak threshold
- average threshold
- sudden spike threshold

Peak threshold will detect the loudest volume in the entire file. This will allow you to detect audio that is clipping or otherwise too loud.

Average threshold will determine the average volume across the entire file. This is useful for determining if a video is too loud in general.
In general, a video should have audio normalized at -6db or -3db for comfortable listening.

Sudden spike threshold will detect large increases in volume.
This will detect audio like jumpscares where the video is normally very quiet and suddenly increases in volume to startle you.
