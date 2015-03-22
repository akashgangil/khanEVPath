#include "readConfig.h"

int config_read_node(const ConfigParser_t & cfg, std::string stone_section, std::string & node_name)
{
    if(!cfg.getValue(stone_section, "node", &node_name))
    {
        fprintf(stderr, "Failure to find node value in %s\n", stone_section.c_str());
        return 0;
    }
    return 1;

}

int config_read_type(const ConfigParser_t & cfg, std::string stone_section, stone_type_t & what_type)
{
    std::string temp;
    if(!cfg.getValue(stone_section, "type", &temp))
    {
        log_err("Failure to find type value in %s", stone_section.c_str());
        return 0;
    }

    if(!temp.compare("source"))
    {
        what_type = SOURCE;
        return 1;
    }
    else if(!temp.compare("sink"))
    {
        what_type = SINK;
        return 1;
    }
    else if(!temp.compare("python"))
    {
        what_type = PYTHON;
        return 1;
    }
    else
    {
        log_err("Unidentified stone_type of value %s", temp.c_str());
        return 0;
    }
    
    return 0;
}

int config_read_incoming(const ConfigParser_t & cfg, std::string stone_section, std::vector<std::string> & incoming_list)
{
    if(!cfg.getValue(stone_section, "incoming", &incoming_list))
    {
        log_err("Faliure to return correct incoming list from %s", stone_section.c_str());
        return 0;
    }
    return 1;
}


int config_read_code(const ConfigParser_t & cfg, stone_struct & stone)
{
    return 1;

}

