#include <stdio.h>
#include <stdlib.h>
#include "sensor.h"
#include "database.h"

// --- Prototypen (Damit main die Funktionen kennt) ---
void aufgabe1(int fd);
void aufgabe2(int fd);
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
            case 0: printf("Programm wird beendet...\n"); break;
            default: printf("Option nicht verfuegbar.\n");
        }
    } while (choice != 0);

    return 0;
}