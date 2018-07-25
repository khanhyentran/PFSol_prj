9df1f7fd6d2c2018fd7dfd56a36f7d877b5fc822
https://kernel.googlesource.com/pub/scm/linux/kernel/git/horms/renesas-bsp/+/v4.14/rcar-3.6.1

[    3.296916] rcar-csi2 0.csi2: rcar_csi2_probe_resources 
[    3.304734] rcar-csi2 0.csi2: can't request region for resource [mem 0x00000]
[    3.313468] rcar-csi2 0.csi2: Failed to get resources
[    3.321670] rcar-csi2: probe of 0.csi2 failed with error -16

[    3.331240] rcar-csi2 e8209000.csi2: rcar_csi2_probe
[    3.338813] rcar-csi2 e8209000.csi2: rcar_csi2_probe_resources 
[    3.355006] rcar-csi2 e8209000.csi2: 2 lanes found

[    2.966281] rcar-vin e803f000.video: failed to get cpg reset e803f000.video
[    2.984565] rcar-vin: probe of e803f000.video failed with error -524

$ media-ctl -d /dev/media0 -p
Media controller API version 4.14.43

Media device information
------------------------
driver          Renesas VIN
model           video
serial          
bus info        /video@e803f000
hw revision     0x0
driver version  4.14.43

Device topology
- entity 1: rza_vin e803f000.video (1 pad, 0 link)
            type Node subtype V4L flags 0
            device node name /dev/video0
        pad0: Sink

$ 
$ media-ctl -d /dev/media0 -l "'rza-csi2 e8209000.csi2':1 -> 'rza_vin e803f000.v
ideo':0 [1]"
Unable to parse link: Invalid argument (22)
=======
$ media-ctl -d /dev/media0 -p
Media controller API version 4.14.43

Media device information
------------------------
driver          Renesas VIN
model           video
serial          
bus info        /video@e803f000
hw revision     0x0
driver version  4.14.43

Device topology
- entity 1: rza_vin e803f000.video (1 pad, 0 link)
            type Node subtype V4L flags 0
            device node name /dev/video0
        pad0: Sink

- entity 5: rza_csi2 e8209000.csi2 (5 pads, 0 link)
            type V4L2 subdev subtype Unknown flags 0
        pad0: Sink
        pad1: Source
        pad2: Source
        pad3: Source
        pad4: Source
======
$ media-ctl -d /dev/media0 -p
Media controller API version 4.14.43

Media device information
------------------------
driver          Renesas VIN
model           video
serial          
bus info        /video@e803f000
hw revision     0x0
driver version  4.14.43

Device topology
- entity 1: rza_vin e803f000.video (1 pad, 1 link)
            type Node subtype V4L flags 0
            device node name /dev/video0
        pad0: Sink
                <- "rza_csi2 e8209000.csi2":1 [ENABLED]

- entity 5: rza_csi2 e8209000.csi2 (5 pads, 1 link)
            type V4L2 subdev subtype Unknown flags 0
            device node name /dev/v4l-subdev0
        pad0: Sink
                [fmt:unknown/0x0]
        pad1: Source
                [fmt:unknown/0x0]
                -> "rza_vin e803f000.video":0 [ENABLED]
        pad2: Source
                [fmt:unknown/0x0]
        pad3: Source
                [fmt:unknown/0x0]
        pad4: Source
                [fmt:unknown/0x0]

$ capturev4l2 
Driver Caps:
  Driver: "rza_vin"
  Card: "R_Car_VIN"
  Bus: "platform:e803f000.video"
  Version: 1.0
  Capabilities: 85200001
Camera Cropping:
  Bounds: 0x0+0+0
  D[   66.691657] rza-vin e803f000.video: Format 0x47504a4d not found, using de9
[   66.703244] rza-vin e803f000.video: Format 640x480 bpl: 1280 size: 614400

  Aspect: 1/1
  FMT : CE Desc
--------------------
  NV12:    Y/CbCr 4:2:0
  NV16:    Y/CbCr 4:2:2
  YUYV:    YUYV 4:2:2
  UYVY:    UYVY 4:2:2
  RGBP:    16-bit RGB 5-6-5
  AR15:    16-bit ARGB 1-5-5-5
  AR24:    32-bit BGRA 8-8-8-8
  XR24:    32-bit BGRX 8-8-8-8
