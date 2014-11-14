#include "Python.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
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
#include "log.h"


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
  
pthread_t khan_init_thread;

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
    log_info("file_path %s", event->file_path);
    log_info("file_buf_len %ld", event->file_buf_len);
  }
  std::string filepath (event->file_path);
  //24 is the length of the server name
  //10 is the length of the im7 file name
  std::string dir_name = filepath.substr(24, strlen(event->file_path) - 34);
  std::string file_name = "/dev/shm/" + filepath.substr(24, strlen(event->file_path) - 24);

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
  extract_attr_init(file_name.c_str(), event->exp_id, filepath);
  unlink(file_name.c_str());
}

static int simple_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
  log_info("Simple Handler");
  simple_rec_ptr event = (_simple_rec*)vevent;
  file_receive(event);
  //free(event->file_buf);
  //free(event->file_path);
  return 1;
}

static void cleanup_handler(int dummy=0){
  log_info("Cleanup Called");


  std::string command = "fusermount -zu " + mount_point;
  FILE* stream=popen(command.c_str(),"r");
  if(!stream) fclose(stream);

  log_info("Command executed %s", command.c_str());
  
  EVfree_stone(cm, stone);

  if(!khan_data) free(khan_data);
  free(string_list);
  free_attr_list(contact_list);
  fuse_opt_free_args(&args);
  measurements_cleanup();
 
  log_info("Freed constants and fuse arguments");

  //CMsleep(cm, 10); /* service network for 600 seconds */
  CManager_close(cm);
  log_info("Closed the CManager");
  
  cleanup_python();
  
  log_info("Stop Python");
  Py_Finalize();

  pthread_cancel(khan_init_thread);
  pthread_join(khan_init_thread, NULL);
  //pthread_exit(NULL); 
  log_info("Exit pthreads");
  redis_destroy();
  log_info("Shut down redis");
  exit(0);
}

void create_graphs(int signum)
{
  if (signum == SIGUSR1)
  {
    log_info("Received SIGUSR1!");
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
    log_info("Forked a communication thread");
  } else {
    log_info("Doing non-threaded communication handling");
  }
  CMlisten(cm);

  stone = EValloc_stone(cm);
  EVassoc_terminal_action(cm, stone, simple_format_list, simple_handler, NULL);
  contact_list = CMget_contact_list(cm);
  string_list = attr_list_to_string(contact_list);

  log_info("Contact list %d:%s", stone, string_list);

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
//  fuse_opt_add_arg(&args, "-o");
//  fuse_opt_add_arg(&args, "allow_other");
  fuse_opt_add_arg(&args, "-o");
  fuse_opt_add_arg(&args, "default_permissions");
  fuse_opt_add_arg(&args, "-o");
  fuse_opt_add_arg(&args, "umask=022"); 

  /* Set signal handler */
  //signal(SIGTERM, cleanup_handler);
  //signal(SIGKILL, cleanup_handler);
  //signal(SIGSEGV, cleanup_handler);

  log_info("Store filename %s", store_filename.c_str());

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
    log_err("Could not allocate memory to khan_data.. Aborting");
    abort();
  }

  arg_struct khan_args;
  khan_args.mnt_dir = argv[1];
  khan_args.servers = servers;
  khan_args.server_ids = server_ids;
  khan_args.port = port;

  pthread_create(&khan_init_thread, NULL, &initializing_khan, (void*)&khan_args);

  log_info("Initialized Khan");

  fuse_main(args.argc,args.argv, &khan_ops, khan_data);

  log_info("Fuse Running");

  cleanup_handler();

  return 0;
}
