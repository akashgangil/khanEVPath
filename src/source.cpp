#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <boost/log/trivial.hpp>

#include <vector>

#include <set>

#include <fcntl.h>
#include <glob.h>

#include "evpath.h"

std::vector < std::string > servers;
std::vector < std::string > server_ids;

std::string this_server;
std::string this_server_id;

typedef struct _simple_rec {
    char* file_path;
    long file_buf_len;
    char* file_buf;
    int exp_id;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
    {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
    {"file_buf_len", "integer", sizeof(long), FMOffset(simple_rec_ptr, file_buf_len)},
    {"file_buf", "char[file_buf_len]", sizeof(char), FMOffset(simple_rec_ptr, file_buf)},
    {"exp_id", "integer", sizeof(int), FMOffset(simple_rec_ptr, exp_id)},
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
        BOOST_LOG_TRIVIAL(error) << "Bad arguments " << argv[1];
        exit(0);
    }

    cm = CManager_create();
    CMlisten(cm);
    stone = EValloc_stone(cm);
    contact_list = attr_list_from_string(string_list);
    EVassoc_bridge_action(cm, stone, contact_list, remote_stone);

    source = EVcreate_submit_handle(cm, stone, simple_format_list);

    const char* store_filename="stores.txt";
    FILE* stores = fopen(store_filename, "r");
    char buffer[100];
    char buffer2[100];
    fscanf(stores, "%s\n", buffer);
    this_server_id = buffer;
    while(fscanf(stores, "%s %s\n", buffer, buffer2)!=EOF) {
      if(strcmp(buffer,"cloud")==0) {
        std::string module = buffer2;
        module = "cloud." + module;
        //cloud_interface = PyImport_ImportModule(module.c_str());
      }   
      servers.push_back(buffer);
      server_ids.push_back(buffer2);
      if(this_server_id == buffer2) {
        this_server = buffer;
      }   
    }
    fclose(stores);

    std::string pattern = servers[0] + "/*";
    glob_t files;

    int fdin;
    struct stat statbuf;
    std::set<std::string> experiments;

    for(int count = 18; count > 0; count--) {
      
        glob((pattern +".im7").c_str(), 0, NULL, &files);        

        BOOST_LOG_TRIVIAL(info) << "Globbing with pattern: " << pattern << ".im7";

        for(unsigned j=0; j<files.gl_pathc; j++) {
              std::string filepath = files.gl_pathv[j];
              BOOST_LOG_TRIVIAL(debug) << "FILE Path: " <<  filepath;

              std::string exp_dir = filepath.substr(0, filepath.size() - 10);
              experiments.insert(exp_dir);
              data.exp_id = (int)experiments.size();

              data.file_path = strdup(filepath.c_str());

              if ((fdin = open (filepath.c_str(), O_RDONLY)) < 0)
                  perror ("can't open %s for reading"); 

              if (fstat (fdin,&statbuf) < 0)
                  perror ("fstat error");

              data.file_buf_len = statbuf.st_size;
              
              BOOST_LOG_TRIVIAL(debug) << "Size: " <<  data.file_buf_len;

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


