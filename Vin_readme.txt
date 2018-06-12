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
