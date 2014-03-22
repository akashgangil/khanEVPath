#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include <pthread.h>

#include "thr_pool.h"

/* this file is evpath/examples/triv.c */
#include "evpath.h"

thr_pool_t* t_p;

typedef struct _simple_rec {
    char* file_path;
//    char* file_buf;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
    {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
 //   {"file_buf", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_buf)},
    {NULL, NULL, 0, 0}
};

static FMStructDescRec simple_format_list[] =
{
    {"simple", simple_field_list, sizeof(simple_rec), NULL},
    {NULL, NULL}
};

void* print_message(void *vevent){
    printf("Threading\n");
    
    simple_rec_ptr event = (_simple_rec*)vevent;
    printf("I  got %s\n", event->file_path);
   
/*
    size_t  buf_size = strlen(event->file_buf);
    printf("FILE SIZE: %zu", buf_size);


    FILE* pFile = fopen("new.c","wb");

    if (pFile){
        /* Write your buffer to disk. */
  /*      fwrite(event->file_buf, buf_size, 1, pFile);
        printf("Wrote to file!");
    }
    else{
        printf("Something wrong writing to File.");
    }
 
    fclose(pFile);
 */
}

static int
simple_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
    thr_pool_queue(t_p, &print_message, vevent);    
/*    
    //MAGIC NUMBER
    size_t prefix_len = 24;
    size_t file_path_len = 10;//strlen(event->file_path);
    
    printf("%d %d", prefix_len, file_path_len);
  
    char file_name[200];

    memcpy(file_name, event->file_path + prefix_len + 1, file_path_len - prefix_len);
    file_name[file_path_len - prefix_len] = '\0';
    printf("File Name is %s", file_name);

    size_t  buf_size = strlen(event->file_buf);
    printf("FILE SIZE: %zu", buf_size);


    FILE* pFile = fopen("new.c","wb");

    if (pFile){
        /* Write your buffer to disk. */
/*        fwrite(event->file_buf, buf_size, 1, pFile);
        printf("Wrote to file!");
    }
    else{
      printf("Something wrong writing to File.");
    }

    fclose(pFile);
*/
    

    return 1;
}


int main(int argc, char **argv)
{
    CManager cm;
    EVstone stone;
    char *string_list;
    cm = CManager_create();
    CMlisten(cm);
    stone = EValloc_stone(cm);
    EVassoc_terminal_action(cm, stone, simple_format_list, simple_handler, NULL);
    string_list = attr_list_to_string(CMget_contact_list(cm));
    printf("Contact list \"%d:%s\"\n", stone, string_list);
    
    t_p = thr_pool_create(1, 4, 1000, NULL);    

    CMrun_network(cm);

}
