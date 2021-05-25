while true
do
	io -4 -l 0x10 0xff9105c0
	io -4 -l 0x10 0xff910630
	usleep 10000
done
