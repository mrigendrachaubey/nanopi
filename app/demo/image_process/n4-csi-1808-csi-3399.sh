rk_image_process \
    -input="/dev/video0 /dev/video1 /dev/video2 /dev/video3" \
    -input_width 1280 -input_height 720 -input_format 4 \
    -width 2560 -height 1440 -format 6 \
    -disp -drm_conn_type="DSI" -drm_raw8_mode \
    -processor="none"
