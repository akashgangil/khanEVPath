#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <vector>
#include <set>
#include <signal.h>

#include <fcntl.h>
#include <glob.h>

#include "dfg_functions.h"
#include "evpath.h"
#include "ev_dfg.h"
#include "log.h"
#include "khan_ffs.h"
#include "measurements.h"

std::vector < std::string > servers;
std::vector < std::string > server_ids;

std::string this_server;
std::string this_server_id;

extern FMField simple_field_list[];
extern FMStructDescRec simple_format_list[];

static void cleanup_handler(int dummy, siginfo_t *siginfo, void* context){
  log_info("Cleanuphandler called\n");
  exit(1);
}

int main(int argc, char **argv)
{
  struct sigaction act;
  memset (&act, '\0', sizeof(act));
  act.sa_sigaction = &cleanup_handler;
  act.sa_flags = 0;
 
  if (sigaction(SIGINT, &act, NULL) < 0) {
    perror ("sigaction");
    return 1;
  }
 
  CManager cm;
  EVsource source_handle;
  EVclient test_client;
  EVclient_sources source_capabilities;

  (void)argc; (void)argv;
  cm = CManager_create();

  char master_address[200];
  dfg_get_master_contact_func(master_address,"master.info");

  char source_node[300] = "src_";
  strcat(source_node, argv[1]);

  source_handle = EVcreate_submit_handle(cm, -1, simple_format_list);
  source_capabilities = EVclient_register_source(source_node, source_handle);
  
  test_client = EVclient_assoc(cm, argv[1], master_address, source_capabilities, NULL);

  if (EVclient_ready_wait(test_client) != 1) {
    /* initialization failed! */
    exit(1);
  }

  

  //if (EVclient_source_active(source_handle)) {
  //}

  simple_rec data;

  std::string store_filename="stores.txt";

  int opt;
  while((opt = getopt(argc, argv, "s:")) != -1){
    switch(opt){ 
      case 's':
        store_filename = optarg;
        break;

    }
  }

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
  
  /*e [Shutdown code] */
  if (EVclient_active_sink_count(test_client) > 0) {
    /* if there are active sinks, the handler will call EVclient_shutdown() */
  } else {
    if (EVclient_source_active(source_handle)) {
      /* we had a source and have already submitted, indicate success */
      EVclient_shutdown(test_client, 0 /* success */);
    } else {
      /* we had neither a source or sink, ready to shutdown, no opinion */
      EVclient_ready_for_shutdown(test_client);
    }
  }
  return(EVclient_wait_for_shutdown(test_client));  
  /*! [Shutdown code] */
}


