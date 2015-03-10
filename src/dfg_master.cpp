#include "dfg_functions.h"
#include "cfgparser.h"

#define CONFIG_FILE "test1.cfg"

struct timeval start,end;
extern struct dfg_unit test_dfg;

std::vector<stone_struct> stone_holder;

void usage()
{
  printf("Usage:\n ./dfg_master configfile \"dynamic/static\"\n where dynamic or static is the type of dfg you want to create\n");
}



void JoinHandlerFunc(EVmaster master, char * identifier, void * cur_unused1, void * cur_unused2)
{
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
