#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <vector>
#include <set>

#include <fcntl.h>
#include <glob.h>

#include "evpath.h"
#include "ev_dfg.h"
#include "log.h"

std::vector < std::string > servers;
std::vector < std::string > server_ids;

std::string this_server;
std::string this_server_id;

typedef struct _simple_rec {
  char* file_path;
  long file_buf_len;
  char* file_buf;
  int exp_id;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
  {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
  {"file_buf_len", "integer", sizeof(long), FMOffset(simple_rec_ptr, file_buf_len)},
  {"file_buf", "char[file_buf_len]", sizeof(char), FMOffset(simple_rec_ptr, file_buf)},
  {"exp_id", "integer", sizeof(int), FMOffset(simple_rec_ptr, exp_id)},
  {NULL, NULL, 0, 0}
};

static FMStructDescRec simple_format_list[] =
{
  {"simple", simple_field_list, sizeof(simple_rec), NULL},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  CManager cm;
  EVsource source_handle;
  EVclient test_client;
  EVclient_sources source_capabilities;

  (void)argc; (void)argv;
  cm = CManager_create();

  source_handle = EVcreate_submit_handle(cm, -1, simple_format_list);
  source_capabilities = EVclient_register_source("event source", source_handle);

  test_client = EVclient_assoc(cm, "b", argv[1], source_capabilities, NULL);

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

  /* Cleanup */
  globfree(&files);

  CMrun_network(cm);
}


