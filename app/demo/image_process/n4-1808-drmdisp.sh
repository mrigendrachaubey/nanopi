rk_image_process -input="/dev/video0 /dev/video1 /dev/video2 /dev/video3" \
    -width=1280 -height=720 \
    -input_width=1920 -input_height=1080 -input_format 1 \
    -disp -disp_rotate=90 -processor="none"
