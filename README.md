# False Touch Detector
A tool for detecting false touch events by merging mouse and touch input into one virtual touchscreen device, allowing comparison between expected (mouse) and actual (touch) input to identify phantom or erroneous touch events.

## Compilation (for Yocto)

### Source Yocto SDK
```shell
$ source /opt/fsl-imx-wayland/5.15-kirkstone/environment-setup-armv8a-poky-linux
```

### Make
```shell
$ $CC touch_merger.c -o touch_merger
```

## How to use

### 1. Open a virtual merged touch device
Run `sudo ./touch_merger &` to create a virtual merged touch device in background.
```
$ sudo ./touch_merger &
[1] 674
[ 6654.171584] input: merged-touch as /devices/virtual/input/input20
Mouse : '/dev/input/event2'
Touch : /dev/input/event5
Create merged touch device succeed!
```

After the previous step, if you run the `evtest` command now, you should see a new device named 'merged-touch'.
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

### 2. Event monitoring and logging
Record events to a log file for analysis:
```shell
$ evtest /dev/input/event7 | tee -a log.txt
```

This will monitor the merged device and save all events to `log.txt`. Now you can:
- Click the left mouse button (expected touches)
- Observe the touchscreen for any unexpected touches (ghost/false touches)

**Note:** Coordinates (ABS_X, ABS_Y) only appear when the position changes. If you click at the same location multiple times, you'll only see BTN_TOUCH events without coordinates - this is normal touchscreen behavior.

### 3. Analyze for false touches
After recording events, use the summary script to count total touch events:
```shell
$ bash touch_log_summary.sh
=====================
Total touches: 42
=====================
```

**False Touch Detection:**
- Compare the total touch count with your actual mouse clicks
- If the count is higher than expected, ghost/false touches have occurred
- Review `log.txt` to identify when unexpected touch events happened

```shell
$ evtest /dev/input/event7
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
  Property type 1 (INPUT_PROP_DIRECT)
Testing ... (interrupt to exit)
Event: time 1758174356.687041, type 3 (EV_ABS), code 0 (ABS_X), value 948
Event: time 1758174356.687041, type 3 (EV_ABS), code 1 (ABS_Y), value 540
Event: time 1758174356.687041, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 1
Event: time 1758174356.687041, -------------- SYN_REPORT ------------
Event: time 1758174356.791260, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
Event: time 1758174356.791260, -------------- SYN_REPORT ------------
Event: time 1758174359.766684, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 1
Event: time 1758174359.766684, -------------- SYN_REPORT ------------
Event: time 1758174359.886673, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
```

## Troubleshooting

### Device not found
- Ensure the target devices exist in `/proc/bus/input/devices`
- Check device names match `TARGET_MOUSE` and `TARGET_TOUCH` in the code
- Verify permissions to access `/dev/input/` devices

### Missing coordinates
- **Normal behavior:** Coordinates only appear when position changes
- Move the mouse slightly between clicks to see coordinates
- This matches standard touchscreen behavior

### Permission denied
- Run with `sudo` as the program needs access to `/dev/uinput` and input devices
- Ensure user is in the `input` group (optional alternative to sudo)
