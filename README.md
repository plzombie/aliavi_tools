# aliavi_tools

A project to study ALIAVI **.MTV** format. This is not the format described in [this](https://en.wikipedia.org/wiki/AMV_video_format) article but similar.

The format consists of frames with 48-bytes header. Each header starts with `ALIAVI` signature. The body of frame consists of audio data and video data.

Audio data is a WAV file splitted for each frame (the first frame contains RIFF WAV header. You can just concatenate frames and get working WAV file). Codec is IMA IDPCM, Codec Id 11.

Video frame is motion jpeg. The first frame is a standalone JPEG file. Other frames are JPEG files with just data contained. You need to extract sections from the first jpeg frame and place them to others.

## aliavi_info

This utility helps you to extract data from **.MTV** format. Usage:

```
mkdir images
aliavi_info.exe video.mtv video.wav images
```
