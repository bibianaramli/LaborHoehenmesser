#include "sensor.h"

int configure_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cc[VTIME] = 5; // 0.5s Timeout
    tty.c_cc[VMIN] = 0;  // Non-blocking für macOS

    tcflush(fd, TCIFLUSH);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

int connect_to_sensor(char *tty_path) {
    int serial_fd = open(tty_path, OPEN_FLAGS);
    if (serial_fd < 0) {
        fprintf(stderr, "Fehler beim Öffnen von %s: %s\n", tty_path, strerror(errno));
        return -1;
    }
    if (configure_serial(serial_fd) != 0) {
        close(serial_fd);
        return -1;
    }
    return serial_fd;
}

float convert_to_sensor_val(const char *line) {
    char *endptr = NULL;
    errno = 0;
    float value = strtof(line, &endptr);
    if (!(errno == 0 && endptr != line)) {
        value = -1.0f;
    }
    return value;
}

int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

ssize_t read_sim(int serial_fd, char* chunk, size_t chunk_len) {
    static sim_data s_data = {32, 0, 
        {10.25f, 12.80f, 15.60f, 18.45f, 21.10f, 23.95f, 26.70f, 29.55f,
        32.30f, 35.15f, 37.90f, 40.75f, 43.50f, 46.35f, 49.10f, 51.95f,
        54.70f, 57.55f, 60.30f, 63.15f, 65.90f, 68.75f, 71.50f, 74.35f,
        77.10f, 79.95f, 82.70f, 85.55f, 88.30f, 91.15f, 93.90f, 99.60f}};

    sleep(1);
    float value = s_data.data[s_data.index];
    s_data.index = (s_data.index + 1) % s_data.length; 

    if (chunk_len == 0) return 0;
    return snprintf(chunk, chunk_len, "%f\n", value);
}

int open_database(sqlite3 **db, const char *db_name) {
    if (sqlite3_open(db_name, db) != SQLITE_OK) {
        fprintf(stderr, "Datenbankfehler: %s\n", sqlite3_errmsg(*db));
        return 1;
    }
    return 0;
}

int execute_sql(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL Fehler: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int cols = sqlite3_column_count(stmt);
        for (int i = 0; i < cols; i++) {
            printf("%s | ", sqlite3_column_text(stmt, i));
        }
        printf("\n");
    }
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}