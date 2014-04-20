#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#define MAX_PATH_LENGTH 2048

extern char* fpath;
extern std::vector<std::string> servers;

std::string trim(std::string source, std::string t = " \n");
char* append_path2(std::string newp);
char* append_path(const char * newp);
int count_string(std::string tobesplit);
std::string intersect(std::string files1, std::string files2);
std::string subtract(std::string files1, std::string files2);
int get_file_size(std::string file_name);
std::vector<std::string> split(std::string str, std::string delim);
std::string join(std::vector<std::string> these, std::string delim);
std::string bin2hex(const char* input, size_t size);
std::string hex2bin(std::string in);


#ifdef APPLE
int clock_gettime(int i, struct timespec* b);
char* strdup(const char* str);
#endif

#endif
