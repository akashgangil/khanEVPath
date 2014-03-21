#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <glob.h>
#include <pthread.h>

/* this file is evpath/examples/triv.c */
#include "evpath.h"

using namespace std;

typedef struct _simple_rec {
    char* file_path;
    char* file_buf;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
    {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
    {"file_buf", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_buf)},
    {NULL, NULL, 0, 0}
};

static FMStructDescRec simple_format_list[] =
{
    {"simple", simple_field_list, sizeof(simple_rec), NULL},
    {NULL, NULL}
};


/* this file is evpath/examples/net_send.c */
int main(int argc, char **argv)
{
    CManager cm;
    simple_rec data;
    EVstone stone;
    EVsource source;
    char string_list[2048];
    attr_list contact_list;
    EVstone remote_stone;
    if (sscanf(argv[1], "%d:%s", &remote_stone, &string_list[0]) != 2) {
        printf("Bad arguments \"%s\"\n", argv[1]);
        exit(0);
    }
    cm = CManager_create();
    CMlisten(cm);
    stone = EValloc_stone(cm);
    contact_list = attr_list_from_string(string_list);
    EVassoc_bridge_action(cm, stone, contact_list, remote_stone);

    source = EVcreate_submit_handle(cm, stone, simple_format_list);

    string pattern = "/net/hu21/agangil3/data" + "/*";

    for(int count = 18; count > 0; count--) {
      
        glob((pattern +".im7").c_str(), 0, NULL, &files);        

        for(int j=0; j<files.gl_pathc; j++) {
              string file_path = files.gl_pathv[j];
              printf("*** FILE Path *** %s\n", file_path.c_str());
              
              size_t path_len = file_path.size() + 1;
              data -> file_path = malloc(sizeof(char) * path_len);
              strncpy(data -> file_path, file_path, path_len); 

              FILE *f = fopen("start.c", "rb");
              fseek(f, 0, SEEK_END);
              size_t fsize = ftell(f);
              fseek(f, 0, SEEK_SET);

              data -> file_buf = malloc(fsize + 1);
              fread(data -> file_buf, fsize, 1, f);
              fclose(f);

              data -> file_buf[fsize] = 0;

              EVsubmit(source, &data, NULL);

        }
      
        pattern += "/*";

    }

}


