#include "utils.h"
#include "database.h"

struct timespec start, stop;
double time_spent;

int vold_calls=0;
int readdir_calls=0;
int access_calls=0;
int getattr_calls=0;
int read_calls=0;
int write_calls=0;
int create_calls=0;
int rename_calls=0;
double tot_time=0;
double vold_avg_time=0;
double readdir_avg_time=0;
double access_avg_time=0;
double getattr_avg_time=0;
double read_avg_time=0;
double write_avg_time=0;
double create_avg_time=0;
double rename_avg_time=0;
double localize_time=0;
int redis_calls=0;
double redis_avg_time=0;

bool init_database(std::string host, int port){
  #ifdef VOLDEMORT_FOUND
  if(DATABASE==VOLDEMORT) {
    return voldemort_init();
  }
  #endif
  #ifdef REDIS_FOUND
  if(DATABASE==REDIS){
    return redis_init(host, port);
  }
  #endif
  #ifdef BDB_FOUND
    if(DATABASE==BDB) {
      return bdb_init();
    }
  #endif
}


std::string database_setval(std::string file_id, std::string col, std::string val){
  file_id=trim(file_id);
  col=trim(col);
  val=trim(val);
  #ifdef VOLDEMORT_FOUND
  if(DATABASE==VOLDEMORT){
    vold_calls++;
    std::string retstd::string="fail";
    retstd::string=voldemort_setval(file_id,col,val);
    return retstd::string;
  }
  #endif
  #ifdef REDIS_FOUND
  if(DATABASE==REDIS){
    redis_calls++;
    std::string retstring="fail";
    retstring=redis_setval(file_id,col,val);
    return retstring;
  }
  #endif
  #ifdef BDB_FOUND
  if(DATABASE==BDB) {
    std::string retstring = bdb_setval(file_id, col, val);
    return retstring;
  }
  #endif  
}


std::string database_getval(std::string col, std::string val){
  //fprintf(stderr, "in getval with %s %s", col.c_str(), val.c_str());
  col=trim(col);
  val=trim(val);
  #ifdef VOLDEMORT_FOUND
  if(DATABASE==VOLDEMORT){
    //log_msg("using vold");
    vold_calls++;
    std::string retstd::string=voldemort_getval(col,val);
    return retstd::string;
  }
  #endif
  #ifdef REDIS_FOUND
  if(DATABASE==REDIS){
    redis_calls++;
    std::string retstring=redis_getval(col,val);
    return retstring;
  }
  #endif
  #ifdef BDB_FOUND
  if(DATABASE==BDB) {
    std::string retstd::string = bdb_getval(col, val);
    return retstd::string;
  }
  #endif  
}

std::string database_getvals(std::string col){
  col=trim(col);
  #ifdef VOLDEMORT_FOUND
  if(DATABASE==VOLDEMORT){
    std::string retstr=voldemort_getkey_cols(col);
    return retstr;
  }
  #endif
  #ifdef REDIS_FOUND
  if(DATABASE==REDIS){
    redis_calls++;
    std::string retstr=redis_getkey_cols(col);
    return retstr;
  }
  #endif
  #ifdef BDB_FOUND
  if(DATABASE==BDB) {
    std::string retstd::string = bdb_getkey_cols(col);
    return retstd::string;
  }
  #endif  
}

void database_remove_val(std::string file, std::string col, std::string val){
  file=trim(file);
  col=trim(col);
  val=trim(val);
  #ifdef VOLDEMORT_FOUND
  if(DATABASE==VOLDEMORT){
    voldemort_remove_val(file,col,val);
    return;
  }
  #endif
  #ifdef REDIS_FOUND
  if(DATABASE==REDIS){
    redis_remove_val(file,col,val);
    return;
  }
  return;
  #endif
  #ifdef BDB_FOUND
    if(DATABASE==BDB) {
      bdb_remove_val(file, col, val);
      return;
    }
  #endif  
}



