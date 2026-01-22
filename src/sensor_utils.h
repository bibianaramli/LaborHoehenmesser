#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <errno.h>

#define MAXCHAR 1024
#define READ_CHUNK 64
#define MAX_VALUES 512

// Struktur für die Simulation
typedef struct {
    size_t length;
    size_t index;
    float data[32];
} sim_data;

// Funktionsprototypen
int configure_serial(int fd);
int connect_to_sensor(char *tty_path);
float convert_to_sensor_val(const char *line);
ssize_t read_sim(char* chunk, size_t chunk_len);
int open_database(sqlite3 **db, const char *db_name);
int execute_sql(sqlite3 *db, const char *sql);
void print_histogram_horizontal(float values[], int count, float max_val);
float calibrate_sensor(int fd, int is_sim);
void save_histogram_to_csv(const char* filename, float values[], int count, float zero_level);

#endif
