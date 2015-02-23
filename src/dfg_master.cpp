#include "dfg_functions.h"

struct timeval start,end;


void usage()
{
  printf("Usage:\n ./dfg_master nodesfile sourcestonesfile linksfile\n where nodesfile is a line separated list of nodes with the first line specifying the number of nodes, \n and sourcestonesfile is a file that lists the stones  as \"nodename sources src_end\" \n and linksfile is a file that specifies the links between the stones as \"sinknode sink source(s)\"\n");
}

int main(int argc, char *argv[])
{
  /*
   * 		We want the dfg creator file to be able to read in a list of nodes, a list of stones per node and links between stones.
   * 	*/
  if(argc != 4)
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
        if(dfg_create_func("static", node_count, nodes, NULL))
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

          if(!dfg_finalize_func())
            fprintf(stderr,"DFG not created\n");

        }
        else
          printf("DFG not created. Create fuction failure.\n");
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
