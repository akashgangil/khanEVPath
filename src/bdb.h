#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <db_cxx.h>
#include "log.h"

bool bdb_init();
std::string bdb_getval(std::string file_id, std::string col);
std::string bdb_getkey_cols(std::string col);
std::string bdb_setval(std::string file_id, std::string col, std::string val);
void bdb_remove_val(std::string fileid, std::string col, std::string val);

