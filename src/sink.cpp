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

#include "fileprocessor.h"
#include "database.h"
#include "params.h"
#include "threadpool.h"
#include "evpath.h"
#include "fuseapi.h"
#include "khan.h" 
#include "data_analytics.h"
#include "stopwatch.h"
#include "measurements.h"
extern struct fuse_operations khan_ops;
extern struct stopwatch_t* sw;

std::vector < std::string > servers;
std::vector < std::string > server_ids;

std::string this_server;
std::string this_server_id;

std::string mount_point;

struct khan_state* khan_data;

CManager cm;
EVstone stone;

char* string_list;
attr_list contact_list;
  
struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

typedef struct _simple_rec {
  int exp_id;
  char* file_path;
  long file_buf_len;
  char* file_buf;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
  {"exp_id", "integer", sizeof(int), FMOffset(simple_rec_ptr, exp_id)},
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


//void file_receive(void *vevent){
void file_receive(simple_rec_ptr event){
  //simple_rec_ptr event = (_simple_rec*)vevent;
  if(event) {
    BOOST_LOG_TRIVIAL(info) << "[THREADING ]file_path " << event->file_path;
    BOOST_LOG_TRIVIAL(info) << "[THREADING ]file_buf_len " << event->file_buf_len;
  }
  std::string filepath (event->file_path);
  //24 is the length of the server name
  //10 is the length of the im7 file name
  std::string dir_name = filepath.substr(24, strlen(event->file_path) - 34);
  std::string file_name = "/dev/shm/" + filepath.substr(24, strlen(event->file_path) - 24);

  BOOST_LOG_TRIVIAL(info) << "DIR_NAME " << dir_name;

  _mkdir(("/dev/shm/" + dir_name).c_str());

  if(event->file_buf != NULL) {

    FILE* pFile = fopen(file_name.c_str(), "wb");

    if (pFile){
      size_t w = fwrite(event->file_buf, 1, event->file_buf_len, pFile);
      BOOST_LOG_TRIVIAL(debug) << "Wrote to file! " << w;
      fsync(fileno(pFile));
      fclose(pFile);
    }
    else{
      BOOST_LOG_TRIVIAL(error) <<  "Something wrong writing to File.";
    }
  }
  extract_attr_init(file_name.c_str(), event->exp_id, filepath);
  unlink(file_name.c_str());
}

static int simple_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
  BOOST_LOG_TRIVIAL(debug) << "Simple Handler";
  simple_rec_ptr event = (_simple_rec*)vevent;
  file_receive(event);
  //free(event->file_buf);
  //free(event->file_path);
  return 1;
}

static void cleanup_handler(int dummy=0){
  BOOST_LOG_TRIVIAL(info) << "Cleanup Called";

  std::string command = "fusermount -zu " + mount_point;
  FILE* stream=popen(command.c_str(),"r");
  if(!stream) fclose(stream);

  BOOST_LOG_TRIVIAL(info) << "Command executed: " << command;

  if(!khan_data) free(khan_data);
  EVfree_stone(cm, stone);
  free(string_list);
  free_attr_list(contact_list);
  fuse_opt_free_args(&args);
  measurements_cleanup();
  exit(0);
}

void create_graphs(int signum)
{
  if (signum == SIGUSR1)
  {
    printf("Received SIGUSR1!\n");
    analytics();
  }
}

int main(int argc, char **argv)
{
  measurements_init();
  
  int forked = 0;
  cm = CManager_create();
  forked = CMfork_comm_thread(cm);
  assert(forked == 1);
  if (forked) {
    BOOST_LOG_TRIVIAL(info) << "Forked a communication thread";
  } else {
    BOOST_LOG_TRIVIAL(info) << "Doing non-threaded communication handling";
  }
  CMlisten(cm);

  stone = EValloc_stone(cm);
  EVassoc_terminal_action(cm, stone, simple_format_list, simple_handler, NULL);
  contact_list = CMget_contact_list(cm);
  string_list = attr_list_to_string(contact_list);

  BOOST_LOG_TRIVIAL(info) << "Contact List: " << stone << ":" << string_list;

  signal(SIGINT, cleanup_handler);
  signal(SIGUSR1, create_graphs);

  Py_SetProgramName(argv[0]);  /* optional but recommended */
  Py_Initialize();

  PyRun_SimpleString("import sys");
  PyRun_SimpleString("import os");
  PyRun_SimpleString("sys.path.append(os.path.join(os.getcwd(), \"PyScripts\"))");

  xmp_initialize();

  std::string store_filename="stores.txt"; /* Default */

  int port = -1;
  int opt;

  /*Add the program name to fuse*/
  fuse_opt_add_arg(&args, argv[0]);

  while ((opt = getopt (argc, argv, "dm:p:s:")) != -1)
  {
    switch (opt)
    {
      case 'm':
        mount_point = optarg;
        fuse_opt_add_arg(&args, optarg);
        break;

      case 'd':
        fuse_opt_add_arg(&args, "-d");
        break;

      case 'p':
        port = atoi(optarg);
        break;

      case 's':
        store_filename = optarg;
        break;
    }
  }

  /* Setting fuse options */
  fuse_opt_add_arg(&args, "-o");
  fuse_opt_add_arg(&args, "allow_other");
  fuse_opt_add_arg(&args, "-o");
  fuse_opt_add_arg(&args, "default_permissions");
  fuse_opt_add_arg(&args, "-o");
  fuse_opt_add_arg(&args, "umask=022"); 

  /* Set signal handler */
  signal(SIGTERM, cleanup_handler);
  signal(SIGKILL, cleanup_handler);
  signal(SIGSEGV, cleanup_handler);

  BOOST_LOG_TRIVIAL(debug) << "Store filename: " << store_filename;

  FILE* stores = fopen(store_filename.c_str(), "r");
  char buffer[100];
  char buffer2[100];
  fscanf(stores, "%s\n", buffer);
  this_server_id = buffer;
  while(fscanf(stores, "%s %s\n", buffer, buffer2)!=EOF) {
    servers.push_back(buffer);
    server_ids.push_back(buffer2);
    if(this_server_id == buffer2) {
      this_server = buffer;
    }
  }
  fclose(stores);

  khan_data = (khan_state*)calloc(sizeof(struct khan_state), 1);
  if (khan_data == NULL)  {
    BOOST_LOG_TRIVIAL(fatal) << "Could not allocate memory to khan_data.. Aborting";
    abort();
  }

  boost::thread khan_init_thread(initializing_khan, (void*)mount_point.c_str(), servers, server_ids, port);

  BOOST_LOG_TRIVIAL(info) << "Initialized Khan";

  fuse_main(args.argc,args.argv, &khan_ops, khan_data);

  BOOST_LOG_TRIVIAL(info) << "Fuse Running";

  measurements_cleanup();

  return 0;
}
