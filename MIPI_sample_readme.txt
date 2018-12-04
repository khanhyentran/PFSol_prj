
$ ls /tmp/ -la
total 777
drwxrwxrwt    2 root     root           100 Oct 31 09:00 .
drwxr-xr-x    1 root     root            79 Jan  1  1970 ..
-rw-r--r--    1 root     root        768000 Oct 31 09:00 capture.bin
-rw-r--r--    1 root     root         14574 Oct 31 09:00 messages
-rw-------    1 root     root             0 Jan  1  1970 sshd
$ mem r w 0x40200000
40200000: d48e cf91 e192 cb8a d191 ca7f c58f c489 
40200010: c78e d898 ffd1 ffff ffff ffff ffff ffff 
40200020: ffff ffff ffff ffff ffff ffff ffff ffff 
40200030: ffff ffff ffff ffff ffff ffff ffff ffff 
    
$ mem r w 0x40260000
40260000: d593 d984 cb93 bf90 be85 bf96 c685 c289 
40260010: c48a e9a1 ffe2 ffff ffff ffff ffff ffff 
40260020: ffff ffff ffff ffff ffff ffff ffff ffff 
40260030: ffff ffff ffff ffff ffff ffff ffff ffff

ifconfig eth1 192.168.1.100
scp /tmp/capture* rvc@192.168.1.94:/data1/KhanhTran/
../../debug_VIN/bayer2rgb/bayer2rgb --input=/data1/KhanhTran/capture.bin --output=pic.tiff --width=800 --height=480 --bpp=8 --first=RGGB --method=BILINEAR --tiff

convert pic.tiff pic.png
eog pic.png

mem w b fcfe0430 1
mem w b fcfe042c 1

./bayer2rgb --input=/tmp/capture.bin --output=/tmp/capture.bmp --width=800 --height=480 --bpp=8 --first=RGGB --method=BILINEAR --bmp


/* Set the address of our capture buffer */
	retval = R_MIPI_SetBufferAdr(0, map_ram.addr);

	/*Set the capture mode*/
	retval = R_MIPI_SetMode(0); /*0: sigle mode, 1 continuous mode*/
	retval = R_MIPI_Setup();
=> ok from sencond time



