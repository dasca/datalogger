/* 
 * File:   log.c
 * Author: toffe
 *
 * Created on den 19 mars 2013, 20:23
 */
#include "log.h"

extern int KEEP_RUNNING;
extern int zExitCode;

void log_entry(char *text, int doexit) {
    time_t rawtime;
    struct tm *timeinfo;
    char buf[100];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buf, 100, "%y-%m-%d %H:%M:%S", timeinfo);
            
    fprintf(stderr, "%s:%s\r\n", buf, text);
    
    if (doexit != LOG_NO_EXIT) {
        KEEP_RUNNING = 0;
        zExitCode = doexit;           
    }
}
