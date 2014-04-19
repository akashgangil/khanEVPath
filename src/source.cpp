#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <glob.h>

#include "evpath.h"

using namespace std;

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
    string server = "/net/hu21/agangil3/data";

    string pattern = server + "/*";
    glob_t files;

    int fdin;
    char *src;
    struct stat statbuf;

    for(int count = 18; count > 0; count--) {
      
        glob((pattern +".im7").c_str(), 0, NULL, &files);        

        printf("Globbing with pattern: %s .im7\n", pattern.c_str());

        for(int j=0; j<files.gl_pathc; j++) {
              string filepath = files.gl_pathv[j];
              printf("*** FILE Path *** %s\n", filepath.c_str());
              
              data.file_path = strdup(filepath.c_str());

              if ((fdin = open (filepath.c_str(), O_RDONLY)) < 0)
                  perror ("can't open %s for reading"); 

              if (fstat (fdin,&statbuf) < 0)
                  perror ("fstat error");

              data.file_buf_len = statbuf.st_size;
              
              printf("Size: %ld\n", data.file_buf_len);

              if ((data.file_buf = (char*)mmap (0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fdin, 0))
                         == (caddr_t) -1)
                  perror ("mmap error for input");

              EVsubmit(source, &data, NULL);

              if (munmap(data.file_buf, statbuf.st_size) == -1) {
                  perror("Error un-mmapping the file");
              }

              close(fdin);

              free(data.file_path);
        }
      
        pattern += "/*";

    }
}


