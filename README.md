<img height=230 src="assets/image/CanMV_logo_800x260.png">

<br />

<div class="title_pic">
    <img src="assets/image/CanMV_logo_800x260.svg"  style="margin-right: 10px;" height=45> <img src="assets/image/micropython.png" height=50>
</div>

<br />
<br />

<a href="LICENSE.md">
    <img src="https://img.shields.io/badge/license-Apache%20v2.0-orange.svg?style=for-the-badge" alt="License" />
</a>


<br/>
<br/>

[中文](README_ZH.md)

<br />
<br />

**CanMV, makes AIOT easier!**

This implementation was based on Sipeed MaixPy, but it diverged from it and is now a completely independent project.

CanMV is designed to make AIOT programming easier, based on the [Micropython](http://www.micropython.org) syntax, running on a very powerful embedded AIOT chip [K210](https://kendryte.com).


> K210 brief: 
> * Image Recognition with hardware AI acceleration
> * Dual core with FPU
> * 8MB(6MB+2MB) RAM
> * 16MB external Flash
> * Max 800MHz CPU freq (see the dev board in detail, usually 400MHz)
> * Microphone array(8 mics)
> * Hardware AES SHA256
> * FPIOA (Periphrals can map to any pins)
> * Peripherals: I2C, SPI, I2S, WDT, TIMER, RTC, UART, GPIO etc.



## Simple code

Find I2C devices:

```python
from machine import I2C

i2c = I2C(I2C.I2C0, freq=100000, scl=28, sda=29)
devices = i2c.scan()
print(devices)
```

Take picture:

```python
import sensor
import image
import lcd

lcd.init()
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.run(1)
while True:
    img=sensor.snapshot()
    lcd.display(img)
```

Use AI model to detect face:
```python
import sensor, image, time, lcd
from maix import KPU
import gc

lcd.init()
sensor.reset()                      # Reset and initialize the sensor. It will
                                    # run automatically, call sensor.run(0) to stop
sensor.set_pixformat(sensor.RGB565) # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)   # Set frame size to QVGA (320x240)
sensor.skip_frames(time = 1000)     # Wait for settings take effect.
clock = time.clock()                # Create a clock object to track the FPS.

anchor = (0.1075, 0.126875, 0.126875, 0.175, 0.1465625, 0.2246875, 0.1953125, 0.25375, 0.2440625, 0.351875, 0.341875, 0.4721875, 0.5078125, 0.6696875, 0.8984375, 1.099687, 2.129062, 2.425937)
kpu = KPU()
kpu.load_kmodel("/sd/face_detect_320x240.kmodel")
kpu.init_yolo2(anchor, anchor_num=9, img_w=320, img_h=240, net_w=320 , net_h=240 ,layer_w=10 ,layer_h=8, threshold=0.5, nms_value=0.2, classes=1)

while True:
    clock.tick()                    # Update the FPS clock.
    img = sensor.snapshot()
    kpu.run_with_output(img)
    dect = kpu.regionlayer_yolo2()
    fps = clock.fps()
    if len(dect) > 0:
        for l in dect :
            a = img.draw_rectangle(l[0],l[1],l[2],l[3], color=(0, 255, 0)) # draw face box

    a = img.draw_string(0, 0, "%2.1ffps" %(fps), color=(0, 60, 128), scale=2.0)
    lcd.display(img)
    gc.collect()

kpu.deinit()
```
> please read doc before run it

## Release



## Documentation

Doc refer to [canmv document](https://developer.canaan-creative.com/index.html?channel=developer#/word)

## Examples

[canmv_examples](https://github.com/kendryte/canmv_examples)

## Build From Source

See [build doc](build.md)

## License

See [LICENSE](LICENSE.md) file


## Other: As C SDK for C developers


In `CanMV` project, since `micropython` exists as a component, it can be configured to not participate in compilation, so this repository can also be developed as `C SDK`. For the usage details, see [Building Documentation](build.md), which can be started by compiling and downloading `projects/hello_world`.

The compilation process is briefly as follows:

```
wget https://github.com/kendryte/kendryte-gnu-toolchain/releases/download/v8.2.0-20190409/kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz
sudo tar -Jxvf kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz -C /opt
cd projects/hello_world
python3 project.py menuconfig
python3 project.py build
python3 project.py flash -B auto -b 1500000 -p /dev/ttyUSB0 -t
```


