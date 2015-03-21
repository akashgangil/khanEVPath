#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <glob.h>
#include <set>
#include <unistd.h>

#include "dfg_functions.h"
#include "cfgparser.h"
#include "log.h"
#include "khan_ffs.h"
#include "measurements.h"
#include "readConfig.h"

struct timeval start,end;
extern struct dfg_unit test_dfg;

std::vector<stone_struct> stone_holder;

void usage()
{
  printf("Usage:\n ./dfg_master configfile \n");
}



void JoinHandlerFunc(EVmaster master, char * identifier, void * cur_unused1, void * cur_unused2)
{
    /*This code does not handle code in the stones yet */
    //TODO: Make it so we can have stones actually do stuff
    static int num_of_nodes = 0;
    ++num_of_nodes;
    if(num_of_nodes < test_dfg.node_count)
    {
        printf("Received node %s\n", identifier);
        EVmaster_assign_canonical_name(master, identifier, identifier);
        return;
    }
    printf("Received node %s\n", identifier);
    EVmaster_assign_canonical_name(master, identifier, identifier);
    EVdfg_stone *stones = (EVdfg_stone*)malloc(sizeof(EVdfg_stone) * stone_holder.size());
    test_dfg.dfg = EVdfg_create(test_dfg.dfg_master);

    for(unsigned i = 0; i < stone_holder.size(); ++i)
    {
        /*Getting the stones correctly set up when they are not sources and sinks will go here*/

    }

    for(unsigned i = 0; i < stone_holder.size(); ++i)
    {
        stones[i] = create_stone(stone_holder[i]); 
        if(!stones[i])
        {
            fprintf(stderr, "Error: stone not created in handler function\n");
            exit(1);
        }
        char * p = strdup(stone_holder[i].node_name.c_str());
        EVdfg_assign_node(stones[i], p);
        free(p);

    }
    

    for(unsigned i = 0; i < stone_holder.size(); ++i)
    {
        for(unsigned j = 0; j < stone_holder[i].incoming_stones.size(); ++j)
        {
            std::string temp_string1 = stone_holder[i].incoming_stones[j];
            for(unsigned k = 0; k < stone_holder.size(); ++k)
            {
                if(!temp_string1.compare(stone_holder[k].stone_name))
                {
                    EVdfg_link_dest(stones[k], stones[i]);
                    break;
                }
            }
        }
    }
    // Realize the dfg hopefully
    EVdfg_realize(test_dfg.dfg);

}

