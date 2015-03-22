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
python_general_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
    log_info("Received a python event");
    simple_rec_ptr event = (_simple_rec*)vevent;

    /* FIXME: Needless waste of source stones here, need a better system in the future, but for now this is fine */
    if(EVclient_source_active(source_handles[0]))
    {
        printf("Sending the message on...\n");
        EVsubmit(source_handles[0], event, NULL);
    }
    else
        log_info("No active client source registered");
    return 1;
}

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


    /* Search for source, sink, and python stones for this node and distinguish them*/
    std::vector<std::string> stone_names = cfg_slave.getSections();
    std::vector<std::string> handler_names;
    std::vector<stone_type_t> stone_types;
    unsigned int num_of_sources = 0;
    for(std::vector<std::string>::iterator I = stone_names.begin(), E = stone_names.end(); I != E; ++I)
    {
        std::string temp_node_name;
        stone_type_t temp_type;
        std::string temp_handler_name;
        
        /* Check for stones that are relevant to our node for registration purposes */
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

        if(temp_type == SOURCE || temp_type == PYTHON)
            ++num_of_sources;


        /* If type is a python type, create a unique sink and source handler name
           Otherwise, just do the normal naming convention */ 
        if(temp_type == PYTHON)
        {
            // Need the python stone to turn into two stone names *I + "py_src"
            std::string temp_handler_name_src = (*I) + "_py_src_" + temp_node_name;
            std::string temp_handler_name_sink = (*I) + "_py_sink_" + temp_node_name;
            stone_types.push_back(SOURCE);
            stone_types.push_back(PYTHON);
            handler_names.push_back(temp_handler_name_src);
            handler_names.push_back(temp_handler_name_sink);
        }
        else
        {
            temp_handler_name = (*I) + "_" + temp_node_name;
            handler_names.push_back(temp_handler_name);
            stone_types.push_back(temp_type);
        }
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
        if(stone_types[counter] == SOURCE)
        {
            source_handles[num_of_sources] = EVcreate_submit_handle(cm, -1, simple_format_list);
            char * perm_ptr = strdup(handler_names[counter].c_str());
            source_capabilities = EVclient_register_source(perm_ptr, source_handles[num_of_sources]);
            ++num_of_sources;
        }
        else if(stone_types[counter] == SINK)
        {
            char * perm_ptr = strdup(handler_names[counter].c_str());
            sink_capabilities = EVclient_register_sink_handler(cm, perm_ptr, simple_format_list, 
                                                                  (EVSimpleHandlerFunc) general_handler, NULL);
        }
        else if(stone_types[counter] == PYTHON)
        {
            char * perm_ptr = strdup(handler_names[counter].c_str());
            sink_capabilities = EVclient_register_sink_handler(cm, perm_ptr, simple_format_list,
                                                                  (EVSimpleHandlerFunc) python_general_handler, NULL);

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

