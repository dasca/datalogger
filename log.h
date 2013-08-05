#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define LOG_EXIT        1
#define LOG_NO_EXIT     0

char log_buf[100];

void log_entry(char *text, int doexit);
