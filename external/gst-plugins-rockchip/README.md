
## About

This is a set of GStreamer plugins about camera for rockchip platform.<br>
Most of them were modified based on upstream existing plugin.

This software has been tested only with kernel after 4.4.
This plugin is based on rkisp1, and the 3A-library since then is camera_engine_rkisp.

## Status

| Elements       | Type  |  Comments  | Origin |
| :----:  | :----:  | :----:  | :----:  |
| rkv4l2src        |    Device Sources  |  rockchip isp camera source  | [v4l2src](https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good/html/gst-plugins-good-plugins-v4l2src.html) |

## Usage

### rkv4l2src
gst-launch-1.0 rkv4l2src device=/dev/video0 ! video/x-raw,format=NV12,width=640,height=480, framerate=30/1 ! videoconvert ! autovideosink

Most of the properties are the same as that of v4l2src, below are rockchip extend properties:
* `xml-path` : tuning xml IQ-file, needed by 3A : (default : "/etc/cam_iq.xml")
* `isp-mode` : "0A" to disable 3A, "2A" to enable AWB/AE, ~~"3A" to enable AWB/AE/AF~~ : (default : "false")

> This feature is only used to make debug conveniently.  
> rkv4l2src plugin is not designed as a CamHal. It's more like `v4l2-ctl`, just a simple capture program.  
> Since the use cases are divers, please handle `media-controller` and `pad format/selection` in APP level.