int main(int argc, char *argv[])
{
  /*
   * 		We want the dfg creator file to be able to read in a list of nodes, a list of stones per node and links between stones.
   * 	*/
  if(argc != 2)
    usage();
  else
  {
    std::string config_file_name = argv[1]; 

    ConfigParser_t cfg;
    if(cfg.readFile(config_file_name))
    {
      printf("Error: Cannot open config file %s", config_file_name.c_str());
      return 1;
    }
  
    /*Read the file information into the appropriate data structures for later use*/
    std::vector<std::string> stone_sections = cfg.getSections();
    std::vector<std::string> temp_node_name_list;

    for(std::vector<std::string>::iterator I = stone_sections.begin(), E = stone_sections.end(); I != E; ++I)
    {
      stone_struct new_stone_struct;

      /*Get the node name for the stone and in doing so, 
        bookkeep which node names that we have already seen*/
      std::string which_node;
      if(!config_read_node(cfg, *I, which_node))
      {
          fprintf(stderr, "Failure to read node from config\n");
          exit(1);
      }

      unsigned int temp_i = 0;
      for(; temp_i < temp_node_name_list.size(); ++temp_i)
      {
        if(!which_node.compare(temp_node_name_list[temp_i]))
          break;
      }
      if(temp_i == temp_node_name_list.size())
      {
        temp_node_name_list.push_back(which_node);
        ++test_dfg.node_count;
      }
      new_stone_struct.node_name = which_node;

      /*Store the section name as the stone name
        This must be a unique value for every stone*/
      new_stone_struct.stone_name = *I;

      /*Construct a unique handler for each source and sink node
        For now the nodes will have to read the config file*/
      std::string unique_handler_name = new_stone_struct.stone_name + "_" + new_stone_struct.node_name;
      new_stone_struct.src_sink_handler_name = unique_handler_name;
    
      /*Read the stone type into the enum value...*/
      if(!config_read_type(cfg, *I, new_stone_struct.stone_type))
      {
        fprintf(stderr, "Error: reading config type of stone failed\n");
        exit(1);
      }
    
    
      /*Read the incoming stones to connect to from the config file*/
      if(!config_read_incoming(cfg, *I, new_stone_struct.incoming_stones))
      {
        log_err("Error reading incoming stones");
        exit(1);
      }

      /*Read the code type, either Python or Cod*/ 
      if(!config_read_code_type(cfg, *I, new_stone_struct.code_type))
      {
        log_err("Error reading code type");
        exit(1);
      }

      /*Read the actual code, probably going to need to 
        have a seperate code file for the COD code*/
      if(!config_read_code(cfg, new_stone_struct))
      {
        log_err("Error reading the actual code somehow");
        exit(1);
      }

      //Push-back here
      stone_holder.push_back(new_stone_struct);

      /*  
      printf("Testing: node_name := %s\n", new_stone_struct.node_name.c_str());
      printf("Testing: stone_name := %s\n", new_stone_struct.stone_name.c_str());
      printf("Testing: handler_name := %s\n", new_stone_struct.src_sink_handler_name.c_str());
      printf("Testing: stone_type := %d\n", new_stone_struct.stone_type);
      printf("Testing: node_count := %d\n", test_dfg.node_count);
      for(unsigned int i = 0; i < new_stone_struct.incoming_stones.size(); ++i)
      {
          printf("Testing: incoming_stone for %s: %s\n", new_stone_struct.stone_name.c_str(),
                                                          new_stone_struct.incoming_stones[i].c_str());
      }
      printf("Testing: code_type := %d\n", new_stone_struct.code_type);
      */ 
    

    }

    /*TODO: Add in a master node default source stone known as entry here
    */
    stone_struct entry_stone_struct;
    entry_stone_struct.node_name = "master_node";
    entry_stone_struct.stone_name = "entry";
    entry_stone_struct.src_sink_handler_name = entry_stone_struct.stone_name + "_" + entry_stone_struct.node_name;
    entry_stone_struct.stone_type = SOURCE;
    entry_stone_struct.code_type = COD;
    stone_holder.push_back(entry_stone_struct);

    ++test_dfg.node_count;

    if(dfg_init_func())
    {
      EVmaster_node_join_handler(test_dfg.dfg_master, JoinHandlerFunc);
      printf("DFG handler read...\n");
    }

    
    EVsource source_handle;
    EVclient master_client;
    EVclient_sources source_capabilities;

    std::vector < std::string > servers;
    std::vector < std::string > server_ids;

    std::string this_server;
    std::string this_server_id;


    source_handle = EVcreate_submit_handle(test_dfg.cm, -1, simple_format_list);
    char * temp_char_ptr = strdup(entry_stone_struct.src_sink_handler_name.c_str());
    source_capabilities = EVclient_register_source(temp_char_ptr, source_handle);
    temp_char_ptr = NULL;

    master_client = EVclient_assoc_local(test_dfg.cm, "master_node", test_dfg.dfg_master, source_capabilities, NULL);

    if (EVclient_ready_wait(master_client) != 1) {
      /* initialization failed! */
      exit(1);
    }

  
    simple_rec data;

    std::string store_filename="stores.txt";

    /*int opt;
    while((opt = getopt(argc, argv, "s:")) != -1){
      switch(opt){ 
        case 's':
          store_filename = optarg;
          break;

      }
     }*/

    FILE* stores = fopen(store_filename.c_str(), "r");
    char buffer[100];
    char buffer2[100];
    fscanf(stores, "%s\n", buffer);
    this_server_id = buffer;
    while(fscanf(stores, "%s %s\n", buffer, buffer2)!=EOF) {
      if(strcmp(buffer,"cloud")==0) {
        std::string module = buffer2;
        module = "cloud." + module;
        //cloud_interface = PyImport_ImportModule(module.c_str());
      }   
      servers.push_back(buffer);
      server_ids.push_back(buffer2);
      if(this_server_id == buffer2) {
        this_server = buffer;
      }   
    }
    fclose(stores);

    std::string pattern = servers[0] + "/*";
    glob_t files;

    int fdin;
    struct stat statbuf;
    std::set<std::string> experiments;

    for(int count = 18; count > 0; count--) {

      glob((pattern +".im7").c_str(), 0, NULL, &files);        

      log_info("Globbing with pattern: %s.im7", pattern.c_str());

      for(unsigned j=0; j<files.gl_pathc; j++) {
        std::string filepath = files.gl_pathv[j];
        log_info("File Path: %s", filepath.c_str());

        std::string exp_dir = filepath.substr(0, filepath.size() - 10);
        experiments.insert(exp_dir);
        data.exp_id = (int)experiments.size();

        data.file_path = strdup(filepath.c_str());

        if ((fdin = open (filepath.c_str(), O_RDONLY)) < 0)
          perror ("can't open %s for reading"); 

        if (fstat (fdin,&statbuf) < 0)
          perror ("fstat error");

        data.file_buf_len = statbuf.st_size;

        log_info("Size: %zu", data.file_buf_len);

        if ((data.file_buf = (char*)mmap (0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fdin, 0))
            == (caddr_t) -1)
          perror ("mmap error for input");
      
        EVsubmit(source_handle, &data, NULL);

        if (munmap(data.file_buf, statbuf.st_size) == -1) {
          perror("Error un-mmapping the file");
        }

        /*Free Memory*/
        close(fdin);
        free(data.file_path);
      }
      pattern += "/*";
    }
    log_info("Cleanup data structures");
    /* Cleanup */
    globfree(&files);

    log_info("Shutdown evdfg");
  

    CMrun_network(test_dfg.cm);     
  }  
        
  return 0;
}
