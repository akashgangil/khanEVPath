
/*#include <mach/mach.h>
#include <mach/mach_time.h>*/

#ifndef KHAN_H
#define KHAN_H

#define MAX_LEN 4096 

//#define FUSE_USE_VERSION 26
#define MAX_PATH_LENGTH 2048

#ifndef SELECTOR_C
#define SELECTOR_C '@'
#endif

#ifndef SELECTOR_S
#define SELECTOR_S "@"
#endif

#include <string>
#include <vector>
#include "fuse.h"

#define PACKAGE_VERSION 2.6

typedef struct{
  void *mnt_dir;
  std::vector < std::string > servers;
  std::vector < std::string > server_ids;
  int port;
} arg_struct;

void* initializing_khan(void* args);
      
int khan_opendir(const char *c_path, struct fuse_file_info *fi);
    
bool find(std::string str, std::vector< std::string > arr); 
   
std::string str_intersect(std::string str1, std::string str2); 
        
bool content_has(std::string vals, std::string val); 
        
void dir_pop_stbuf(struct stat* stbuf, std::string contents); 
        
void file_pop_stbuf(struct stat* stbuf, std::string filename); 
            
std::string resolve_selectors(std::string path); 
   
int populate_getattr_buffer(struct stat* stbuf, std::stringstream &path); 
    
int khan_getattr(const char *c_path, struct stat *stbuf); 
    
void dir_pop_buf(void* buf, fuse_fill_dir_t filler, std::string content, bool convert);
      
void populate_readdir_buffer(void* buf, fuse_fill_dir_t filler, std::stringstream &path);

void unmounting(std::string);

#endif
