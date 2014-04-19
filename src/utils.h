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

using namespace std;

extern char* fpath;
extern vector<string> servers;

string trim(string source, string t = " \n");
char* append_path2(string newp);
char* append_path(const char * newp);
int count_string(string tobesplit);
string intersect(string files1, string files2);
string subtract(string files1, string files2);
int get_file_size(string file_name);
vector<string> split(string str, string delim);
string join(vector<string> these, string delim);
string bin2hex(const char* input, size_t size);
string hex2bin(string in);


#ifdef APPLE
int clock_gettime(int i, struct timespec* b);
char* strdup(const char* str);
#endif

#endif
