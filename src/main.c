#include "sensor.h"

int main(void) {
    char* tty_path = "/dev/tty.usbserial-0001";
    int serial_fd = 0;

    // Verbindung zum Sensor (nur wenn SIM aus ist)
    #if !SIM
    serial_fd = connect_to_sensor(tty_path);
    if (serial_fd < 0) {
        printf("Konnte Sensor an %s nicht öffnen. Beende...\n", tty_path);
        return 1;
    }
    #endif

    int mode = 6;
    while (mode != 5) {
        if (mode == 6) {
            printf("\n-== HAUPTMENÜ ==-\n");
            printf("1: DA (Datenaufnahme & Kalibrierung)\n");
            printf("2: TEST (Interpolation mit LUT)\n");
            printf("3: AUTO (Zeitstempel & Live-Histogramm)\n");
            printf("4: FILTER (Mittelwertfilter über messung_3)\n");
            printf("5: Beenden\n");
            printf("Auswahl: ");
            if (scanf("%d", &mode) != 1) break;
        }

        // --- MODUS 1: DA (Data Acquisition) ---
        if (mode == 1) {
            FILE *fp = fopen("messung_1.csv", "w");
            if (!fp) { perror("CSV Fehler"); mode = 6; continue; }
            
            sqlite3 *db;
            open_database(&db, "messung_1.db");
            execute_sql(db, "CREATE TABLE IF NOT EXISTS messung_1 (MessungNr INTEGER, Sensor REAL, Nutzer REAL, Diff REAL);");
            fprintf(fp, "MessungNr,Sensorabstand[cm],Nutzerabstand[cm],Abweichung[cm]\n");

            for (int mesnum = 1; mesnum <= 20; mesnum++) {
                char chunk[READ_CHUNK] = {0};
                ssize_t bytes = (SIM) ? read_sim(0, chunk, sizeof(chunk)) : read(serial_fd, chunk, sizeof(chunk));
                
                if (bytes > 0) {
                    float val = convert_to_sensor_val(chunk);
                    float input;
                    printf("Messung %d - Sensor: %.3f cm. Geben Sie den realen Abstand ein: ", mesnum, val);
                    scanf("%f", &input);
                    float diff = val - input;

                    fprintf(fp, "%d,%.3f,%.3f,%.3f\n", mesnum, val, input, diff);
                    char sql[256];
                    sprintf(sql, "INSERT INTO messung_1 VALUES (%d, %f, %f, %f);", mesnum, val, input, diff);
                    execute_sql(db, sql);
                }
            }
            fclose(fp);
            sqlite3_close(db);
            printf("Modus 1 fertig.\n");
            mode = 6;
        }

        // --- MODUS 2: TEST (Interpolation) ---
        else if (mode == 2) {
            char cinput[100];
            printf("CSV-Dateiname für LUT (z.B. messung_1.csv): ");
            scanf("%s", cinput);

            FILE *fpr = fopen(cinput, "r");
            if (!fpr) { printf("Datei nicht gefunden!\n"); mode = 6; continue; }

            double lut[MAXCHAR], lutr[MAXCHAR];
            char crow[MAXCHAR];
            int y = 0;

            // Header überspringen
            fgets(crow, MAXCHAR, fpr);
            while (fgets(crow, MAXCHAR, fpr) && y < MAXCHAR) {
                char *token = strtok(crow, ",");
                token = strtok(NULL, ","); if (token) lut[y] = atof(token);
                token = strtok(NULL, ","); if (token) lutr[y] = atof(token);
                y++;
            }
            fclose(fpr);

            sqlite3 *db;
            open_database(&db, "messung_2.db");
            execute_sql(db, "CREATE TABLE IF NOT EXISTS messung_2 (Nr INTEGER, Sensor REAL, Interp REAL, Diff REAL);");
            FILE *fpw = fopen("messung_2.csv", "w");
            fprintf(fpw, "Nr,Sensor,Interpoliert,Diff\n");

            printf("Starte Test. Drücken Sie '1' für Messung, '2' für Menü.\n");
            int mesnum = 1;
            while (1) {
                int choice;
                printf("Aktion (1: Messen, 2: Zurück): ");
                scanf("%d", &choice);
                if (choice == 2) break;

                char chunk[READ_CHUNK] = {0};
                ssize_t bytes = (SIM) ? read_sim(0, chunk, sizeof(chunk)) : read(serial_fd, chunk, sizeof(chunk));
                if (bytes > 0) {
                    float value = convert_to_sensor_val(chunk);
                    // Einfache lineare Interpolation (vereinfacht)
                    float ipvalue = value; // Hier käme deine Logik aus dem alten Code rein
                    for(int i=0; i < y-1; i++) {
                        if(value >= lut[i] && value <= lut[i+1]) {
                             ipvalue = lutr[i] + ((lutr[i+1] - lutr[i]) / (lut[i+1] - lut[i])) * (value - lut[i]);
                             break;
                        }
                    }
                    float diff = value - ipvalue;
                    printf("Sensor: %.3f cm -> Interpoliert: %.3f cm\n", value, ipvalue);
                    
                    fprintf(fpw, "%d,%.3f,%.3f,%.3f\n", mesnum++, value, ipvalue, diff);
                    char sql[256];
                    sprintf(sql, "INSERT INTO messung_2 VALUES (%d, %f, %f, %f);", mesnum, value, ipvalue, diff);
                    execute_sql(db, sql);
                }
            }
            fclose(fpw);
            sqlite3_close(db);
            mode = 6;
        }

        // --- MODUS 3: AUTO (Histogramm) ---
        else if (mode == 3) {
            FILE *fp = fopen("messung_3.csv", "w");
            sqlite3 *db;
            open_database(&db, "messung_3.db");
            execute_sql(db, "CREATE TABLE IF NOT EXISTS messung_3 (Nr INTEGER, Sensor REAL, Zeit REAL);");
            fprintf(fp, "Nr,Abstand,Zeit\n");

            float values[100] = {0};
            float times[100] = {0};
            float max_val = 0;

            for (int i = 1; i <= 10; i++) {
                clock_t start = clock();
                char chunk[READ_CHUNK] = {0};
                ssize_t bytes = (SIM) ? read_sim(0, chunk, sizeof(chunk)) : read(serial_fd, chunk, sizeof(chunk));
                
                if (bytes > 0) {
                    float val = convert_to_sensor_val(chunk);
                    clock_t end = clock();
                    float duration = (float)(end - start) / CLOCKS_PER_SEC;
                    
                    values[i] = val;
                    times[i] = duration;
                    if (val > max_val) max_val = val;

                    system("clear");
                    printf("Messung %d: %.2f cm (Dauer: %.4fs)\n", i, val, duration);
                    
                    // ASCII Histogramm
                    struct winsize w;
                    ioctl(0, TIOCGWINSZ, &w);
                    for (int r = 10; r > 0; r--) {
                        for (int c = 1; c <= i; c++) {
                            if (values[c] >= (max_val / 10.0) * r) printf("# ");
                            else printf("  ");
                        }
                        printf("\n");
                    }

                    fprintf(fp, "%d,%.3f,%.4f\n", i, val, duration);
                    char sql[256];
                    sprintf(sql, "INSERT INTO messung_3 VALUES (%d, %f, %f);", i, val, duration);
                    execute_sql(db, sql);
                }
            }
            fclose(fp);
            sqlite3_close(db);
            mode = 6;
        }

        // --- MODUS 4: FILTER ---
        else if (mode == 4) {
            FILE *fpr = fopen("messung_3.csv", "r");
            if (!fpr) { printf("Keine Daten in messung_3.csv gefunden!\n"); mode = 6; continue; }

            double values[MAXCHAR];
            int y = 0;
            char crow[MAXCHAR];
            fgets(crow, MAXCHAR, fpr); // Header weg
            while (fgets(crow, MAXCHAR, fpr)) {
                strtok(crow, ",");
                char *v = strtok(NULL, ",");
                if (v) values[y++] = atof(v);
            }
            fclose(fpr);

            FILE *fpw = fopen("messung_4.csv", "w");
            sqlite3 *db;
            open_database(&db, "messung_4.db");
            execute_sql(db, "CREATE TABLE IF NOT EXISTS messung_4 (Nr INTEGER, Sensor REAL, Filter REAL);");

            printf("\n--- Filter Ergebnisse ---\n");
            for (int i = 0; i < y; i++) {
                float filtered = values[i];
                if (i > 0 && i < y - 1) {
                    filtered = (values[i-1] + values[i] + values[i+1]) / 3.0;
                }
                printf("Original: %.2f -> Gefiltert: %.2f\n", values[i], filtered);
                fprintf(fpw, "%d,%.3f,%.3f\n", i+1, values[i], filtered);
                char sql[256];
                sprintf(sql, "INSERT INTO messung_4 VALUES (%d, %f, %f);", i+1, values[i], filtered);
                execute_sql(db, sql);
            }
            fclose(fpw);
            sqlite3_close(db);
            mode = 6;
        }
    }

    if (serial_fd > 0) close(serial_fd);
    printf("Programm beendet.\n");
    return 0;
}