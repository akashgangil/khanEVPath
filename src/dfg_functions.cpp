#include "dfg_functions.h"
#include "khan_ffs.h"

struct dfg_unit test_dfg;

extern FMField simple_field_list[];
extern FMStructDescRec simple_format_list[];

static char *router_function = "\
{\n\
  static int count = 0;\n\
    return (count++) % 2;\n\
}\0\0";

char *router_action;

int dfg_init_func(void)
{
  test_dfg.cm=CManager_create();
  CMlisten(test_dfg.cm);
  test_dfg.dfg_master = EVmaster_create(test_dfg.cm);
  test_dfg.str_contact = EVmaster_get_contact_list(test_dfg.dfg_master);
  FILE *op;
  op = fopen("master.info","w");
  fprintf(op,"%s",test_dfg.str_contact);
  fclose(op);
  router_action = create_router_action_spec(simple_format_list, router_function);
  //Router not necessary at this point....
  return 1;
}

int dfg_create_func(char *mode, int ncount, char **nodelist, EVmasterJoinHandlerFunc func)
{
  //I'm going to hardcode in my storage stone stuff now, but ideally that needs to change

  int ret=0,i;
  if(test_dfg.dfg_master)
  {
    if(strcmp(mode,"static")==0)
    {
      if(ncount>0)
      {
        test_dfg.node_count = ncount;
        for (i=0; i <= test_dfg.node_count; i++) 
          test_dfg.nodes[i] = (char*)malloc(15);
        //NOTE--This makes the max length of a name 15 with no overspill check, also in dfg_master.cpp
        test_dfg.nodes[0]="masternode";
        for(i=1;i<=ncount;i++)
          test_dfg.nodes[i]=nodelist[i-1];
        test_dfg.nodes[test_dfg.node_count+1]=NULL;
        test_dfg.srcstone = (source_stone_unit*)malloc(sizeof(ss_unit) * MAXSTONES); // I changed this sketchy code 
        test_dfg.numsourcestones=0;
        EVmaster_register_node_list(test_dfg.dfg_master,&test_dfg.nodes[0]);
        test_dfg.dfg = EVdfg_create(test_dfg.dfg_master);
        ret = 1;
      }
      else
        fprintf(stderr,"Nodelist is empty; failed to create DFG in static mode\n");

    }
    else if(strcmp(mode,"dynamic")==0)
    {
      if(func)
      {
        EVmaster_node_join_handler(test_dfg.dfg_master,func);
        test_dfg.dfg = EVdfg_create(test_dfg.dfg_master);
        ret = 1;
      }
      else
        fprintf(stderr,"Node Join Handler Function not valid \n");
    }
  }
  else
    fprintf(stderr,"DFG not initialized correctly, call dfg_init_func first\n");
  return ret;
}

int dfg_create_assign_source_stones_func(char *nodename, char *sourcestone)
{
  int ret = 0, i;

  assert(test_dfg.dfg);

  if(sourcestone!=NULL)
  {
    for(i=0; strcmp(test_dfg.nodes[i],nodename)!=0 && i<=test_dfg.node_count; ++i);
    if(i<=test_dfg.node_count) {
      test_dfg.srcstone[/*test_dfg.numsourcestones*/i].src = EVdfg_create_source_stone(test_dfg.dfg, sourcestone);
      EVdfg_assign_node(test_dfg.srcstone[/*test_dfg.numsourcestones*/i].src, nodename);

      // test_dfg.srcstone[test_dfg.numsourcestones].router = EVdfg_create_stone(test_dfg.dfg, router_action);
      //EVdfg_assign_node(test_dfg.srcstone[test_dfg.numsourcestones].router, nodename);

      test_dfg.srcstone[/*test_dfg.numsourcestones*/i].sourcename=sourcestone;
      test_dfg.numsourcestones++;

      //test_dfg.srcstone[test_dfg.numsourcestones].port = 0;

      ret = 1;
    }
    else
      fprintf(stderr,"Node isn't on the registered list\n");
  }
  return ret;
}

int dfg_create_assign_link_sink_stones_func(char *nodename, char *handler, int numsources, char **sourcename)
{
  static int once = 0;
  int ret = 0, i,j;
  assert(test_dfg.dfg);
  if(handler!=NULL)
  {
    EVdfg_stone sink;
    for(i=1; strcmp(test_dfg.nodes[i],nodename)!=0 && i<=test_dfg.node_count ; ++i);
    if(i<=test_dfg.node_count) {
      sink = EVdfg_create_sink_stone(test_dfg.dfg, handler);
      EVdfg_assign_node(sink, nodename);
      for(i=0;i<numsources;++i)
      {
        for(j=0; j<test_dfg.node_count;++j)
        {
            if(!test_dfg.srcstone[j].sourcename)
                 continue;
            if(strcmp(sourcename[i],test_dfg.srcstone[j].sourcename) == 0)
                 break;
        }
        if(j<test_dfg.node_count) {
/*          if(once == 0)
          {
              EVdfg_link_dest(test_dfg.srcstone[j].src, test_dfg.srcstone[j].router);
              once = 1;
          }
          EVdfg_link_port(test_dfg.srcstone[j].router, test_dfg.srcstone[j].port++, sink);
  */      
          EVdfg_link_dest(test_dfg.srcstone[j].src,sink);
          ret = 1;
        }
        else
          fprintf(stderr,"Source not registered. Failed to establish link. Call dfg_create_assign_source_stone_func first\n");
      }
    }
    else
      fprintf(stderr,"Node isn't on the registered list\n");
  }
  return ret;

}

int dfg_finalize_func(void)
{
  int ret = 0;
  if(test_dfg.dfg)
  {
    if(/*dfg_create_assign_source_stones_func(test_dfg.nodes[0],"master_source")*/1)
    {
      EVdfg_realize(test_dfg.dfg);
      test_dfg.test_client = EVclient_assoc_local(test_dfg.cm,test_dfg.nodes[0],test_dfg.dfg_master,NULL,NULL);
      EVclient_ready_wait(test_dfg.test_client);
      if(EVclient_active_sink_count(test_dfg.test_client)==0)
        EVclient_ready_for_shutdown(test_dfg.test_client);

      EVclient_wait_for_shutdown(test_dfg.test_client);

      ret = 1;
    }
    else
      fprintf(stderr,"Couldn't create a stone for master. Check if dfg_create_func has been called first\n");
  }
  else
    fprintf(stderr,"DFG not created correctly. Call dfg_init_func first\n");

  return ret;
}

void dfg_get_master_contact_func(char *retvalue, char* contact_file)
{
  FILE *op;
  op = fopen(contact_file,"r");
  if(op != NULL)
  {
    fscanf(op,"%s",retvalue);
    fclose(op);
  }
  else
  {
    fprintf(stderr, "Could not open master.info file\n");
  }
}

