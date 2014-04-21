#include "Python.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#include <boost/log/trivial.hpp>
#include <boost/thread.hpp>
#include <boost/log/utility/setup/file.hpp>

#include "fileprocessor.h"
#include "database.h"
#include "params.h"
#include "threadpool.h"
#include "evpath.h"
#include "fuseapi.h"
#include "khan.h" 

extern struct fuse_operations khan_ops;

std::vector < std::string > servers;
std::vector < std::string > server_ids;

std::string this_server;
std::string this_server_id;

threadpool_t* t_p;
CManager cm;

typedef struct _simple_rec {
  char* file_path;
  long file_buf_len;
  char* file_buf;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
  {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
  {"file_buf_len", "integer", sizeof(long), FMOffset(simple_rec_ptr, file_buf_len)},
  {"file_buf", "char[file_buf_len]", sizeof(char), FMOffset(simple_rec_ptr, file_buf)},
  {NULL, NULL, 0, 0}
};

static FMStructDescRec simple_format_list[] =
{
  {"simple", simple_field_list, sizeof(simple_rec), NULL},
  {NULL, NULL}
};

void file_receive(void *vevent){

  simple_rec_ptr event = (_simple_rec*)vevent;
  if(event) printf("[THREADING ]I  got %s\n", event->file_path);

  std::string filepath (event->file_path);
  //24 is the length of the server name
  //10 is the length of the im7 file name
  std::string dir_name = filepath.substr(24, strlen(event->file_path) - 34);
  std::string file_name = filepath.substr(24, strlen(event->file_path) - 24);

  FILE* stream=popen(("mkdir -p \"" + dir_name + "\"").c_str(),"r");
  fclose(stream);

  if(event->file_buf != NULL) {

    int pFile = open(file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC , 0776);

    if (pFile){
      size_t w = write(pFile, event->file_buf, event->file_buf_len);
      fsync(pFile);
      printf("Wrote to file! %zu\n", w);
    }
    else{
      printf("Something wrong writing to File.");
    }
    close(pFile);
  }

  extract_attr_init(filepath);

  printf("Return event buffer\n");
  EVreturn_event_buffer(cm, vevent);
}

static int simple_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
  EVtake_event_buffer(cm , vevent);
  threadpool_add(t_p, &file_receive, vevent, 0);    
  return 1;
}

static void cleanupHandler(int dummy=0){
  BOOST_LOG_TRIVIAL(info) << "Cleanup Called";
  threadpool_destroy(t_p, 0);
  exit(0);
}

void log_init() {
    boost::log::add_file_log("sample.log");

    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= boost::log::trivial::info
    );
    boost::log::keywords::auto_flush = true; 
}

int main(int argc, char **argv)
{

  //log_init();

  EVstone stone;
  char *string_list;
  int forked = 0;

  cm = CManager_create();
  forked = CMfork_comm_thread(cm);
  assert(forked == 1);
  if (forked) {
    printf("Forked a communication thread\n");
  } else {
    printf("Doing non-threaded communication handling\n");
  }
  CMlisten(cm);
  
  stone = EValloc_stone(cm);
  EVassoc_terminal_action(cm, stone, simple_format_list, simple_handler, NULL);
  string_list = attr_list_to_string(CMget_contact_list(cm));
  
  BOOST_LOG_TRIVIAL(info) << "Contact List: " << stone << ":" << string_list;

  signal(SIGINT, cleanupHandler);

  t_p = threadpool_create( 1, 1000, 0);    

  Py_SetProgramName(argv[0]);  /* optional but recommended */
  Py_Initialize();

  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append(\"/net/hu21/agangil3/KhanScripts\")");

  xmp_initialize();

  struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
  int j;
  const char* store_filename="/net/hu21/agangil3/MediaKhan/stores.txt";

  for(j = 0; j < argc; j++) {
    if((j == 2) && (argv[j][0]!='-')) {
      store_filename = argv[j];
    } else {
      fuse_opt_add_arg(&args, argv[j]);
    }
  }

  //set signal handler
  signal(SIGTERM, khan_terminate);
  signal(SIGKILL, khan_terminate);


  BOOST_LOG_TRIVIAL(debug) << "Store filename: " << store_filename;

  FILE* stores = fopen(store_filename, "r");
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

  umask(0);

  khan_data = (khan_state*)calloc(sizeof(struct khan_state), 1);
  if (khan_data == NULL)  {
    BOOST_LOG_TRIVIAL(fatal) << "Could not allocate memory to khan_data.. Aborting";
    abort();
  }

  boost::thread khan_init_thread(initializing_khan, argv[1], servers, server_ids);

  BOOST_LOG_TRIVIAL(info) << "Initialized Khan";
  
  fuse_main(args.argc,args.argv, &khan_ops, khan_data);

  BOOST_LOG_TRIVIAL(info) << "Fuse Running";
  
  return 0;
}
