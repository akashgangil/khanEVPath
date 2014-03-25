/**
 **   File: log.cpp
 **   Description: Logging functionality, always flushes.
 **/
#include <stdio.h>
#include <string.h>
#include <time.h>  
#include "params.h"
#include "log.h"

static int openFile = 0;
FILE *fileDescriptor = NULL;
static const char* LOGDIR = "/net/hu21/agangil3/KhanLogs/";

char buf[80];


char* currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

int log_open() {
    char logFile[100];
    sprintf(logFile, "%skhanlog-%s.log",LOGDIR, currentDateTime());
    printf("Log File Location: %s\n", logFile);
    fileDescriptor = fopen(logFile,"w");
    
    if(fileDescriptor == NULL) {
        printf("Cannot open log file.\n");
        return -1;
    } else {
        openFile = 1;
        return 0;
    }
}

void log_msg(const char *msg) {
    fprintf(stderr, "[%s]  %s\n", currentDateTime(), msg);
    fflush(stderr);
    fprintf(fileDescriptor, "[%s] %s\n", currentDateTime(), msg);
    fflush(fileDescriptor);
}

