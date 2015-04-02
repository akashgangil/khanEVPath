#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "evpath.h"
#include "ev_dfg.h"
#include "dfg_functions.h"
#include "khan_ffs.h"
#include "khan.h"
#include "readConfig.h"
#include "Python.h"
#include "fileprocessor.h"


EVclient test_client;
EVsource * source_handles;
std::string script_name;
std::string method_name;
std::vector < std::string > servers;
std::vector < std::string > server_ids;

/* Used for creating the recursive subdirectories */ 
static void _mkdir(const char *dir) {
  char tmp[1000];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp),"%s",dir);
  len = strlen(tmp);
  if(tmp[len - 1] == '/')
    tmp[len - 1] = 0;
  for(p = tmp + 1; *p; p++)
    if(*p == '/') {
      *p = 0;
      mkdir(tmp, S_IRWXU);
      *p = '/';
    }
  mkdir(tmp, S_IRWXU);
}

/* Set's up a shared memory location for the python files
   to pull their information from */
void file_receive(simple_rec_ptr event){

  if(event) {
    log_info("file_path %s", event->file_path);
    log_info("file_buf_len %ld", event->file_buf_len);
  }
  
  // FIXME:Need to make a more dynamic way of doing this, rather than hard
  // coding this in the future.
  std::string filepath (event->file_path);
  //49 is the length of the server name, the path name up to "data"
  //10 is the length of the im7 file name
  std::string dir_name = filepath.substr(49, strlen(event->file_path) - 59);
  std::string file_name = "/dev/shm/" + filepath.substr(49, strlen(event->file_path) - 49);

  log_info("Dir name %s", dir_name.c_str());

  _mkdir(("/dev/shm/" + dir_name).c_str());

  if(event->file_buf != NULL) {

    FILE* pFile = fopen(file_name.c_str(), "wb");

    if (pFile){
      size_t w = fwrite(event->file_buf, 1, event->file_buf_len, pFile);
      log_info("Wrote to file %zu", w );
      fsync(fileno(pFile));
      fclose(pFile);
    }
    else{
      log_err("Something wrong writing to file");
    }
  }
  char * database_id = event->db_id;
  // Process attribute for python
  process_python_code(script_name, method_name, file_name, database_id);
  unlink(file_name.c_str());

}

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

    file_receive(event);
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

    servers.push_back("nothing");
    server_ids.push_back("nothing");

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
            if(!config_read_script_name(cfg_slave, *I, script_name))
            {
                log_err("Could not read script file name for %s", (*I).c_str());
                exit(1);
            }

            if(!config_read_method_name(cfg_slave, *I, method_name))
            {
                log_err("Could not read method name for %s", (*I).c_str());
                exit(1);
            }
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


    /* Set the python to the path for later processing */
    Py_SetProgramName(argv[2]);  /* optional but recommended */
    Py_Initialize();
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    char cwd[1024];
    std::string py = "/PyScripts";
    std::string pyscripts_path = strdup(getcwd(cwd, sizeof(cwd))) + py;
    PyList_Append(path, PyString_FromString(pyscripts_path.c_str()));
    PySys_SetObject("path", path);

    /*Set up connection to redis */
    std::string hostname = "localhost";
    int port = 6379;
    init_database_from_client(hostname, port);

	if (EVclient_ready_wait(test_client) != 1) {
	/* dfg initialization failed! */
	exit(1);
    }

    

    /*! [Shutdown code] */
	CMrun_network(cm);

	return 1;

}

