input=$1
input_format=1
v4l2_memory_type="mmap"
if [ "$input" == "camera" ]; then

for dev in `ls /dev/video*`
do
    v4l2-ctl -d $dev -D | grep rkisp1_mainpath
    if [ $? -eq 0 -a -z "$rkisp_main" ]; then rkisp_main=$dev; fi
    v4l2-ctl -d $dev -D | grep uvcvideo
    if [ $? -eq 0 -a -z "$uvcvideo" ]; then uvcvideo=$dev; fi
done
# uvc first, then rkisp
echo uvcvideo=$uvcvideo,rkisp_main=$rkisp_main

if test $uvcvideo; then
    input=$uvcvideo
    input_format=5
elif test $rkisp_main; then
    input=$rkisp_main
    input_format=0
    v4l2_memory_type="dma"
else
    echo find non valid camera via v4l2-ctl
fi

fi #if [ "$input" == "camera" ]; then

npu_model_name=$2

input_width=1920
input_height=1080
if [ $# -ge 4 ]; then
if [ $3 ]; then input_width=$3; fi
if [ $4 ]; then input_height=$4; fi
fi

npu_piece_width=300
npu_piece_height=300
if [ $# -ge 6 ]; then
if [ $5 ]; then npu_piece_width=$5; fi
if [ $6 ]; then npu_piece_height=$6; fi
fi

if [ $# -eq 5 -o $# -eq 7 ]; then
for extra_param in $@; do true; done
fi

cd /userdata/ && \
rk_image_process -input=$input \
    -input_width $input_width -input_height $input_height -input_format $input_format -v4l2_memory_type $v4l2_memory_type \
    -width 1280 -height 720 -format 3 -slice_num 1 \
    -disp -disp_rotate=270 \
    -processor="npu" -npu_data_source="none" -npu_model_name=$npu_model_name \
    -npu_piece_width $npu_piece_width -npu_piece_height $npu_piece_height \
    $extra_param
