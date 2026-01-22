#include "sensor_utils.h"

#define SIM 1 // 1 für Simulation, 0 für echten Sensor

int main() {
    int mode = 6;
    int serial_fd = -1;
    float zero_level = 0;
    float collected_heights[MAX_VALUES];

    #if !SIM
    serial_fd = connect_to_sensor("/dev/ttyUSB0");
    if (serial_fd < 0) { perror("Fehler: Sensor nicht gefunden"); return 1; }
    #endif

    while (mode != 5) {
        if (mode == 6) {
            printf("\n-== Menue ==-\n1: EMS-Labor\n5: Exit\nWahl: ");
            if (scanf("%d", &mode) != 1) break;
        }

        if (mode == 1) {
            zero_level = calibrate_sensor(serial_fd, SIM);
            int val_count = 0;
            int active_scan = 1;
            int object_detected = 0;
            float max_h = 0;
            char line_buf[READ_CHUNK];
            int line_pos = 0;

            printf("Scanne... Objekt unter den Sensor schieben.\n");

            while (active_scan && val_count < MAX_VALUES) {
                char chunk[READ_CHUNK];
                ssize_t b_read = SIM ? read_sim(chunk, sizeof(chunk)) : read(serial_fd, chunk, sizeof(chunk));

                if (b_read > 0) {
                    for (int i = 0; i < b_read; i++) {
                        char c = chunk[i];
                        if (c != '\n' && c != '\r') {
                            if (line_pos < READ_CHUNK - 1) line_buf[line_pos++] = c;
                        } else if (line_pos > 0) {
                            line_buf[line_pos] = '\0';
                            float val = convert_to_sensor_val(line_buf);
                            line_pos = 0;

                            if (val <= 0.1) continue;
                            float height = zero_level - val;

                            if (height > 0.8) { // Objekt erkannt
                                object_detected = 1;
                                if (height < 0) height = 0;
                                collected_heights[val_count++] = height;
                                if (height > max_h) max_h = height;
                                printf("Messung %d: Höhe %.2f cm\n", val_count, height);
                            } else if (object_detected) {
                                active_scan = 0; // Ende wenn Objekt weg
                                break;
                            }
                        }
                    }
                }
                usleep(10000);
            }

            if (val_count > 0) {
                print_histogram_horizontal(collected_heights, val_count, max_h);
                save_histogram_to_csv("scan_ergebnis.csv", collected_heights, val_count, zero_level);
            }
            mode = 6;
        }
        
        if (mode == 5) break;
    }

    if (serial_fd != -1) close(serial_fd);
    return 0;
}