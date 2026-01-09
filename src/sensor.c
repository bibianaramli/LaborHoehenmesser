#include "sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

int configure_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) return -1;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cc[VTIME] = 5;
    tty.c_cc[VMIN] = 0;
    tcflush(fd, TCIFLUSH);
    return tcsetattr(fd, TCSANOW, &tty);
}

int connect_to_sensor(char *tty_path) {
#if SIM
    int fd = open(tty_path, O_RDWR | O_NOCTTY);
    if (fd < 0 || configure_serial(fd) != 0) return -1;
    return fd;
#else
    return 0; // Fake file descriptor for simulation
#endif
}

float get_sensor_value(int serial_fd) {
#if SIM
    char buf[64];
    ssize_t n = read(serial_fd, buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = '\0';
        return strtof(buf, NULL);
    }
    return -1.0f;
#else
    // Simulated values as requested [cite: 231]
    sleep(1); 
    return (float)(10 + (rand() % 90)); 
#endif
}