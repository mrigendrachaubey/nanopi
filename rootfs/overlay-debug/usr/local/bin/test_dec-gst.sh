#!/bin/sh

export DISPLAY=:0.0 
#export GST_DEBUG=*:5
#export GST_DEBUG_FILE=/tmp/2.txt

case "$1" in
	rk3036) ###TODO:Display bug, since rk3036 just two overlay.
	sudo service lightdm stop
	sleep 2
	su linaro -c "gst-launch-1.0 uridecodebin uri=file:///usr/local/test.mp4 ! kmssink plane-id=52"
	sudo service lightdm start
	;;
*)
	su linaro -c "gst-launch-1.0 uridecodebin uri=file:///usr/local/test.mp4  ! rkximagesink"
	;;
esac
shift
