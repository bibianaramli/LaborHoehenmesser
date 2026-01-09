#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

typedef struct {
    int id;
    float real_dist; // Der Wert vom Lineal (Eingabe)
    float sensor_dist; // Der Wert aus der Black-Box (Sensor)
    float diff; // Berechnete Abweichung
} Measurement;

int init_db(const char *db_name);
int save_measurement(float real, float sensor); // Diese Funktion schreibt in die DB
void export_to_csv(const char *filename);

#endif