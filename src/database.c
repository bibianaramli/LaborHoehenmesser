#include "database.h"
#include <stdio.h>
#include <stdlib.h>

sqlite3 *db;

int init_db(const char *db_name) {
    int rc = sqlite3_open(db_name, &db);
    if (rc != SQLITE_OK) return -1;
    char *sql = "CREATE TABLE IF NOT EXISTS measurements ("  //Tabelle erstellen, falls sie nicht exsistiert
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "real_dist REAL, sensor_dist REAL, diff REAL);";
    int rc_create = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc_create != SQLITE_OK) return rc_create;

    // Tabelle leeren, damit nur die aktuellen 20 Werte drin sind
   //const char *sql_delete = "DELETE FROM measurments;";
   //int rc_delete = sqlite3_exec(db, sql_delete, 0, 0, 0);
   //return rc_delete; // Gibt 0 bei Erfolg oder Fehlercode zurück
}

int save_measurement(float real, float sensor) {
    char sql[256];
    sprintf(sql, "INSERT INTO measurements (real_dist, sensor_dist, diff) VALUES (%f, %f, %f);", 
            real, sensor, real - sensor);
    return sqlite3_exec(db, sql, 0, 0, 0);
}

void export_to_csv(const char *filename) {
    
    FILE *f = fopen(filename, "w");
    fprintf(f, "Nr,Real,Sensor,Diff\n");
    sqlite3_stmt *res;
    sqlite3_prepare_v2(db, "SELECT * FROM measurements", -1, &res, 0);
    while (sqlite3_step(res) == SQLITE_ROW) {
        fprintf(f, "%d,%.2f,%.2f,%.2f\n", 
                sqlite3_column_int(res, 0),
                sqlite3_column_double(res, 1),
                sqlite3_column_double(res, 2),
                sqlite3_column_double(res, 3));
    }
    sqlite3_finalize(res);
    fclose(f);
    printf("Datei %s wurde erfolgreich erstellt.\n", filename);
}

int get_calibration_data(CalibrationPoint *points, int max_points) {
    sqlite3_stmt *res;
    const char *sql = "SELECT real_dist, sensor_dist FROM measurements ORDER BY sensor_dist ASC";
    
    if (sqlite3_prepare_v2(db, sql, -1, &res, 0) != SQLITE_OK) return 0;

    int i = 0;
    while (sqlite3_step(res) == SQLITE_ROW && i < max_points) {
        // Hier die Namen anpassen:
        points[i].real_dist = (float)sqlite3_column_double(res, 0);
        points[i].sensor_dist = (float)sqlite3_column_double(res, 1);
        i++;
    }
    sqlite3_finalize(res);
    return i;
}

int save_test_measurement(float sensor_raw, float interpolated) {
    char sql[256];
    // Wir speichern den berechneten "realen" Wert und den rohen Sensorwert
    sprintf(sql, "INSERT INTO measurements (real_dist, sensor_dist, diff) VALUES (%f, %f, %f);", 
            interpolated, sensor_raw, interpolated - sensor_raw);
    return sqlite3_exec(db, sql, 0, 0, 0);
}