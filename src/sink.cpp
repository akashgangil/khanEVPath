#include "Python.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include "fileprocessor.h"
#include "database.h"
#include "params.h"
#include "threadpool.h"
#include "evpath.h"
#include "ev_dfg.h"
#include "khan.h" 
#include "data_analytics.h"
#include "stopwatch.h"
#include "measurements.h"
#include "log.h"
#include "dfg_functions.h"
#include "khan_ffs.h"

extern struct stopwatch_t* sw;

CManager cm;

std::vector < std::string > servers;
std::vector < std::string > server_ids;
std::string this_server;
std::string this_server_id;

extern FMField simple_field_list[];
extern FMStructDescRec simple_format_list[]; 

static EVsource stor_source_handle;

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
  // FIXME:Need to make a more dynamic way of doing this, rather than hard
  // coding this in the future.
  std::string filepath (event->file_path);
  //24 is the length of the server name
  //10 is the length of the im7 file name
  std::string dir_name = filepath.substr(49, strlen(event->file_path) - 59);
  std::string file_name = "/dev/shm/" + filepath.substr(49, strlen(event->file_path) - 49);

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
  extract_attr_init(file_name, event->exp_id, filepath);
  unlink(file_name.c_str());
}

static int simple_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
  log_info("Simple Handler");
  simple_rec_ptr event = (_simple_rec*)vevent;
  if(EVclient_source_active(stor_source_handle))
  {
    printf("Sending the message on...\n");
    EVsubmit(stor_source_handle, event, NULL);
  }
  else
  {
    fprintf(stderr, "Error, stor_source_handle inactive something is wrong!!\n");
  }

  file_receive(event);
  return 1;
}

static void cleanup_handler(int dummy, siginfo_t *siginfo, void* context){
  log_info("Cleanup Called");

  log_info("Sending PID: %ld, UID: %ld\n",
      (long)siginfo->si_pid, (long)siginfo->si_uid);

  measurements_cleanup();

  //CMsleep(cm, 10); /* service network for 600 seconds */
  CManager_close(cm);
  log_info("Closed the CManager");

  cleanup_python();
  PyRun_SimpleString("import gc");
  PyRun_SimpleString("gc.collect()");

  log_info("Stop Python");
  Py_Finalize();

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
  struct sigaction act;
  memset (&act, '\0', sizeof(act));
  act.sa_sigaction = &cleanup_handler;
  act.sa_flags = 0;

  measurements_init();

  EVclient test_client;
  EVclient_sinks sink_capabilities;
  EVclient_sources source_capabilities;

  (void)argc; (void)argv;
  cm = CManager_create();
  CMlisten(cm);

  char master_address[200];
  dfg_get_master_contact_func(master_address,"master.info");

  char source_node[300] = "src_";
  strcat(source_node, argv[1]);

  stor_source_handle = EVcreate_submit_handle(cm, -1, simple_format_list);
  source_capabilities = EVclient_register_source(source_node, stor_source_handle);

  sink_capabilities = EVclient_register_sink_handler(cm, "sink_b", simple_format_list,
      (EVSimpleHandlerFunc) simple_handler, NULL);

  /* We're node "a" in the DFG */
  test_client = EVclient_assoc(cm, argv[1], master_address, source_capabilities, sink_capabilities);

  if (EVclient_ready_wait(test_client) != 1) {
    /* dfg initialization failed! */
    exit(1);
  }

  /**************************/

  if (sigaction(SIGINT, &act, NULL) < 0) {
    perror ("sigaction");
    return 1;
  }

  //signal(SIGUSR1, create_graphs);

  Py_SetProgramName(argv[0]);  /* optional but recommended */
  Py_Initialize();
  PyObject *sys = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sys, "path");
  char cwd[1024];
  std::string py = "/PyScripts";
  std::string pyscripts_path = strdup(getcwd(cwd, sizeof(cwd))) + py;
  PyList_Append(path, PyString_FromString(pyscripts_path.c_str()));
  PySys_SetObject("path", path);

  std::string store_filename="stores.txt"; /* Default */

  int port = 6379;
  int opt;
  std::string host = "localhost";
  while ((opt = getopt (argc, argv, "p:s:h:")) != -1)
  {
    switch (opt)
    {
      case 'p':
        port = atoi(optarg);
        break;

      case 's':
        store_filename = optarg;
        break;

      case 'h':
        host = optarg;
        break;
    }
  }
  /* Set signal handler */
  //signal(SIGTERM, cleanup_handler);
  //signal(SIGKILL, cleanup_handler);
  //signal(SIGSEGV, cleanup_handler);

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

  arg_struct khan_args;
  khan_args.mnt_dir = argv[1];
  khan_args.servers = servers;
  khan_args.server_ids = server_ids;
  khan_args.port = port;
  khan_args.host = host;

  initializing_khan((void*)&khan_args);
  log_info("Initialized Khan");

  CMrun_network(cm);
  return 0;
}
