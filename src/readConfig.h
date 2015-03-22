#include <string>
#include <stdio.h>
#include <vector>
#include "dfg_functions.h"
#include "cfgparser.h"
#include "log.h"

// I feel as though the names are self_explanatory...
// Every function returns 1 on success and 0 on failure
int config_read_node(const ConfigParser_t & cfg, std::string stone_section, std::string & node_name);

int config_read_type(const ConfigParser_t & cfg, std::string stone_section, stone_type_t & what_type);

int config_read_incoming(const ConfigParser_t & cfg, std::string stone_section, std::vector<std::string> & incoming_list);

int config_read_code(const ConfigParser_t & cfg, stone_struct & stone);


