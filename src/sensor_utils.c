#include "sensor_utils.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
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
    static sim_data s_data = {32, 0, 
        {10.2, 10.2, 8.5, 7.0, 5.0, 5.0, 5.0, 7.0, 8.5, 10.2, 10.2} // Beispiel Profil
    };
    usleep(100000); // 100ms Simulation
    float value = s_data.data[s_data.index];
    s_data.index = (s_data.index + 1) % 11; // Nur Beispielwerte nutzen
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

void print_histogram(float values[], int count, float zero_level) {
    printf("\n--- Styrodur Profil (Histogramm) ---\n");
    for (int i = 0; i < count; i++) {
        float height = zero_level - values[i];
        if (height < 0) height = 0;
        printf("[%03d]: ", i);
        for (int j = 0; j < (int)(height * 2); j++) printf("#");
        printf(" (%.2f cm)\n", height);
    }
}
float calibrate_sensor(int fd, int is_sim) {
    printf("Kalibrierung: Bitte Sensor auf den leeren Boden richten...\n");
    float sum = 0;
    int count = 0;
    char line_buf[READ_CHUNK];
    int line_pos = 0;

    while (count < 10) {
        char chunk[READ_CHUNK];
        ssize_t b_read;
        if (is_sim) b_read = read_sim(chunk, sizeof(chunk));
        else b_read = read(fd, chunk, sizeof(chunk));

        if (b_read > 0) {
            for (int i = 0; i < b_read; i++) {
                if (chunk[i] == '\n' && line_pos > 0) {
                    line_buf[line_pos] = '\0';
                    float val = convert_to_sensor_val(line_buf);
                    if (val > 1.0) { // Ignoriere Fehlmessungen
                        sum += val;
                        count++;
                        printf("Kalibrierung Schritt %d/10: %.2f cm\r", count, val);
                        fflush(stdout);
                    }
                    line_pos = 0;
                } else if (chunk[i] != '\r' && chunk[i] != '\n') {
                    if (line_pos < READ_CHUNK - 1) line_buf[line_pos++] = chunk[i];
                }
            }
        }
        usleep(50000);
    }
    float result = sum / 10.0f;
    printf("\nKalibrierung fertig. Null-Niveau: %.2f cm\n", result);
    return result;
}
void save_histogram_to_csv(const char* filename, float values[], int count, float zero_level) {
    // Wir versuchen die Datei ohne Pfadangabe zu öffnen
    // Wenn das fehlschlägt, liegt es oft an den Rechten des 'build' Ordners
    FILE *fp = fopen(filename, "w"); 
    
    if (fp == NULL) {
        // Falls das fehlschlägt, versuchen wir es im übergeordneten Ordner
        char fallback_path[256];
        snprintf(fallback_path, sizeof(fallback_path), "../%s", filename);
        fp = fopen(fallback_path, "w");
        
        if (fp == NULL) {
            fprintf(stderr, "Kritischer Fehler: Kann Datei weder in '.' noch in '..' erstellen.\n");
            fprintf(stderr, "System-Fehlermeldung: %s\n", strerror(errno));
            return;
        }
        printf("Hinweis: Datei wurde im Hauptordner (statt build) gespeichert.\n");
    }

    fprintf(fp, "Index,Hoehe_cm,Profil_Horizontal\n");
    for (int i = 0; i < count; i++) {
        float height = zero_level - values[i];
        if (height < 0) height = 0;
        fprintf(fp, "%d,%.2f,", i, height);
        int num_hashes = (int)(height * 2); 
        for (int j = 0; j < num_hashes; j++) fputc('#', fp);
        fprintf(fp, "\n");
    }
    fclose(fp);
    printf("Erfolg! Datei gespeichert.\n");
}