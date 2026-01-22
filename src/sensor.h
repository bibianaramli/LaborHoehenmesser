#ifndef SENSOR_H
#define SENSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <sys/ioctl.h>
#include <time.h>

#define MAXCHAR 1024
#define READ_CHUNK 64
#define SIM 1  // 0 für Simulation aus, 1 für Simulation an

// Betriebssystem-abhängige Flags
#ifdef __APPLE__
#define OPEN_FLAGS O_NONBLOCK
#else
#define OPEN_FLAGS O_RDWR
#endif

// Struktur für Simulationsdaten
typedef struct {
    size_t length;
    size_t index;
    float data[32];
} sim_data;

// Funktionsprototypen
int configure_serial(int fd);
int connect_to_sensor(char *tty_path);
float convert_to_sensor_val(const char *line);
int compare(const void* a, const void* b);
ssize_t read_sim(int serial_fd, char* chunk, size_t chunk_len);

// Datenbank-Funktionen
int open_database(sqlite3 **db, const char *db_name);
int execute_sql(sqlite3 *db, const char *sql);

#endif