Selected Camera Mode:
  Width: 640
  Height: 480
  PixFmt: YUYV
  Field: 1
Length: 614400
Address: 0xb6ddb000
Image Length: 0
Start Capture: Broken pipe
[   66.803053] rza-vin e803f000.video: MSTP status timeout
$ media-ctl -d /dev/media0 -l "'[  124.499495] random: crng init done
^C
$ media-ctl -d /dev/media0 -l "'rza_csi2 e8209000.csi2':1 -> 'rza_vin e803f000.v
ideo':0 [1] "
$ ==========
$ 
$ yavta -f NV12 -s 640x480 -c10 --skip 7 -F /dev/video0 
Device /dev/video0 opened.
Device `R_Car_VIN' on `platform:e803f000.video' is a video capture device.
Video format set: NV12 (3231564e) 640x480 (stride 640) buffer size 460800
Video format: NV12 (3231564e) 640x480 (stride 640) buffer size 460800
[  887.838111] yavta: page allocation failure: order:7, mode:0x14000c0(GFP_KERN)
[  887.850269] CPU: 0 PID: 593 Comm: yavta Not tainted 4.14.43-g033f5ff-dirty #9
[  887.858266] Hardware name: Generic R7S9210 (Flattened Device Tree)
[  887.866776] [<bf808eb5>] (unwind_backtrace) from [<bf80779b>] (show_stack+0x)
[  887.875434] [<bf80779b>] (show_stack) from [<bf843a1b>] (warn_alloc+0x61/0xf)
[  887.882957] [<bf843a1b>] (warn_alloc) from [<bf84408f>] (__alloc_pages_nodem)
[  887.892162] [<bf84408f>] (__alloc_pages_nodemask) from [<bf809fbf>] (__dma_a)
[  887.902568] [<bf809fbf>] (__dma_alloc_buffer.constprop.5) from [<bf80a10d>] )
[  887.912711] [<bf80a10d>] (remap_allocator_alloc) from [<bf809b57>] (__dma_al)
[  887.921452] [<bf809b57>] (__dma_alloc) from [<bf809c3d>] (arm_dma_alloc+0x2d)
[  887.929606] [<bf809c3d>] (arm_dma_alloc) from [<bf9d4b45>] (vb2_dc_alloc+0x6)
[  887.937629] [<bf9d4b45>] (vb2_dc_alloc) from [<bf9d0e6b>] (__vb2_queue_alloc)
[  887.946099] [<bf9d0e6b>] (__vb2_queue_alloc) from [<bf9d2223>] (vb2_core_req)
[  887.954932] [<bf9d2223>] (vb2_core_reqbufs) from [<bf9d41cf>] (vb2_ioctl_req)
[  887.963688] [<bf9d41cf>] (vb2_ioctl_reqbufs) from [<bf9c93c1>] (__video_do_i)
[  887.972692] [<bf9c93c1>] (__video_do_ioctl) from [<bf9c96fb>] (video_usercop)
[  887.981972] [<bf9c96fb>] (video_usercopy) from [<bf9c4c33>] (v4l2_ioctl+0x57)
[  887.990289] [<bf9c4c33>] (v4l2_ioctl) from [<bf872aed>] (vfs_ioctl+0x11/0x1c)
[  887.997781] [<bf872aed>] (vfs_ioctl) from [<bf872f3b>] (do_vfs_ioctl+0x61/0x)
[  888.005377] [<bf872f3b>] (do_vfs_ioctl) from [<bf873507>] (SyS_ioctl+0x23/0x)
[  888.012982] [<bf873507>] (SyS_ioctl) from [<bf804e81>] (ret_fast_syscall+0x1)
[  888.020954] Mem-Info:
[  888.023491] active_anon:94 inactive_anon:3 isolated_anon:0
[  888.023491]  active_file:1 inactive_file:1 isolated_file:0
[  888.023491]  unevictable:0 dirty:0 writeback:0 unstable:0
[  888.023491]  slab_reclaimable:30 slab_unreclaimable:325
[  888.023491]  mapped:1 shmem:11 pagetables:16 bounce:0
[  888.023491]  free:566 free_pcp:0 free_cma:0
[  888.054848] Node 0 active_anon:376kB inactive_anon:12kB active_file:4kB inaco
[  888.076812] Normal free:2264kB min:344kB low:428kB high:512kB active_anon:37B
[  888.102994] lowmem_reserve[]: 0 0 0
[  888.106854] Normal: 12*4kB (U) 31*8kB (U) 19*16kB (U) 12*32kB (U) 4*64kB (U)B
[  888.120336] 13 total pagecache pages
[  888.124177] 0 pages in swap cache
[  888.127568] Swap cache stats: add 0, delete 0, find 0/0
[  888.132848] Free swap  = 0kB
[  888.135872] Total swap = 0kB
[  888.138787] 2048 pages RAM
[  888.141547] 0 pages HighMem/MovableOnly
[  888.145465] 171 pages reserved
[  888.149283] rcar-vin e803f000.video: dma_alloc_coherent of size 462848 failed
5 buffers requested.
length: 460800 offset: 0
Buffer 0 mapped at address 0xb6d94000.
length: 460800 offset: 462848
Buffer 1 mapped at address 0xb6d23000.
length: 460800 offset: 925696
Buffer 2 mapped at address 0xb6cb2000.
length: 460800 offset: 1388544
Buffer 3 mapped at address 0xb6c41000.
length: 460800 offset: 1851392
Buffer 4 mapped at address 0xb6bd0000.
Unable to start streaming: Broken pipe (32).
5 buff[  888.283552] rcar-vin e803f000.video: MSTP status timeout
ers released.
$ 
https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/drivers/media/platform/renesas-ceu.c?id=32e5a70dc8f4e9813c61e5465d72d2e9830ba0ff
https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/Documentation/devicetree/bindings/media/renesas,ceu.txt?id=a444e5184f329738691b06ed31addaa0edb6aa01

