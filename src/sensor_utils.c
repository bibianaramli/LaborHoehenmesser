#include "sensor_utils.h"
#include <termios.h>
#include <fcntl.h>

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
    int fd = open(tty_path, O_RDWR | O_NOCTTY);
    if (fd < 0) return -1;
    if (configure_serial(fd) != 0) { close(fd); return -1; }
    return fd;
}

float convert_to_sensor_val(const char *line) {
    char *endptr;
    return strtof(line, &endptr);
}

ssize_t read_sim(char* chunk, size_t chunk_len) {
    static sim_data s_data = {11, 0, 
        {10.2, 10.0, 8.5, 7.0, 5.0, 4.5, 5.0, 7.0, 8.5, 10.0, 10.2}
    };
    usleep(100000); 
    float value = s_data.data[s_data.index];
    s_data.index = (s_data.index + 1) % s_data.length;
    return snprintf(chunk, chunk_len, "%.2f\n", value);
}

int open_database(sqlite3 **db, const char *db_name) {
    return sqlite3_open(db_name, db);
}

int execute_sql(sqlite3 *db, const char *sql) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    return rc;
}

// Das Histogramm "von links nach rechts" (Profil-Ansicht)
void print_histogram_horizontal(float values[], int count, float max_val) {
    printf("\n--- Profil-Scan (Horizontal) ---\n");
    
    for (float h = max_val; h > 0; h -= 0.5) {
        for (int i = 0; i < count; i++) {
            if (values[i] >= h) {
                printf("#");
            } else {
                printf(" ");
            }
        }
        printf(" %.1f cm\n", h);
    }
}

float calibrate_sensor(int fd, int is_sim) {
    printf("Kalibrierung läuft (Sensor auf Boden richten)...\n");
    float sum = 0;
    int count = 0;
    char line_buf[READ_CHUNK];
    int line_pos = 0;
    while (count < 10) {
        char chunk[READ_CHUNK];
        ssize_t b_read = is_sim ? read_sim(chunk, sizeof(chunk)) : read(fd, chunk, sizeof(chunk));
        if (b_read > 0) {
            for (int i = 0; i < b_read; i++) {
                if (chunk[i] == '\n' && line_pos > 0) {
                    line_buf[line_pos] = '\0';
                    float val = convert_to_sensor_val(line_buf);
                    if (val > 1.0) { sum += val; count++; }
                    line_pos = 0;
                } else if (chunk[i] != '\r' && chunk[i] != '\n') {
                    if (line_pos < READ_CHUNK - 1) line_buf[line_pos++] = chunk[i];
                }
            }
        }
    }
    return sum / 10.0f;
}

void save_histogram_to_csv(const char* filename, float values[], int count, float zero_level) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    fprintf(fp, "Index,Hoehe_cm\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d,%.2f\n", i, values[i]);
    }
    fclose(fp);
    printf("CSV gespeichert: %s\n", filename);
}