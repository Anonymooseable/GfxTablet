
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <arpa/inet.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdint.h>
#include "protocol.h"

#define die(str, args...) { \
    perror(str); \
    exit(EXIT_FAILURE); \
}


int udp_socket;


void init_device(int fd)
{
    struct uinput_user_dev uidev;

    // enable synchronization
    if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
        die("error: ioctl UI_SET_EVBIT EV_SYN");

    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        die("error: ioctl UI_SET_EVBIT EV_KEY");
    /*
    if (ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) < 0)
        die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_PEN) < 0)
        die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS) < 0)
        die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS2) < 0)
        die("error: ioctl UI_SET_KEYBIT");*/
    if (ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0) {
        die("error: ioctl UI_SET_KEYBIT");
    }

    /*
    // enable 2 main axes + pressure (absolute positioning)
    if (ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0)
        die("error: ioctl UI_SET_EVBIT EV_ABS");
    if (ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0)
        die("error: ioctl UI_SETEVBIT ABS_X");
    if (ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0)
        die("error: ioctl UI_SETEVBIT ABS_Y");
    if (ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE) < 0)
        die("error: ioctl UI_SETEVBIT ABS_PRESSURE");
        */

    if (ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
        die("error: ioctl UI_SET_EVBIT EV_REL");
    if (ioctl(fd, UI_SET_RELBIT, REL_X) < 0)
        die("error: ioctl UI_SETEVBIT REL_X");
    if (ioctl(fd, UI_SET_RELBIT, REL_Y) < 0)
        die("error: ioctl UI_SETEVBIT REL_Y");

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Network Tablet");
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;
    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = UINT16_MAX;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = UINT16_MAX;
    uidev.absmin[ABS_PRESSURE] = 0;
    uidev.absmax[ABS_PRESSURE] = INT16_MAX;
    if (write(fd, &uidev, sizeof(uidev)) < 0)
        die("error: write");

    if (ioctl(fd, UI_DEV_CREATE) < 0)
        die("error: ioctl");
}

int prepare_socket()
{
    int s;
    struct sockaddr_in addr;

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("error: prepare_socket()");

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(GFXTABLET_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("error: prepare_socket()");

    return s;
}

void send_event(int device, int type, int code, int value)
{
    struct input_event ev = {
        .type = type,
        .code = code,
        .value = value
    };
    if (write(device, &ev, sizeof(ev)) < 0)
        die("error: write()");
}

void quit(int signal) {
    close(udp_socket);
}


int main(void)
{
    int device;
    int vx = 0, vy = 0, vz = 0;
    short ax, ay, az;
    struct event_packet ev_pkt;

    if ((device = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
        die("error: open");

    init_device(device);
    udp_socket = prepare_socket();

    printf("GfxTablet driver (protocol version %u) is ready and listening on 0.0.0.0:%u (UDP)\n"
        "Hint: Make sure that this port is not blocked by your firewall.\n", PROTOCOL_VERSION, GFXTABLET_PORT);

    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    while (recv(udp_socket, &ev_pkt, sizeof(ev_pkt), 0) >= 9) {        // every packet has at least 9 bytes

        if (memcmp(ev_pkt.signature, "GfxTablet", 9) != 0) {
            fprintf(stderr, "\nGot unknown packet on port %i, ignoring\n", GFXTABLET_PORT);
            continue;
        }
        ev_pkt.version = ntohs(ev_pkt.version);
        if (ev_pkt.version != PROTOCOL_VERSION) {
            fprintf(stderr, "\nGfxTablet app speaks protocol version %i but driver speaks version %i, please update\n",
                ev_pkt.version, PROTOCOL_VERSION);
            break;
        }

        ev_pkt.x = ntohs(ev_pkt.x);
        ev_pkt.y = ntohs(ev_pkt.y);
        ev_pkt.pressure = ntohs(ev_pkt.pressure);

        ax = ev_pkt.x;
        ay = ev_pkt.y;
#define SHIFT 10
        ax >>= SHIFT;
        ay >>= SHIFT;
#define THRESH 4
        ax = (abs(ax) > THRESH) ? ax : 0;
        ay = (abs(ay) > THRESH) ? ay : 0;

        vx += ax;
        vy += ay;
#define decrease(x) x += (x<0)?1:0 //(x>0)?-3:0
        decrease(vx);
        decrease(vy);
        printf("vx %5d; vy %5d; ax %5d; ay %5d\n", vx, vy, ax, ay);
        switch (ev_pkt.type) {
            /*
            case EVENT_TYPE_BUTTON:
                printf("x: %hu, y: %hu, pressure: %hu\n", ev_pkt.x, ev_pkt.y, ev_pkt.pressure);
                send_event(device, EV_ABS, ABS_X, ev_pkt.x);
                send_event(device, EV_ABS, ABS_Y, ev_pkt.y);
                send_event(device, EV_ABS, ABS_PRESSURE, ev_pkt.pressure);
                send_event(device, EV_SYN, SYN_REPORT, 1);
                // stylus hovering
                if (ev_pkt.button == -1)
                    send_event(device, EV_KEY, BTN_TOOL_PEN, ev_pkt.down);
                // stylus touching
                if (ev_pkt.button == 0)
                    send_event(device, EV_KEY, BTN_TOUCH, ev_pkt.down);
                // button 1
                if (ev_pkt.button == 1)
                    send_event(device, EV_KEY, BTN_STYLUS, ev_pkt.down);
                // button 2
                if (ev_pkt.button == 2)
                    send_event(device, EV_KEY, BTN_STYLUS2, ev_pkt.down);
                printf("sent button: %hhi, %hhu\n", ev_pkt.button, ev_pkt.down);
                send_event(device, EV_SYN, SYN_REPORT, 1);
                break;
            case EVENT_TYPE_MOTION:
                printf("x: %hu, y: %hu, pressure: %hu\n", ev_pkt.x, ev_pkt.y, ev_pkt.pressure);
                send_event(device, EV_ABS, ABS_X, ev_pkt.x);
                send_event(device, EV_ABS, ABS_Y, ev_pkt.y);
                send_event(device, EV_ABS, ABS_PRESSURE, ev_pkt.pressure);
                send_event(device, EV_SYN, SYN_REPORT, 1);
                break;
                */
            case EVENT_TYPE_MOTION3D:
                send_event(device, EV_REL, REL_X, vx);
                send_event(device, EV_REL, REL_Y, vy);
                send_event(device, EV_SYN, SYN_REPORT, 1);
                break;
            default:
                //printf("Unknown event type: %d\n", ev_pkt.type);
                break;
        }
    }
    close(udp_socket);

    printf("Removing network tablet from device list\n");
    ioctl(device, UI_DEV_DESTROY);
    close(device);

    printf("GfxTablet driver shut down gracefully\n");
    return 0;
}
