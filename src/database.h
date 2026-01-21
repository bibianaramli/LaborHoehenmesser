#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

typedef struct {
    int id;
    float real_dist; // Der Wert vom Lineal (Eingabe)
    float sensor_dist; // Der Wert aus der Black-Box (Sensor)
    float diff; // Berechnete Abweichung
} Measurement;
// NEU: Für die Look-Up Tabelle in Aufgabe 2
typedef struct {
    float real_dist;   
    float sensor_dist; 
} CalibrationPoint;

int init_db(const char *db_name);
int save_measurement(float real, float sensor); // Diese Funktion schreibt in die DB
void export_to_csv(const char *filename);
// NEU: Funktionen für Aufgabe 2
int get_calibration_data(CalibrationPoint *points, int max_points);
int save_test_measurement(float sensor_raw, float interpolated);

#endif