/home/rvc/RZA_prj/02_uboot_drivermodel/rza_linux-4.9_bsp/output/buildroot-2017.02/output/host/usr/bin/arm-linux-gnueabihf-gcc -static -O2 --sysroot=/home/rvc/RZA_prj/02_uboot_drivermodel/rza_linux-4.9_bsp/output/buildroot-2017.02/output/host/usr/arm-buildroot-linux-gnueabihf/sysroot -o ceu_omni ./ceu_omni.c ceu.c ov7740.c ov7670.c
cp ceu_omni ../output/buildroot-2017.02/output/rootfs_overlay/root/
 cd ../output/buildroot-2017.02
 
make
cp output/images/rootfs.squashfs /media/sf_Store/RZA/01_SDIO/Binary/rootfs.squashfs.bin

 ./ceu_omni -v -c -n
 
 ====
 Error when load file by Jlink after run DRP
Cortex-A9 identified.
Halting CPU for downloading file.
Downloading file [/tmp/rootfs.bin]...

****** Error: Timeout while checking target RAM, core does not stop. (PC = 0x200041CC, CPSR = 0x00000197, LR = 0x81226450)!
Failed to prepare for programming.
Failed to execute RAMCode for RAM check!
Error while determining flash info (Bank @ 0x20000000)
Unspecified error -1
=======
	if (ret)
		goto error_v4l2_unregister;

	ceudev->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(ceudev->clk)) {
		dev_err(&pdev->dev, "failed to get clock%s\n",
			dev_name(ceudev->dev));
		ret = PTR_ERR(ceudev->clk);
		goto error_v4l2_unregister;
	}
	
	ret = clk_prepare_enable(ceudev->clk);
	if (ret < 0)
		dev_info(dev, "failed to enable clk\n");
	dev_info(dev, "Renesas Capture Engine Unit %s\n", dev_name(dev));

	return 0;
======
Welcome to Buildroot
buildroot login: root
yavta /dev/video0 -c1 -n1 -f NV12 -s 640x480 -F"/tmp/NV12#.bin"$ yavta /dev/video0 -c1 -n1 -f NV12 -s 640x480 -F"/tmp/NV12#.bin"
Device /dev/video0 opened.
Device `Renesas CEU' on `platform:renesas-ceu-e8210000.c' (driver 'renesas-ceu') is a video capture (with mplanes) device.
Video format set: NV12 (3231564e) 640x480 field none, 2 planes: 
===
https://svn.dd-wrt.com/browser/src/linux/universal/linux-4.14/drivers/staging/media/atomisp/i2c/imx?rev=34113
