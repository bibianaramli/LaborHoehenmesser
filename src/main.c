#include <stdio.h>
#include "sensor.h"
#include "database.h"

void aufgabe1(int fd) {
    float real;
    printf("--- Aufgabe 1: Datenaufnahme (20 Messungen) ---\n");
    //remove("messung_1.csv"); // Lösche die CSV-Datei, bevor die Messungen beginnen.
    for (int i = 0; i < 2; i++) { // 20 measurements required 
        printf("\nMeasurement %d/20\n", i + 1);
        printf("Enter real distance in cm: ");
        if (scanf("%f", &real) != 1) break;

        float sensor = get_sensor_value(fd);
        printf("Sensor read: %.2f cm\n", sensor);

        save_measurement(real, sensor);
    }
    export_to_csv("messung_1.csv");
    printf("\nTask completed. Data exported to messung_1.csv\n");
}

int main() {
    init_db("labor.db");
    int fd = connect_to_sensor("/dev/ttyUSB0");

    int choice;
    do {
        printf("\n--- Main Menu ---\n1. Aufgabe 1 (DA)\n0. Exit\nChoice: ");
        scanf("%d", &choice);
        if (choice == 1) aufgabe1(fd);
    } while (choice != 0);

    return 0;
}