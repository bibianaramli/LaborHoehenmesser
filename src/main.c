#include <stdio.h>
#include <stdlib.h>
#include "sensor.h"
#include "database.h"
#include <sys/time.h> // Für hochauflösende Zeitmessung (Linux/WSL)

// --- Prototypen (Damit main die Funktionen kennt) ---
void aufgabe1(int fd);
void aufgabe2(int fd);
void aufgabe4();
float interpolate(float x, CalibrationPoint *lut, int count);
void clear_buffer(); // Hilfsfunktion um den Tastaturpuffer zu leeren

// --- Hilfsfunktion: Leert den Eingabepuffer ---
void clear_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void aufgabe1(int fd) {
    float real;
    printf("\n--- Aufgabe 1: Datenaufnahme (20 Messungen) ---\n");
    
    for (int i = 0; i < 20; i++) { // Auf 20 korrigiert laut deinem Kommentar
        printf("\nMessung %d/20\n", i + 1);
        printf("Echten Abstand in cm eingeben: ");
        
        if (scanf("%f", &real) != 1) {
            clear_buffer();
            break;
        }
        clear_buffer(); // \n entfernen

        float sensor = get_sensor_value(fd);
        printf("Sensor gelesen: %.2f cm\n", sensor);

        save_measurement(real, sensor);
    }
    export_to_csv("messung_1.csv");
    printf("\nAufgabe abgeschlossen. Daten exportiert nach messung_1.csv\n");
}

void aufgabe2(int fd) {
    CalibrationPoint lut[100];
    int count = get_calibration_data(lut, 100);

    if (count < 2) {
        printf("\n[FEHLER] Zu wenige Kalibrierungsdaten vorhanden. Bitte erst Aufgabe 1 ausführen!\n");
        return;
    }

    printf("\n--- Aufgabe 2: TEST Modus (Lineare Interpolation) ---\n");
    printf("Befehle: [ENTER] = Messung durchfuehren, [q] = Zurueck zum Hauptmenue\n");

    char input[10];
    while (1) {
        printf("Warten auf Eingabe (Enter/q): ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        if (input[0] == 'q' || input[0] == 'Q') {
            break; 
        }

        // Bei Enter (oder jeder anderen Eingabe außer q) wird gemessen
        float raw = get_sensor_value(fd);
        if (raw < 0) {
            printf("Sensor Fehler!\n");
            continue;
        }

        float corrected = interpolate(raw, lut, count);
        
        printf(">> ROHWERT: %.2f cm  =>  KORRIGIERT: %.2f cm\n", raw, corrected);
        save_test_measurement(raw, corrected);
    }
    
    export_to_csv("messung_2.csv");
    printf("Testdaten in messung_2.csv gespeichert.\n");
}

float interpolate(float x, CalibrationPoint *lut, int count) {
    // Randbereiche abfangen
    if (x <= lut[0].sensor_dist) return lut[0].real_dist;
    if (x >= lut[count-1].sensor_dist) return lut[count-1].real_dist;

    // Suche Nachbarn für die Lineare Interpolation
    for (int i = 0; i < count - 1; i++) {
        if (x >= lut[i].sensor_dist && x <= lut[i+1].sensor_dist) {
            float x1 = lut[i].sensor_dist;
            float x2 = lut[i+1].sensor_dist;
            float y1 = lut[i].real_dist;
            float y2 = lut[i+1].real_dist;
            
            // Mathematische Formel
            return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
        }
    }
    return x;
}
//Aufgabe 3
#include <sys/time.h> // Für hochauflösende Zeitmessung (Linux/WSL)

// Hilfsfunktion: Gibt die aktuelle Zeit in Sekunden zurück
double get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

// Visualisierung: ASCII-Balken
void print_histogram(float value) {
    int bar_length = (int)(value / 2.0); // 1 Raute pro 2 cm
    if (bar_length > 50) bar_length = 50; // Max Länge begrenzen
    
    printf("%6.2f cm | ", value);
    for (int i = 0; i < bar_length; i++) {
        printf("#");
    }
    printf("\n");
}

