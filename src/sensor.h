#ifndef SENSOR_H
#define SENSOR_H

#include <unistd.h>

#define SIM 0 // Set to 1 for real sensor, 0 for simulation

int connect_to_sensor(char *tty_path);
float get_sensor_value(int serial_fd);

#endif