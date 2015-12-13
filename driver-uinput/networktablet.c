
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
    abort(); \
    exit(EXIT_FAILURE); \
}


int udp_socket;


void init_abs(int fd)
{
    struct uinput_user_dev uidev;

    // enable synchronization
    if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
        die("error: ioctl UI_SET_EVBIT EV_SYN");

    // enable 1 button
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        die("error: ioctl UI_SET_EVBIT EV_KEY");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) < 0)
        die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_PEN) < 0)
        die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS) < 0)
        die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS2) < 0)
        die("error: ioctl UI_SET_KEYBIT");

    // enable 2 main axes + pressure (absolute positioning)
    if (ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0)
        die("error: ioctl UI_SET_EVBIT EV_ABS");
    if (ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0)
        die("error: ioctl UI_SETEVBIT ABS_X");
    if (ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0)
        die("error: ioctl UI_SETEVBIT ABS_Y");
    if (ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE) < 0)
        die("error: ioctl UI_SETEVBIT ABS_PRESSURE");

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
void init_rel(int fd)
{
    struct uinput_user_dev uidev;

    // enable synchronization
    if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
        die("error: ioctl UI_SET_EVBIT EV_SYN");

    // enable 1 button
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        die("error: ioctl UI_SET_EVBIT EV_KEY");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0)
        die("error: ioctl UI_SET_KEYBIT");

    if (ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0)
        die("error: ioctl UI_SET_EVBIT EV_ABS");
    if (ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
        die("error: ioctl UI_SET_EVBIT EV_REL");
    if (ioctl(fd, UI_SET_RELBIT, REL_X) < 0)
        die("error: ioctl UI_SETEVBIT REL_X");
    if (ioctl(fd, UI_SET_RELBIT, REL_Y) < 0)
        die("error: ioctl UI_SETEVBIT REL_Y");
    if (ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE) < 0)
        die("error: ioctl UI_SETEVBIT ABS_PRESSURE");

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Network Trackpad");
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x2;
    uidev.id.version = 1;
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
    struct input_event ev;
    ev.type = type;
    ev.code = code;
    ev.value = value;
    if (write(device, &ev, sizeof(ev)) < 0)
        die("error: write()");
}

void send_abs_motion(int device, int x, int y, int pressure)
{
    printf("ABS x %4d;  y %4d;  p %4d\n", x, y, pressure);
    struct input_event events[] = {
        {.type = EV_ABS, .code = ABS_X, .value = x},
        {.type = EV_ABS, .code = ABS_Y, .value = y},
        {.type = EV_ABS, .code = ABS_PRESSURE, .value = pressure},
    };
    write(device, &events, sizeof(events));
}

void send_rel_motion(int device, int x, int y, int pressure)
{
    printf("REL x %4d;  y %4d;  p %4d\n", x, y, pressure);
    struct input_event events[] = {
        {.type = EV_REL, .code = REL_X, .value = x},
        {.type = EV_REL, .code = REL_Y, .value = y},
        //{.type = EV_ABS, .code = ABS_PRESSURE, .value = pressure},
    };
    write(device, &events, sizeof(events));
}

void send_syn(int device)
{
    struct input_event syn = {
        .type = EV_SYN, .code = SYN_REPORT
    };
    write(device, &syn, sizeof(syn));
}

void quit(int signal) {
    close(udp_socket);
}


int main(void)
{
    int device_abs, device_rel;
    struct event_packet ev_pkt;
    int button;
    short sx, sy;

    if ((device_abs = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
        die("error: open");
    if ((device_rel = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
        die("error: open");

    init_rel(device_rel);
    init_abs(device_abs);

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

        switch (ev_pkt.type) {
            case EVENT_TYPE_MOTION:
                send_abs_motion(device_abs, ev_pkt.x, ev_pkt.y, ev_pkt.pressure);
                break;
            case EVENT_TYPE_BUTTON:
                send_abs_motion(device_abs, ev_pkt.x, ev_pkt.y, ev_pkt.pressure);
                switch(ev_pkt.button) {
                    case -1: button = BTN_TOOL_PEN; break;
                    case  0: button = BTN_TOUCH; break;
                    case  1: button = BTN_STYLUS; break;
                    case  2: button = BTN_STYLUS2; break;
                }
                send_event(device_abs, EV_KEY, button, ev_pkt.down);
                send_syn(device_abs);
                printf("sent button: %hhi, %hhu\n", ev_pkt.button, ev_pkt.down);
                break;
            case EVENT_TYPE_RELMOTION:
                sx = ev_pkt.x;
                sy = ev_pkt.y;
                send_rel_motion(device_rel, sx, sy, ev_pkt.pressure);
                send_syn(device_rel);
                break;
            case EVENT_TYPE_RELBUTTON:
                sx = ev_pkt.x;
                sy = ev_pkt.y;
                send_rel_motion(device_rel, sx, sy, ev_pkt.pressure);
                printf("REL down %d\n", ev_pkt.down);
                send_event(device_rel, EV_KEY, BTN_LEFT, ev_pkt.down);
                send_syn(device_rel);
                break;
        }
    }
    close(udp_socket);

    printf("Removing network tablet from device list\n");
    ioctl(device_abs, UI_DEV_DESTROY);
    close(device_abs);
    ioctl(device_rel, UI_DEV_DESTROY);
    close(device_rel);

    printf("GfxTablet driver shut down gracefully\n");
    return 0;
}
