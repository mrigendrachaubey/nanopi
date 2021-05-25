This Folder contains packages form comunity people.  

> NOTE: Rockchip don't have much knowledge about those packages and we can't give you offical support.(Like BUG-Fix, Add New Features, Explain how it works......) We just provide those pakcages for testing purpose, do not ask us "How-to".

# Usage

### FFmpeg
More infos see https://github.com/rockchip-linux/meta-rockchip/tree/master/recipes-multimedia/ffmpeg

### MPV
You need setup a minimal-gbm env before run mpv, because it can not work with X11.
```
wget https://github.com/rockchip-linux/libmali/raw/rockchip/lib/arm-linux-gnueabihf/libmali-midgard-t76x-r14p0-r0p0-gbm.so
mv /usr/lib/arm-linux-gnueabihf/libMali.so libMali.so.backup
mv libmali*.so /usr/lib/arm-linux-gnueabihf/libMali.so
sudo service lightdm stop
```

More infos see https://github.com/rockchip-linux/meta-rockchip/tree/master/recipes-multimedia/mplayer

### KODI
This kodi package is just a compromise version made for X11/Debian, it can not support 4K videos render.  
Kodi in libreelec is better for serious use. It use GBM+KMS, like MPV.
https://github.com/LibreELEC/LibreELEC.tv

More infos see https://github.com/rockchip-linux/meta-rockchip/tree/master/recipes-multimedia/kodi
