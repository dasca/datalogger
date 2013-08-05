/* 
 * File:   c_sqlite.c
 * Author: toffe
 *
 * Created on den 20 mars 2013, 14:12
 */

#include "c_onewire.h"
#include "log.h"
#include "c_sqlite.h"
#include <sqlite3.h>

const char *dbname = "/var/datalogger.db";

sqlite3 *openDB() {
    sqlite3 *db;
    int rc = sqlite3_open(dbname, &db);

    if (rc) {
        sprintf(log_buf, "Unable to open database '%s'", dbname);
        sqlite3_close(db);
        log_entry(log_buf, LOG_EXIT);
    }

    return db;
}

void initDB() {
    sqlite3 *db = NULL;
    char *zErrMsg = 0;
    int rc;
    db = openDB();

    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS sensors (\
                                id      INTEGER PRIMARY KEY,\
                                adress  TEXT            NOT NULL,\
                                name    TEXT            NOT NULL,\
                                created INTEGER         NOT NULL)",
            NULL, NULL, &zErrMsg);


    if (rc != SQLITE_OK) {
        sprintf(log_buf, "SQL Error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        log_entry(log_buf, LOG_EXIT);
    }

    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS readings (\
                                id      INTEGER PRIMARY KEY,\
                                device  INTEGER         NOT NULL,\
                                reading REAL            NOT NULL,\
                                created INTEGER         NOT NULL)",
            NULL, NULL, &zErrMsg);


    if (rc != SQLITE_OK) {
        sprintf(log_buf, "SQL Error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        log_entry(log_buf, LOG_EXIT);
    }
    
    log_entry("Successfully opened database", LOG_NO_EXIT);

    sqlite3_close(db);
}

static int deviceFindCallback(void *retval, int numCols, char **colText, 
char **colName) {
    int i;

    sscanf(colText[0], "%i", &i);

    *(int*) retval = i;

    return 0;
}

int addReading(struct sOneWireDevice *devicePtr) {
    sqlite3 *db = openDB();
    char *zErrMsg = 0;
    int rc, sensorID = 0;
    char sqlString[100];
    sprintf(sqlString, "SELECT * FROM sensors WHERE adress='%s'", 
devicePtr->adress);
    rc = sqlite3_exec(db, sqlString, deviceFindCallback, &sensorID, &zErrMsg);


    if (rc != SQLITE_OK) {
        sprintf(log_buf, "SQL Error 1: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        log_entry(log_buf, LOG_EXIT);
    }

    if (sensorID == 0) {
        sprintf(sqlString, "INSERT INTO sensors (adress, name, created) VALUES ('%s','%s', %i)", devicePtr->adress, devicePtr->adress, (int) time(NULL));
        rc = sqlite3_exec(db, sqlString, 0, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            sprintf(log_buf, "SQL Error 2: %s", zErrMsg);
            sqlite3_free(zErrMsg);
            sqlite3_close(db);
            log_entry(log_buf, LOG_NO_EXIT);
            return(0);
        }
        sensorID = sqlite3_last_insert_rowid(db);
        sprintf(log_buf, "New sensor %s successful inserted. ID: %i\r\n", devicePtr->adress, sensorID);
        log_entry(log_buf, LOG_NO_EXIT);
    }

    sprintf(sqlString, "INSERT INTO readings (device, reading, created) VALUES (%i, %4.2f, %i)", sensorID, devicePtr->value, (int) time(NULL));
    rc = sqlite3_exec(db, sqlString, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sprintf(log_buf, "SQL Error 3: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        log_entry(log_buf, LOG_NO_EXIT);
        return(0);
    }
    
    //sensorID = sqlite3_last_insert_rowid(db);
    sqlite3_close(db);
    return(1);
}


