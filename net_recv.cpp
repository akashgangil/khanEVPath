#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include "threadpool.h"

#include "evpath.h"

threadpool_t* t_p;
CManager cm;

typedef struct _simple_rec {
    char* file_path;
    char* file_buf;
    long file_buf_len;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
    {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
    {"file_buf", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_buf)},
    {"file_buf_len", "integer", sizeof(long), FMOffset(simple_rec_ptr, file_buf_len)},
    {NULL, NULL, 0, 0}
};

static FMStructDescRec simple_format_list[] =
{
    {"simple", simple_field_list, sizeof(simple_rec), NULL},
    {NULL, NULL}
};

void print_message(void *vevent){
    
    simple_rec_ptr event = (_simple_rec*)vevent;
    if(event != NULL)
      printf("[THREADING ]I  got %s\n", event->file_path);

    std::string filepath (event->file_path);
    //24 is the length of the server name
    //10 is the length of the im7 file name
    std::string dir_name = filepath.substr(24, strlen(event->file_path) - 34);
    std::string file_name = filepath.substr(24, strlen(event->file_path) - 24);

    FILE* stream=popen(("mkdir -p \"" + dir_name + "\"").c_str(),"r");
    fclose(stream);

    //std::cout << "Directory NAME:   " << dir_name << "\n";
    //std::cout << "FILE NAME:    " << file_name << "\n";

    if(event->file_buf != NULL) {
      
      printf("GOT: ***: %ld\n", event->file_buf_len);

      FILE* pFile = fopen(file_name.c_str() ,"wb");

      if (pFile){
          /* Write your buffer to disk. */
          size_t w = fwrite(event->file_buf, event->file_buf_len-1, 1, pFile);
          printf("Wrote to file! %zu\n", w);
      }
      else{
          printf("Something wrong writing to File.");
      }
      //fputc ( EOF , pFile ); 
      fclose(pFile);
    }
    
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
