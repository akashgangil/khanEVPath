#include "measurements.h"

#include "stdio.h"

struct stopwatch_t* sw;
FILE* mts_file;

void measurements_init(){
  stopwatch_init();
  sw = stopwatch_create();
  mts_file = fopen("measurements.txt", "wb");
}

void measurements_cleanup(){
  stopwatch_destroy(sw);
  fclose(mts_file);
}
