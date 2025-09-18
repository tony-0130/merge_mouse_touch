#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <sys/select.h>
#include <sys/time.h>

#define SCREEN_X    1920
#define SCREEN_Y    1080

// The device name (correspond to the "Name=" in the /proc/bus/input/devices)
#define TARGET_MOUSE    "Mouse"
#define TARGET_TOUCH    "eGalaxTouch Virtual Device for Touch"

// Search for an input device in /proc/bus/input/devices by keyword
// Returns the full path (/dev/input/eventX) if found
int find_device_event(const char *keyword, char *event_path, size_t len) {
    FILE *f = fopen("/proc/bus/input/devices", "r");
    if (!f) {
        perror("fopen /proc/bus/input/devices");
        return -1;
    }

    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "Name=") && strstr(line, keyword)) {
            found = 1; // Found device name containing the keyword
        } else if (found && strstr(line, "Handlers=")) {
            char *p = strstr(line, "event");
            if (p) {
                char event_name[32];
                sscanf(p, "%31s", event_name); // Just get the "eventX", ignore the space behind
                snprintf(event_path, len, "/dev/input/%s", event_name);  
                fclose(f);
                return 0;
            }

            found = 0; // haven't found event, reset
        }
    }

    fclose(f);
    return -1; // Not found
}

// Create a virtual uinput device that acts like a touchscreen
int setup_uinput() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open /dev/uinput");
        exit(1);
    }

    // Enable basic event
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);

#if defined(UI_ABS_SETUP)   // kernel version >= 5.6, support the UI_ABS_SETUP
    struct uinput_abs_setup abs_setup;
    memset(&abs_setup, 0, sizeof(abs_setup));

    abs_setup.code - ABS_X;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_X;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);

    abs_setup.code = ABS_Y;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "merged-touch");

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
        perror("UI_DEV_SETUP");
        exit(1);
    }
#else   // kernel version < 5.6, using the struct uinput_user_dev
    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "merged-touch");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0x5678;
    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = SCREEN_X;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = SCREEN_Y;

    if (write(fd, &uidev, sizeof(uidev)) < 0) {
        perror("write uinput_user_dev");
        exit(1);
    }
#endif

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("UI_DEV_CREATE");
        exit(1);
    }

    return fd;
}

// Emit a single input_event into uinput device
void emit(int fd, int type, int code, int val) {
    struct input_event ie;
    memset(&ie, 0, sizeof(ie));

    ie.type = type;
    ie.code = code;
    ie.value = val;
    gettimeofday(&ie.time, NULL);
    if (write(fd, &ie, sizeof(ie)) < 0) {
        perror("write event");
    }
}

int main() {
    char mouse_event[128], touch_event[128];

    // Find mouse device (using TARGET_TOUCH)
    if (find_device_event(TARGET_MOUSE, mouse_event, sizeof(mouse_event)) < 0) {
        fprintf(stderr, "Can't find the mouse device : %s\n", TARGET_MOUSE);
        return 1;
    }
    // Find touchscreen device (using TARGET_TOUCH)
    if (find_device_event(TARGET_TOUCH, touch_event, sizeof(touch_event)) < 0) {
        fprintf(stderr, "Can't find the touch device : %s\n", TARGET_TOUCH);
        return 1;
    }

    printf("Mouse : '%s'\n", mouse_event);
    printf("Touch : %s\n", touch_event);

    int fd_mouse = open(mouse_event, O_RDONLY);
    int fd_touch = open(touch_event, O_RDONLY);

    if (fd_mouse < 0 || fd_touch < 0) {
        perror("open input device");
        return 1;
    }

    int fd_uinput = setup_uinput();
    printf("Create merge-touch device succeed!\n");

    struct input_event ev;
    fd_set fds;
    int maxfd = (fd_mouse > fd_touch ? fd_mouse : fd_touch) + 1;
    int x = SCREEN_X / 2;
    int y = SCREEN_Y / 2;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd_mouse, &fds);
        FD_SET(fd_touch, &fds);

        if (select(maxfd, &fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // Mouse events -> convert into touch events
        if (FD_ISSET(fd_mouse, &fds)) {
            if (read(fd_mouse, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_REL) {
                    if (ev.code == REL_X) {
                        x += ev.value;
                        if (x < 0) x = 0;
                        if (x > SCREEN_X) x = SCREEN_X;
                    }
                    if (ev.code == REL_Y) {
                        y += ev.value;
                        if (y < 0) y = 0;
                        if (y > SCREEN_Y) y = SCREEN_Y;
                    }
                }

                if (ev.type == EV_KEY && ev.code == BTN_LEFT) {
                    if (ev.value == 1) { // Press
                        emit(fd_uinput, EV_ABS, ABS_X, x);
                        emit(fd_uinput, EV_ABS, ABS_Y, y);
                        emit(fd_uinput, EV_KEY, BTN_TOUCH, 1);
                        emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                    } else if (ev.value == 0) { // Release
                        emit(fd_uinput, EV_KEY, BTN_TOUCH, 0);
                        emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
                    }
                }
            }
        }

        // Touch events -> Forward events directly
        if (FD_ISSET(fd_touch, &fds)) {
            if (read(fd_touch, &ev, sizeof(ev)) == sizeof(ev)) {
                emit(fd_uinput, ev.type, ev.code, ev.value);
                if (ev.type == EV_SYN)
                    emit(fd_uinput, EV_SYN, SYN_REPORT, 0);
            }
        }
    }

    ioctl(fd_uinput, UI_DEV_DESTROY);
    close(fd_mouse);
    close(fd_touch);
    close(fd_uinput);
    return 0;
}
