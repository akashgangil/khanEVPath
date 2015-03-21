#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "evpath.h"
#include "ev_dfg.h"
#include "dfg_functions.h"
#include "khan_ffs.h"
#include "readConfig.h"

//extern FMField simple_field_list[];
//extern FMStructDescRec simple_format_list[]; 

EVclient test_client;
EVsource * source_handles;

static int
general_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
    printf("Received an event\n");
    return 1;
}


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: dfg_general_client config_file_name node_name\n");
        return 1;
    }

    std::string config_file_name = argv[1];
    std::string client_node_name = argv[2];

    CManager cm;
    EVclient_sinks sink_capabilities = NULL;
    EVclient_sources source_capabilities = NULL;
    cm = CManager_create();
    CMlisten(cm);

    char master_address[200];
    dfg_get_master_contact_func(master_address,"master.info");

    /*Read the config file*/
    ConfigParser_t cfg_slave;
    if(cfg_slave.readFile(config_file_name))
    {
      printf("Error: Cannot open config file %s", config_file_name.c_str());
      return 1;
    }


    /* Search for source and sink stones for this node and distinguish them*/
    std::vector<std::string> stone_names = cfg_slave.getSections();
    std::vector<std::string> handler_names;
    std::vector<bool> is_source;
    unsigned int num_of_sources = 0;
    for(std::vector<std::string>::iterator I = stone_names.begin(), E = stone_names.end(); I != E; ++I)
    {
        std::string temp_node_name;
        stone_type_t temp_type;
        std::string temp_handler_name;
        
        if(!config_read_node(cfg_slave, *I, temp_node_name))
        {
            log_err("Could not read node name for: %s", (*I).c_str());
            exit(1);
        }

        if(client_node_name.compare(temp_node_name))
            continue;

        if(!config_read_type(cfg_slave, *I, temp_type))
        {
            log_err("Could not read node type for: %s", (*I).c_str());
            exit(1);
        }

        if(temp_type == SOURCE)
        {
            is_source.push_back(true);
            ++num_of_sources;
        }
        else if(temp_type == SINK)
            is_source.push_back(false);
        else
            continue;

        temp_handler_name = (*I) + "_" + temp_node_name;
        handler_names.push_back(temp_handler_name);
    } 

    if(num_of_sources > 0)
    {
        source_handles = (EVsource *) malloc(sizeof(EVsource) * num_of_sources);
        num_of_sources = 0;
    }
        
    /* Allows for only one sink handler per node right now, also you can have multiple sinks
       but they all send the simple_format_list data type defined in khan_ffs.h */

    for(unsigned int counter = 0; counter < handler_names.size(); ++counter)
    {
        if(is_source[counter] == true)
        {
            source_handles[num_of_sources] = EVcreate_submit_handle(cm, -1, simple_format_list);
            char * temp_ptr = strdup(handler_names[counter].c_str());
            source_capabilities = EVclient_register_source(temp_ptr, source_handles[num_of_sources]);
            ++num_of_sources;
        }
        else
        {
            char * temp_ptr = strdup(handler_names[counter].c_str());
            sink_capabilities = EVclient_register_sink_handler(cm, temp_ptr, simple_format_list, 
                                                                  (EVSimpleHandlerFunc) general_handler, NULL);
        }
    }

            
    /*  Associate the client */
    char * temp_ptr = strdup(client_node_name.c_str());
    test_client = EVclient_assoc(cm, temp_ptr, master_address, source_capabilities, sink_capabilities);
    free(temp_ptr);
    temp_ptr = NULL;

	if (EVclient_ready_wait(test_client) != 1) {
	/* dfg initialization failed! */
	exit(1);
    }

    




    /*! [Shutdown code] */
	CMrun_network(cm);

	return 1;

}

