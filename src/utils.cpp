#include "database.h"
#include "utils.h"

#include <boost/log/trivial.hpp>

#ifdef APPLE
int clock_gettime(int i, struct timespec* b) { 
    return 0;
}

char* strdup(const char* str) {
    char* newstr = (char*)malloc(strlen(str)+1);
    strcpy(newstr, str);
    return newstr;
}
#endif

int get_file_size(std::string path){
    struct stat st;
    std::cout << "Path: " << path << std::endl;
    stat(path.c_str(), &st);
    return st.st_size;
}

std::vector<std::string> split(std::string istr, std::string delim) {
    std::vector<std::string> vec; 
    char *saveptr;
    char *token;
    char *str = strdup(istr.c_str());
    const char *delimiters = delim.c_str();
    for(token = strtok_r(str, delimiters, &saveptr);
            token != NULL;
            token = strtok_r(NULL, delimiters, &saveptr)) {
        vec.push_back(std::string(token));
    }
    free(str);
    return vec; 
}

std::string join(std::vector<std::string> these, std::string delim) {
    std::string ret = "";
    for(unsigned i=0; i<these.size(); i++) {
        if(i>0) {
            ret+=delim;
        }
        ret+=these[i];
    }
    return ret; 
}


std::string subtract(std::string files1, std::string files2){
    std::string ret="";
    std::string tok1="";
    std::string tok2="";
    std::stringstream f1(files1);
    while(getline(f1,tok1,':')){
        std::stringstream f2(files2);
        bool found = false;
        while(getline(f2,tok2,':')){
            if(strcmp(tok1.c_str(),tok2.c_str())==0){
                found = true;        
            }
        }
        if(!found) {
            ret+=":"+tok1;
        }
    }
    return ret;
}
std::string intersect(std::string files1, std::string files2){
    std::string ret="";
    std::string tok1="";
    std::string tok2="";
    std::stringstream f1(files1);
    while(getline(f1,tok1,':')){
        std::stringstream f2(files2);
        while(getline(f2,tok2,':')){
            if(strcmp(tok1.c_str(),tok2.c_str())==0){
                ret+=":"+tok1;
            }
        }
    }
    return ret;
}

std::string trim_right(std::string source, std::string t = " \n")
{
    std::string str = source;
    return str.erase( str.find_last_not_of(t) + 1);
}

std::string trim_left( std::string source, std::string t = " \n")
{
    std::string str = source;
    return str.erase(0 , source.find_first_not_of(t) );
}

std::string trim(std::string source, std::string t)
{
    std::string str = source;
    return trim_left( trim_right( str , t) , t );
}


int count_string(std::string tobesplit){
    int count=0;
    if(strcmp(tobesplit.c_str(),"null")==0){
        return 0;
    } else {
        std::stringstream ss(tobesplit.c_str());
        std::string token;
        while(getline(ss, token, ':')){
            if(token.length()>0) {
                count++;
            }
        }
        return count;
    }
}


char* append_path(const char * newp) {
    std::string servers[] = {"test1"};
    
    BOOST_LOG_TRIVIAL(debug) << "append_path " << servers[0] << " " << newp;
    
    char* fpath=(char*)malloc(MAX_PATH_LENGTH);
    memset(fpath,0,MAX_PATH_LENGTH);
    sprintf(&fpath[0],"%s%s",servers[0].c_str(),newp);
    return fpath;
}

char* append_path2(std::string newp) {

    std::string fid = database_getval("name", newp);
    std::string file_path = database_getval(fid, "file_path");

    return strdup(file_path.c_str());
}

std::string bin2hex(const char* input, size_t size)
{
    std::string res;
    for(unsigned i=0; i<size; i++)
    {
        unsigned char c = input[i];
        res += (char)(c+10);
    }

    return res;
}

std::string hex2bin(std::string in) {
    for(unsigned i=0; i<in.length(); i++) {
        in[i]-=10;
    }
    return in;
}
