#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include "evpath.h"
#include "ev_dfg.h"
#include "dfg_functions.h"
#include "khan_ffs.h"
#include "khan.h"
#include "readConfig.h"
#include "Python.h"
#include "fileprocessor.h"

#define MAX_SOURCES 10

typedef struct _simple_stone_holder
{
    std::string name;
    std::string py_file;
    std::string py_method;
    std::vector<std::string> incoming;

} simp_stone, * simp_stone_ptr;


EVclient test_client;
EVsource source_handles[MAX_SOURCES];
EVsource activate_source_handle;
std::string script_name[MAX_SOURCES];
std::string method_name[MAX_SOURCES];
std::string sink_handlers[MAX_SOURCES];
std::vector < std::string > servers;
std::vector < std::string > server_ids;
std::vector<std::pair<std::string, std::string> > python_master_list;

std::vector<simp_stone> ordered_stone_info;

int for_comparison;
int old_comparison;
int current_value;


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
  process_python_code(ordered_stone_info[old_comparison].py_file, ordered_stone_info[old_comparison].py_method, file_name, database_id);
  unlink(file_name.c_str());

}

static int 
python_general_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
    log_info("Received a python event");
    simple_rec_ptr event = (_simple_rec*)vevent;

    int meta_count = event->meta_compare_py;
    activate_source_handle = source_handles[meta_count]; 
    
    

    //current_value = event->meta_compare_py;
    /* FIXME: Maybe this is fixed, but wait until it runs....*/ 
    if(EVclient_source_active(activate_source_handle))
    {
        if(event->meta_compare_py == (for_comparison - 1))
        {
            event->meta_compare_py = 0;
            log_info("For_Comparison check just rolled back to 0...");
        }
        else
            event->meta_compare_py = event->meta_compare_py + 1;
        
        printf("Sending the message on...\n");
        printf("The current value of for_comparison is: %d\n", for_comparison);
        //Potentially need to do some fancy footwork with for_comparison here to 
        //prevent a subtle data race?


        EVsubmit(activate_source_handle, event, NULL);
    }
    else
        log_info("No active client source registered");

    old_comparison = meta_count;
    file_receive(event);


    return 1;
}

