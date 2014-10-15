#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include "hiredis.h"


bool redis_init(int port);
std::string redis_getval(std::string file_id, std::string col);
std::string redis_getkey_cols(std::string col);
std::string redis_setval(std::string file_id, std::string col, std::string val);
void redis_remove_val(std::string fileid, std::string col, std::string val);

