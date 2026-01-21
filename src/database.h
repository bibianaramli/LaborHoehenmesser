#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

typedef struct {
    int id;
    float real_dist; // Der Wert vom Lineal (Eingabe)
    float sensor_dist; // Der Wert aus der Black-Box (Sensor)
    float diff; // Berechnete Abweichung
} Measurement;
// Für die Look-Up Tabelle in Aufgabe 2
typedef struct {
    float real_dist;   
    float sensor_dist; 
} CalibrationPoint;
// Aufgabe3
typedef struct {
    float value;
    double timestamp;
} TimeMeasurement;

int init_db(const char *db_name);
int save_measurement(float real, float sensor); // Diese Funktion schreibt in die DB
void export_to_csv(const char *filename);
// NEU: Funktionen für Aufgabe 2
int get_calibration_data(CalibrationPoint *points, int max_points);
int save_test_measurement(float sensor_raw, float interpolated);

//Aufgabe 3
int save_measurement_task3(float dist, double ts);
void export_task3_to_csv(const char *filename);

#endif

