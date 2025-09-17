# Merge Mouse Touch
Merge mouse input event and touch input event into one merge virtual device.

## Compilation (for Yocto)

### Source Yocto SDK
```shell
$ source /opt/fsl-imx-wayland/5.15-kirkstone/environment-setup-armv8a-poky-linux
```

### Make
```shell
$ $CC merge_mouse_touch.c -o merge_mouse_touch
```

## How to use

1. Run the `merge_mouse_touch` in background
```
$ sudo ./merge_mouse_touch &
[1] 525
[ 1330.926315] input: merged-touch as /devices/virtual/input/input10
Create merge-touch device succeed!
```

2. Run evtest, and select the merge-touch device
```shell
$ evtest
No device specified, trying to scan all of /dev/input/event*
Available devices:
/dev/input/event0:      30370000.snvs:snvs-powerkey
/dev/input/event1:      gpio-keys
/dev/input/event2:      ThinkPad USB Laser Mouse
/dev/input/event3:      eGalaxTouch Virtual Device for Single
/dev/input/event4:      eGalaxTouch Virtual Device for Eraser
/dev/input/event5:      eGalaxTouch Virtual Device for Touch
/dev/input/event6:      eGalaxTouch Virtual Device for Pen
/dev/input/event7:      merged-touch
Select the device event number [0-7]:
```

3. Click the left button of the mouse or press the touch panel
```shell
$ evtest | tee -a log.txt
No device specified, trying to scan all of /dev/input/event*
Available devices:
/dev/input/event0:      30370000.snvs:snvs-powerkey
/dev/input/event1:      gpio-keys
/dev/input/event2:      ThinkPad USB Laser Mouse
/dev/input/event3:      eGalaxTouch Virtual Device for Single
/dev/input/event4:      eGalaxTouch Virtual Device for Eraser
/dev/input/event5:      eGalaxTouch Virtual Device for Touch
/dev/input/event6:      eGalaxTouch Virtual Device for Pen
/dev/input/event7:      merged-touch
Select the device event number [0-7]: 7
Input driver version is 1.0.1
Input device ID: bus 0x3 vendor 0x1234 product 0x5678 version 0x0
Input device name: "merged-touch"
Supported events:
  Event type 0 (EV_SYN)
  Event type 1 (EV_KEY)
    Event code 330 (BTN_TOUCH)
  Event type 3 (EV_ABS)
    Event code 0 (ABS_X)
      Value      0
      Min        0
      Max     1920
    Event code 1 (ABS_Y)
      Value      0
      Min        0
      Max     1080
Properties:
Testing ... (interrupt to exit)
Event: time 1758101523.501303, type 3 (EV_ABS), code 0 (ABS_X), value 940
Event: time 1758101523.501303, type 3 (EV_ABS), code 1 (ABS_Y), value 509
Event: time 1758101523.501303, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 1
Event: time 1758101523.501303, -------------- SYN_REPORT ------------
Event: time 1758101523.621311, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
Event: time 1758101523.621311, -------------- SYN_REPORT ------------

```
