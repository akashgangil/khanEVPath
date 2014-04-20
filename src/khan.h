
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

static char command[MAX_PATH_LENGTH];
static struct khan_state *khan_data=NULL;

static time_t time_now;
static char * fpath=NULL;
static mode_t khan_mode=S_ISUID | S_ISGID | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
static char * temp=NULL;
static char * temp2=NULL;
static char *args=NULL;
static int timestamp;

#define PACKAGE_VERSION 2.6

void* initializing_khan(void * mnt_dir, std::vector < std::string > servers, std::vector < std::string > server_ids);
      
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

void khan_terminate(int);

void unmounting(std::string);

#endif
