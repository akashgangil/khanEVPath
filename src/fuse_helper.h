#ifndef FUSE_HELPER_H
#define FUSE_HELPER_H

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define SELECTOR_C '@'
#define SELECTOR_S "@"

void map_path(std::string, std::string);
void unmap_path(std::string, std::string);
void dir_pop_stbuf(struct stat*, std::string contents);
void file_pop_stbuf(struct stat*, std::string filename);
std::string resolve_selectors(std::string);

#endif