static int
python_list_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
    printf("Received the event\n");
    python_list_ptr event = (python_list_ptr)vevent;
    for(int i = 0; i < 4; ++i)
        printf("The int is: %d\n", event->ordered_method_list[i]);

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

    //for_comparison = 0;
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
    unsigned int num_py_stones = 0;
    // I use this to get the correct order of things
    std::vector<simp_stone> stone_info;

    for(std::vector<std::string>::iterator I = stone_names.begin(), E = stone_names.end(); I != E; ++I)
    {
        log_info("Stone: %s", I->c_str());
        //continue;
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
            simp_stone temp_python_stone;
            // Need the python stone to turn into two stone names *I + "py_src"
            // This check may be buggy...as it's max_sources not max_pys
            if(num_py_stones > MAX_SOURCES)
            {
                log_err("Maximum number of python stones per node is: %d", MAX_SOURCES);
                exit(1);
            }

            std::string temp_handler_name_sink = (*I) + "_py_sink_" + temp_node_name;
            std::string temp_handler_name_src = (*I) + "_py_src_" + temp_node_name;
            //temp_handler_name_sink = sstrm.str();

            //std::string temp_handler_name_sink = (*I) + "_py_sink_" + temp_node_name;
            stone_types.push_back(PYTHON);
            //stone_types.push_back(SOURCE);

            handler_names.push_back(temp_handler_name_sink);
            //handler_names.push_back(temp_handler_name_src);

            temp_python_stone.name = (*I);

            if(!config_read_script_name(cfg_slave, *I, temp_python_stone.py_file))
            {
                log_err("Could not read script file name for %s", (*I).c_str());
                exit(1);
            }

            if(!config_read_method_name(cfg_slave, *I, temp_python_stone.py_method))
            {
                log_err("Could not read method name for %s", (*I).c_str());
                exit(1);
            }

            if(!config_read_incoming(cfg_slave, *I, temp_python_stone.incoming))
            {
                log_err("Could not read incoming stones for %s", (*I).c_str());
                exit(1);
            }

            stone_info.push_back(temp_python_stone);
            /*std::stringstream sstrm;
            sstrm << (*I) + "_py_src_" << method_name[num_py_stones] << "_" + temp_node_name;
            std::string temp_handler_name_src = sstrm.str();

            stone_types.push_back(SOURCE);
            handler_names.push_back(temp_handler_name_src);
            */
            

            ++num_py_stones;
        }
        else
        {
            temp_handler_name = (*I) + "_" + temp_node_name;
            handler_names.push_back(temp_handler_name);
            stone_types.push_back(temp_type);
        }
    } 

    //This code is hacky and simplified.  Basically, I make two assumptions. 1) Each python stone only has one 
    //incoming stone and 2) All of the python stones are together in a line
    std::vector<simp_stone>::iterator top_iter_i = stone_info.begin(), top_iter_e = stone_info.end();
    for(; top_iter_i != top_iter_e; ++top_iter_i)
    {
        std::vector<std::string>::iterator II = (*top_iter_i).incoming.begin(), EE = (*top_iter_i).incoming.end();
        for(; II != EE; ++II)
        {
            std::vector<simp_stone>::iterator III = stone_info.begin(), EEE = stone_info.end();
            for(; III != EEE; ++III)
            {
                //printf("The two compared values:\t%s\t:\t%s\n", (*II).c_str(), (*III).name.c_str());
                if(!(*II).compare((*III).name))
                {
                    //printf("Hit the break!\n");
                    break;
                }
            }
            if(III == EEE)
                break;
        }
        //Here we have found someone in our group and therefore must continue
        if(II == EE)
            continue;
        else
            break;
    }
    if(top_iter_i == top_iter_e)
    {
        log_err("ERROR: The python group must be circular");
        exit(1);
    }
    ordered_stone_info.push_back(*top_iter_i);

    int watch_for_inf = 100000;
    while((ordered_stone_info.size() < stone_info.size()) && (watch_for_inf > 0))
    {
        --watch_for_inf;
        for(std::vector<simp_stone>::iterator I = stone_info.begin(), E = stone_info.end(); I != E; ++I)
        {
            if(!((*I).incoming[0].compare(ordered_stone_info.back().name)))
            {
                ordered_stone_info.push_back(*I);
                watch_for_inf = 100;
            }

        }
    }

    if(ordered_stone_info.size() != stone_info.size())
    {
        log_err("ERROR: It looped infinitely, or there were a lot of elements...");
        exit(1);
    }

    for(unsigned int i = 0; i < ordered_stone_info.size(); ++i)
    {
        std::string temp_py_src_name = ordered_stone_info[i].name + "_py_src_" + client_node_name;
        handler_names.push_back(temp_py_src_name);
        stone_types.push_back(SOURCE);
    }
    for_comparison = ordered_stone_info.size();
            
    /*for(std::vector<simp_stone>::iterator I = ordered_stone_info.begin(), E = ordered_stone_info.end(); I != E; ++I)
    {
        printf("The name of pystone: %s\n", (*I).name.c_str());
        printf("List of incoming stones: ");
        for(std::vector<std::string>::iterator II = (*I).incoming.begin(), EE = (*I).incoming.end(); II != EE; ++II)
            printf("%s,\t", (*II).c_str());
        printf("\n");
    }

    for(std::vector<simp_stone>::iterator I = stone_info.begin(), E = stone_info.end(); I != E; ++I)
    {
        printf("The name of pystone: %s\n", (*I).name.c_str());
        printf("List of incoming stones: ");
        for(std::vector<std::string>::iterator II = (*I).incoming.begin(), EE = (*I).incoming.end(); II != EE; ++II)
            printf("%s,\t", (*II).c_str());
        printf("\n");
    }
    exit(1);
    for(int i = 0; script_name[i].compare("") && i < MAX_SOURCES; ++i)
        printf("The value of script_name %d: %s\n", i, script_name[i].c_str());

    for(int i = 0; method_name[i].compare("") && i < MAX_SOURCES; ++i)
        printf("The value of method_name %d: %s\n", i, method_name[i].c_str());

    for(std::vector<std::string>::iterator I = handler_names.begin(), E = handler_names.end(); I != E; ++I)
        printf("The value of a handler name: %s\n", (*I).c_str());
    */
    //exit(0);

    if(num_of_sources > 0)
    {
       // source_handles = (EVsource *) malloc(sizeof(EVsource) * num_of_sources);
        num_of_sources = 0;
    }
        
    /* This allows for a stone with only python stones on it, as long as they are below the stone limit
       This does not allow for multiple types of source stone (i.e python and source) at the same time */
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
    /*if(num_py_stones > 0)
    {
        std::string temp_list_handler_name = "python_list_" + client_node_name;
        char * perm_ptr = strdup(temp_list_handler_name.c_str());
        sink_capabilities = EVclient_register_sink_handler(cm, perm_ptr, python_format_list,
                                                                (EVSimpleHandlerFunc) python_list_handler, NULL);
    }*/

            
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

