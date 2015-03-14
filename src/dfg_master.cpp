#include "dfg_functions.h"
#include "cfgparser.h"
#include "log.h"

#define CONFIG_FILE "src/mytest.cfg"

struct timeval start,end;
extern struct dfg_unit test_dfg;

std::vector<stone_struct> stone_holder;

extern int config_read_node(ConfigParser_t & cfg, std::string stone_section, std::string & node_name);
extern int config_read_type(ConfigParser_t & cfg, std::string stone_section, stone_type_t & what_type);
extern int config_read_incoming(ConfigParser_t & cfg, std::string stone_section, std::vector<std::string> & incoming_list);
extern int config_read_code_type(ConfigParser_t & cfg, std::string stone_section, code_type_t & code_type);
extern int config_read_code(ConfigParser_t & cfg, stone_struct & stone);

void usage()
{
  printf("Usage:\n ./dfg_master configfile \"dynamic/static\"\n where dynamic or static is the type of dfg you want to create\n");
}



void JoinHandlerFunc(EVmaster master, char * identifier, void * cur_unused1, void * cur_unused2)
{
    /*This code does not handle code in the stones yet */
    //TODO: Make it so we can have stones actually do stuff
    static int num_of_nodes = 0;
    ++num_of_nodes;
    if(num_of_nodes < (test_dfg.node_count - 1))
    {
        printf("Received node %s\n", identifier);
        EVmaster_assign_canonical_name(master, identifier, identifier);
        return;
    }
    printf("Received node %s\n", identifier);
    EVmaster_assign_canonical_name(master, identifier, identifier);
    EVdfg_stone *stones = (EVdfg_stone*)malloc(sizeof(EVdfg_stone) * stone_holder.size());

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

  ConfigParser_t cfg;
  if(cfg.readFile(CONFIG_FILE))
  {
    printf("Error: Cannot open config file %s", CONFIG_FILE);
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
     
    

  }
  exit(0);
  
  if(argc != 3)
    usage();
  else
  {
    /*Parse input node file */
    gettimeofday(&start,NULL);
    char *nodes[MAXNODES];
    int node_count,i;
    FILE *nodefile;
    nodefile = fopen(argv[1],"r");
    fscanf(nodefile, "%d\n",&node_count);
    if(node_count > 0)
    {
      for(i=0; i<node_count; ++i)
        nodes[i]=(char*)malloc(15);
      for(i=0; i < node_count; ++i)
        fscanf(nodefile,"%s\n",nodes[i]);

      /*Initiate dfg*/
      if(dfg_init_func())
      {
        if(strcmp(argv[4], "static") == 0)
        {
            if(dfg_create_func("static", node_count, nodes, NULL) == 1)
            {
                /*Parse Source stones*/
                FILE *sourcefile;
                sourcefile = fopen(argv[2],"r");
                char token[15];
                char* node, *source;
                while(fscanf(sourcefile,"%s",token)!=EOF)
                {
                    node=strdup((const char*)token);
                    fscanf(sourcefile," %s",token);
                    while(strcmp(token,"src_end")!=0 && token!=NULL)
                    {
                        source=strdup((const char*)token);
                        if(dfg_create_assign_source_stones_func(node,source));
                        else	{
                            printf("Source stones not created.\n");
                            fclose(sourcefile);
                            exit(0);
                        }
                    fscanf(sourcefile," %s",token);
                    }
                }
                fclose(sourcefile);
                /*Parse sinks and links*/
                FILE *linksfile;
                linksfile = fopen(argv[3],"r");
                char *sink;
                int numsources;
                while(fscanf(linksfile,"%s", token)!=EOF)
                {
                    node=strdup((const char *)token);
                    fscanf(linksfile," %s", token);
                    sink =strdup((const char*) token);
                    fscanf(linksfile, " %d",&numsources);
                    char *sources[numsources+1];

                    for(i=0; i<numsources; ++i)
                        sources[i]=(char*)malloc(15);

                    for(i=0;i<numsources;++i)
                        fscanf(linksfile," %s",sources[i]);

                    sources[numsources]=NULL;
                    if(dfg_create_assign_link_sink_stones_func(node,sink,numsources,sources));
                    else {
                        printf("Sink stones not created.\n");
                        fclose(linksfile);
                        exit(0);
                    }
                    for(i=0; i<numsources; ++i)
                        free(sources[i]);
                }
                fclose(linksfile);
                /*Finalize DFG*/
                char mastercontact[1024];
                dfg_get_master_contact_func(&mastercontact[0], "master.info");
                printf("Master: %s\n",mastercontact);
                gettimeofday(&end,NULL);
                printf("DFG time taken = %ld usec\n",(end.tv_usec + (end.tv_sec * 1000000)) - (start.tv_usec + (start.tv_sec * 1000000)));

                if(!dfg_finalize_func_static())
                    fprintf(stderr,"DFG not created\n");

            }
            else
                printf("DFG not created. Create function failure.\n");

        }
        else if(strcmp(argv[4], "dynamic"))
        {
            if(dfg_create_func("dynamic", node_count, nodes, JoinHandlerFunc) == 2) //adds nodes to dfg_test and registers Function
            {
                printf("DFG handler ready...\n");
            }
            else
                printf("DFG not created. Create function failure.\n");

        }
      }
      else
          printf("DFG not created. Init function failure\n");

    }
    else
      printf("Invalid number of nodes\n");
    fclose(nodefile);

  }
  return 0;
}
