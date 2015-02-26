#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "params.h"
#include "log.h"
#include "khan.h"
#include "fuseapi.h"
#include "database.h"

struct khan_state *khan_data;
extern struct fuse_operations khan_ops;
struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

std::vector < std::string > servers;
std::vector < std::string > server_ids;
std::string this_server;
std::string this_server_id;

std::string mount_point;

static void cleanup_handler(int dummy, siginfo_t *siginfo, void *context){

  struct fuse_context* f_context;

  f_context = fuse_get_context();
  printf("UID: %d GID: %d", f_context->uid, f_context->gid);
  log_info("Unmount fuse");

  std::string command = "fusermount -zu " + mount_point;
  FILE* stream=popen(command.c_str(),"r");
  if(!stream) fclose(stream);

  log_info("Free fuse args");
  fuse_opt_free_args(&args);
  log_info("Free khan_data");
  if(!khan_data) free(khan_data);
  exit(1);
}

int main(int argc, char **argv){
  struct sigaction act;
  memset (&act, '\0', sizeof(act));
  act.sa_sigaction = &cleanup_handler;
  act.sa_flags = 0;

  if(sigaction(SIGINT, &act, NULL) < 0) {
    perror ("sigaction");
    return 1;
  }
  int opt;
  int port;
  std::string host = "localhost";
  fuse_opt_add_arg(&args, argv[0]);

  while ((opt = getopt (argc, argv, "dm:h:p:")) != -1)
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

      case 'h':
        host = optarg;
    }
  }

  init_database(host, port);

  xmp_initialize();

  fuse_opt_add_arg(&args, "-o");
  fuse_opt_add_arg(&args, "default_permissions");
  fuse_opt_add_arg(&args, "-o");
  fuse_opt_add_arg(&args, "umask=022"); 

  khan_data = (khan_state*)calloc(sizeof(struct khan_state), 1);
  if (khan_data == NULL)  {
    log_err("Could not allocate memory to khan_data.. Aborting");
    abort();
  } 

  std::string store_filename="stores.txt"; /* Default */
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

  return fuse_main(args.argc,args.argv, &khan_ops, khan_data);
//  return 0;
}

