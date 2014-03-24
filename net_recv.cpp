#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#include "threadpool.h"
#include "evpath.h"

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

void print_message(void *vevent){
    
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
    
    printf("Return event buffer\n");
    EVreturn_event_buffer(cm, vevent);
}

static int
simple_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
    EVtake_event_buffer(cm , vevent);
    threadpool_add(t_p, &print_message, vevent, 0);    

    return 1;
}

void cleanupHandler(int dummy=0){
    printf("Cleanup Called\n");
    threadpool_destroy(t_p, 0);
    exit(0);
}

int main(int argc, char **argv)
{
    EVstone stone;
    char *string_list;
    cm = CManager_create();
    CMlisten(cm);
    stone = EValloc_stone(cm);
    EVassoc_terminal_action(cm, stone, simple_format_list, simple_handler, NULL);
    string_list = attr_list_to_string(CMget_contact_list(cm));
    printf("Contact list \"%d:%s\"\n", stone, string_list);

    signal(SIGINT, cleanupHandler);
    
    t_p = threadpool_create( 4, 1000, 0);    

    CMrun_network(cm);
    return 0;
}