void aufgabe3(int fd) {
    CalibrationPoint lut[100];
    int count = get_calibration_data(lut, 100);
    
    int num_samples = 20;
    double timestamps[20];
    float values[20];
    
    printf("\n--- Aufgabe 3: Echtzeit-Profil & Timing ---\n");
    
    for (int i = 0; i < num_samples; i++) {
        timestamps[i] = get_timestamp();
        float raw = get_sensor_value(fd);
        values[i] = interpolate(raw, lut, count); // Korrigierten Wert nutzen
        
        // Live Visualisierung
        print_histogram(values[i]);
        
        save_measurement_task3(values[i], timestamps[i]);
    }

    // --- Zeit-Analyse ---
    double min_dt = 999.0, max_dt = 0.0, sum_dt = 0.0;
    
    for (int i = 1; i < num_samples; i++) {
        double dt = timestamps[i] - timestamps[i-1];
        if (dt < min_dt) min_dt = dt;
        if (dt > max_dt) max_dt = dt;
        sum_dt += dt;
    }
    
    double avg_dt = sum_dt / (num_samples - 1);

    printf("\n--- Timing Statistik ---\n");
    printf("Min Zeitabstand: %.4f s\n", min_dt);
    printf("Max Zeitabstand: %.4f s\n", max_dt);
    printf("Durchschnitt:    %.4f s\n", avg_dt);
    
    export_to_csv("messung_3.csv");
}
//Aufgabe4
void aufgabe4() {
    printf("\n--- Aufgabe 4: Mittelwert-Filter (Glättung) ---\n");

    // 1. Daten aus der Datenbank laden (die Werte aus Aufgabe 3)
    CalibrationPoint data[100];
    int count = get_calibration_data(data, 100);

    if (count < 3) {
        printf("Fehler: Zu wenige Daten für einen 3-Punkt-Filter (min. 3 benötigt)!\n");
        return;
    }

    float filtered[100];
    
    // 2. Filter anwenden: u(m) = (y(m-1) + y(m) + y(m+1)) / 3
    // Den ersten und letzten Wert übernehmen wir einfach, da sie keine zwei Nachbarn haben
    filtered[0] = data[0].real_dist; 
    
    for (int m = 1; m < count - 1; m++) {
        // Die Formel laut Aufgabenstellung 
        filtered[m] = (data[m-1].real_dist + data[m].real_dist + data[m+1].real_dist) / 3.0f;
    }

    filtered[count-1] = data[count-1].real_dist;

    // 3. Ausgabe und Export in messung_4.csv 
    printf("%-5s | %-10s | %-10s | %-10s\n", "Nr", "Original", "Gefiltert", "Differenz");
    printf("----------------------------------------------\n");
    
    FILE *f = fopen("messung_4.csv", "w");
    if (f == NULL) {
        printf("Fehler beim Erstellen von messung_4.csv\n");
        return;
    }
    fprintf(f, "sep=;\n");
    fprintf(f, "Nr,Original,Gefiltert,Differenz\n");

    for (int i = 0; i < count; i++) {
        float diff = data[i].real_dist - filtered[i];
        printf("%5d | %10.2f | %10.2f | %10.2f\n", i+1, data[i].real_dist, filtered[i], diff);
        fprintf(f, "%d;%.2f;%.2f;%.2f\n", i + 1, data[i].real_dist, filtered[i], diff);
    }

    fclose(f);
    printf("\nFilterung abgeschlossen. Daten in messung_4.csv gespeichert.\n");
}

int main() {
    init_db("labor.db");
    
    // Versuche Sensor zu verbinden
    int fd = connect_to_sensor("/dev/ttyUSB0");
    if (fd < 0 && SIM == 1) { // Nur Fehler wenn keine Simulation
        printf("Fehler: Sensor an /dev/ttyUSB0 nicht gefunden!\n");
        return 1;
    }

    int choice;
    do {
        printf("\n============================\n");
        printf("       HAUPTMENUE\n");
        printf("============================\n");
        printf("1. Aufgabe 1 (Datenaufnahme)\n");
        printf("2. Aufgabe 2 (Test-Modus/LUT)\n");
        printf("3. Aufgabe 3 (Echtzeit-Profil & Timing)\n");
        printf("4. Aufgabe 4 (Mittelwert-Filter)\n");
        printf("0. Beenden\n");
        printf("Wahl: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Ungueltige Eingabe!\n");
            clear_buffer();
            choice = -1;
            continue;
        }
        clear_buffer(); // Wichtig: \n aus dem Puffer entfernen!

        switch(choice) {
            case 1: aufgabe1(fd); break;
            case 2: aufgabe2(fd); break;
            case 3: aufgabe3(fd); break;
            case 4: aufgabe4(); break;
            case 0: printf("Programm wird beendet...\n"); break;
            default: printf("Option nicht verfuegbar.\n");
        }
    } while (choice != 0);

    return 0;
}