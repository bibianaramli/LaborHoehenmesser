#include "sensor_utils.h"
#include <unistd.h>

#define SIM 1 // 1 für Simulation, 0 für echten Sensor

int main() {
    int mode = 6;
    int serial_fd = -1;
    float zero_level = 12.0; 

    #if !SIM
    serial_fd = connect_to_sensor("/dev/ttyUSB0");
    if (serial_fd < 0) { perror("Sensor Connect"); return 1; }
    #endif

    while (mode != 5) {
        if (mode == 6) {
            printf("\n-== Menue ==-\n1: DA\n0: EMS-Lab (Profilscan)\n5: Exit\nWahl: ");
            if (scanf("%d", &mode) != 1) break;
        }

        if (mode == 0) {
            // Automatische Kalibrierung vor dem Scan
            zero_level = calibrate_sensor(serial_fd, SIM);
            float collected_values[MAX_VALUES];
            int val_count = 0;
            int active_scan = 1;
            int object_detected = 0;
            char line_buf[READ_CHUNK];
            int line_pos = 0;

            printf("Scanne... Bewege den Block langsam unter dem Sensor.\n");

            while (active_scan && val_count < MAX_VALUES) {
                char chunk[READ_CHUNK];
                ssize_t b_read;

                #if SIM
                b_read = read_sim(chunk, sizeof(chunk));
                #else
                b_read = read(serial_fd, chunk, sizeof(chunk));
                #endif

                if (b_read > 0) {
                    for (int i = 0; i < b_read; i++) {
                        char c = chunk[i];
                        
                        // Sammle Zeichen bis zum Zeilenumbruch
                        if (c != '\n' && c != '\r') {
                            if (line_pos < READ_CHUNK - 1) {
                                line_buf[line_pos++] = c;
                            }
                        } 
                        else if (line_pos > 0) { // Zeile komplett
                            line_buf[line_pos] = '\0';
                            float val = convert_to_sensor_val(line_buf);
                            line_pos = 0; // Puffer zurücksetzen

                            // Ignoriere offensichtliche Fehlmessungen (0.0)
                            if (val <= 0.1) continue;

                            float diff = zero_level - val;

                            if (diff > 0.8) { // Schwellenwert etwas erhöht für Stabilität
                                object_detected = 1;
                                collected_values[val_count++] = val;
                                printf("Messung %d: Abstand %.2f cm (Höhe: %.2f cm) \n", val_count, val, diff);
                            } else if (object_detected) {
                                // Erst wenn ein Objekt da war UND jetzt die Diff klein ist -> Ende
                                printf("\nEnde des Objekts erkannt.\n");
                                active_scan = 0;
                                break;
                            }
                        }
                    }
                }
                usleep(10000); // 10ms warten für CPU-Entlastung
            }

            if (val_count > 0) {
                print_histogram(collected_values, val_count, zero_level);
            }
            
            mode = 6;
        }
        
        if (mode == 5) break;
    }

    if (serial_fd != -1) close(serial_fd);
    return 0;
}
