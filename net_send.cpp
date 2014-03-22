#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>

#include <glob.h>

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

    //MAGIC STRING
    string server = "/net/hu21/agangil3/data4";

    string pattern = server + "/*";
    glob_t files;

    for(int count = 18; count > 0; count--) {
      
        glob((pattern +".im7").c_str(), 0, NULL, &files);        

        printf("Globbing with pattern: %s .im7\n", pattern.c_str());

        for(int j=0; j<files.gl_pathc; j++) {
              string filepath = files.gl_pathv[j];
              printf("*** FILE Path *** %s\n", filepath.c_str());
              
              data.file_path = strdup(filepath.c_str());
     /* 
              FILE *f = fopen(file_path.c_str(), "rb");
              fseek(f, 0, SEEK_END);
              long fsize = ftell(f);
              fseek(f, 0, SEEK_SET);

              data.file_buf = (char*) malloc((fsize + 1) * sizeof(char));
              fread(data.file_buf, fsize, 1, f);
              fclose(f);

              data.file_buf[fsize] = '\0';

              printf("FILE buf copied\n");
*/
              EVsubmit(source, &data, NULL);

              free(data.file_path);
  //            free(data.file_buf);
        }
      
        pattern += "/*";

    }
}


