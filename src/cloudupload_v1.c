// Author: Jian Huang, Hongbo Zou
// Modified by Yanwei Zhang
// Georgia Institue of Technology

#include "cloudupload.h"
#include "cloudupload_supplement.h"

//#define MEM_COPY_SIZE 16384

// register the object name into the list "head"
static int _register_name(char *oname, struct list_head *head);
// register the object into the download object list "head" (per container)
static int _register_object(struct MemoryStruct *object, char *oname, struct list_head *head);

/************************start***************************************/
// The data area by ptr may be filled with at MOST "size" multiplied with "nmemb" number of bytes
static size_t ReadMemoryCallback(void *ptr, size_t size, size_t nmemb, void *pMem)
{
    struct MemoryStruct *pRead = (struct MemoryStruct *)pMem;
    //printf("size, nmemb, pRead->bytesize= %d, %d, %d\n", size, nmemb, pRead->bytesize);
    size_t curl_size = size * nmemb;
    size_t to_copy = (pRead->bytesize < curl_size) ? pRead->bytesize : curl_size;
    memcpy(ptr, pRead->memory, to_copy);
    pRead->memory += to_copy;
    pRead->bytesize   -= to_copy;
    
    return to_copy;
}

// Data pointed to by ptr is with size "size" multiplied "nmemb", it will not be zero terminated
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t curl_size = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    //printf("size, nmemb, curl_size = %d, %d, %d\n", size, nmemb, curl_size);
    mem->memory = (char*)realloc(mem->memory, mem->bytesize + curl_size + 1); //for the following copy
    if (mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&(mem->memory[mem->bytesize]), contents, curl_size);
    mem->bytesize += curl_size;
    mem->memory[mem->bytesize] = 0; //for the following copy

    return curl_size;
}

int get_token(const char *tname, const char *username, const char *pswd, const char *addr, char **token, char **url)
{
    char cmd[COMMAND_LINE_SIZE];
    //char strToken[TOKEN_STR_SIZE] = "";
    //char strURL[URL_STR_SIZE] = "";
    //char *strToken = NULL;
    //char *strURL = NULL;
    *token = (char*)malloc(TOKEN_STR_SIZE*sizeof(char));
    *url = (char*)malloc(URL_STR_SIZE*sizeof(char));
    int iSuccess = -1;
    /*
       strcpy(cmd, "curl -s -d '{\"auth\": {\"tenantName\": \"");
       strcat(cmd, tname);
       strcat(cmd, "\", \"passwordCredentials\": {\"username\": \"");
       strcat(cmd, username);
       strcat(cmd, "\", \"password\": \"");
       strcat(cmd, pswd);
       strcat(cmd, "\"}}}' -H 'Content-type: application/json' http://shale.cc.gt.atl.ga.us:5000/v2.0/tokens > raw_tfile");
       */
    strcpy(cmd, "curl -i -k -v -H 'X-Storage-User: ");
    strcat(cmd, tname);
    strcat(cmd,":");
    strcat(cmd,username);
    strcat(cmd, "' -H 'X-Storage-Pass: ");
    strcat(cmd, pswd);
    strcat(cmd, "' ");
    strcat(cmd, "http://");
    strcat(cmd, addr);
    strcat(cmd, ":8080/auth/v1.0 2>comments 1>./raw_tfile");
    //printf("cmd: %s\n", cmd);
    system(cmd);
    system("rm tfile");
    //system("cat raw_tfile | awk -F=\":\" -v RS=\",\" '$1~/\"id\"/ {print}' | sed -n 1p | awk '{FS=\":\"}{print $2}' | awk -F\"}\" '{print $1}' | awk '{gsub(/\042/,\"\");print}' >> tfile");
    //system("cat raw_tfile | awk -F=\":\" -v RS=\",\" '$1~/\"publicURL\"/ {print}' | grep -r \"8080\" | awk '{FS=\":\"}{print $2}' | awk -F\"}\" '{print $1}' | awk '{gsub(/\042/,\"\");print}' >> tfile");
    //system("cat raw_tfile | grep -r \"X-Auth-Token\" | awk '{FS=\":\"}{print $2}'>> tfile");
    //system("cat raw_tfile | grep -r \"X-Storage-Url\"| awk '{FS=\":\"}{print $2}'>> tfile");
    system("cat raw_tfile | grep \"X-Auth-Token\" | awk '{FS=\":\"}{print $2}'>> tfile");
    system("cat raw_tfile | grep \"X-Storage-Url\"| awk '{FS=\":\"}{print $2}'>> tfile");
    FILE *file = fopen(filename, "r");
    if(file != NULL){
        if(fgets(*token, TOKEN_STR_SIZE, file) != NULL){
            //printf("token size: %d\n",strlen(*token));
            (*token)[strlen(*token)-2] = '\0';
        }
        if(fgets(*url, URL_STR_SIZE, file) != NULL){
            //printf("url: %s\n",*url);

            (*url)[strlen(*url)-2] = '\0';
        }
        if((strlen(*token) > 0) && (strlen(*url) > 0)){
            iSuccess = 0; 
        }
    }
    else{
        iSuccess = -1;  
    } 
    if(iSuccess == 0){
        ;
    }
    fclose(file);
    return iSuccess;

}


int create_container(const char *token, const char *url, const char *cname)
{
    if( !token || !url || !cname ) return -1;

    // to check if the "cname" container exists
    struct list_head *cnl = get_clist(token, url);
    struct NameList *item;
    int is_exist = 0;
    list_for_each_entry(item, cnl, link){
        if( is_exist = !(strcmp(cname, item->name)) ) break;
    }
    free_namelist(cnl);
    if(is_exist){
        printf("container %s already exist!\n", cname);
        return 0;
    }

    // otherwise, create the new "cname" container
    struct curl_slist *headers = NULL;
    CURL *curl;
    CURLcode res;
    int iResult = 0;
    curl = curl_easy_init();
    char xtoken_str[128] = "X-Auth-token:";
    char xurl_str[128] = "";

    if(curl){
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
        curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

        headers = curl_slist_append(headers, "-k -X -H");
        sprintf(xtoken_str, "%s%s", xtoken_str, token);
        headers = curl_slist_append(headers, xtoken_str);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // setup CURLOPT_URL
        //sprintf(xurl_str, "%s%s", url, "/");
        strcpy(xurl_str, url);
        strcat(xurl_str,"/");
        //printf("##xurl_str: %s\n", xurl_str);
        strcat(xurl_str,cname);
        //sprintf(xurl_str, "%s%s", xurl_str, cname);
        printf("##create_container: %s\n",xurl_str); 
        curl_easy_setopt(curl, CURLOPT_URL, xurl_str);
        //printf("curl command: %s\n",curl);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            iResult = -1;
        }

        curl_easy_cleanup(curl);

    }
    else{
        iResult = -1;
    }
    return iResult;
}



int delete_container(const char *token, const char *url, const char *cname)
{
    if( !token || !url || !cname ) return -1;
    
    // to check if the "cname" container exists
    struct list_head *cnl = get_clist(token, url);
    struct NameList *item;
    int is_exist = 0;
    list_for_each_entry(item, cnl, link){
        if( is_exist = !(strcmp(cname, item->name)) ) break;
    }
    free_namelist(cnl);
    if(!is_exist){
        printf("container %s does not exist!\n", cname);
        return 0;
    }

    // to delete the "cname" container
    struct curl_slist *headers = NULL;
    CURL *curl;
    CURLcode res;
    int iResult = 0;
    curl = curl_easy_init();
    char xtoken_str[80] = "X-Auth-token:";
    char xurl_str[128] = "";

    if(curl){
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
        curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

        headers = curl_slist_append(headers, "-X -H");
        sprintf(xtoken_str, "%s%s", xtoken_str, token);
        headers = curl_slist_append(headers, xtoken_str);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        sprintf(xurl_str, "%s%s", url, "/");
        sprintf(xurl_str, "%s%s", xurl_str, cname);
        //printf("xurl_str: %s\n",xurl_str);
        curl_easy_setopt(curl, CURLOPT_URL, xurl_str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            iResult = -1;
        }

        curl_easy_cleanup(curl);

    }
    else{
        iResult = -1;
    }
    return iResult;
}


int upload_object(const char *token, const char *url, const char *cname, const char *oname, struct MemoryStruct *sData)
{
    struct curl_slist *headers = NULL;
    CURL *curl;
    CURLcode res;
    int iResult = 0;
    curl = curl_easy_init();
    char xtoken_str[80] = "X-Auth-token:";
    char xurl_str[128] = "";

    if(curl){
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
        curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

        headers = curl_slist_append(headers, "-X -H");
        sprintf(xtoken_str, "%s%s", xtoken_str, token);
        headers = curl_slist_append(headers, xtoken_str);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_READDATA, (void *)sData);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        sprintf(xurl_str, "%s%s", url, "/");
        sprintf(xurl_str, "%s%s", xurl_str, cname);
        //printf("xurl_str: %s\n",xurl_str);
        sprintf(xurl_str, "%s%s", xurl_str, "/");
        sprintf(xurl_str, "%s%s", xurl_str, oname);
        //printf("#upload xurl_str %s\n", xurl_str);
        curl_easy_setopt(curl, CURLOPT_URL, xurl_str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            iResult = -1;
        }

        curl_easy_cleanup(curl);

    }
    else{
        iResult = -1;
    }
    return iResult; 

}

struct MemoryStruct* download_object(const char *token, const char *url, const char *cname, const char *oname)
{
    struct curl_slist *headers = NULL;
    CURL *curl;
    CURLcode res;
    int iResult = 0;
    curl = curl_easy_init();
    char xtoken_str[80] = "X-Auth-token:";
    char xurl_str[128] = "";

    struct MemoryStruct *sData = (struct MemoryStruct *)calloc(1, sizeof(struct MemoryStruct));
    if(!sData){
        printf("Error in calloc sData\n");
        return NULL;
    }
    sData->memory = (char*)malloc(1); // only 1 byte space
    sData->bytesize = 0;    

    if(curl){

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
        curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

        headers = curl_slist_append(headers, "-X -H");
        sprintf(xtoken_str, "%s%s", xtoken_str, token);
        headers = curl_slist_append(headers, xtoken_str);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)sData);
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&sData);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        sprintf(xurl_str, "%s%s", url, "/");
        sprintf(xurl_str, "%s%s", xurl_str, cname);
        //printf("xurl_str: %s\n",xurl_str);
        sprintf(xurl_str, "%s%s", xurl_str, "/");
        sprintf(xurl_str, "%s%s", xurl_str, oname);
        curl_easy_setopt(curl, CURLOPT_URL, xurl_str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            iResult = -1;
        }

        curl_easy_cleanup(curl);

    }
    else{
        iResult = -1;
    }
    if(iResult == 0) 
        return sData;
    else{ 
        free(sData->memory);
        free(sData);
        return NULL;
    }
}

int delete_object(const char *token, const char *url, const char *cname, const char *oname)
{
    if( !token || !url || !cname || !oname) return -1;

#if 0
    // to check if the "cname" container exists
    struct list_head *onl = get_olist(token, url, cname);
    struct NameList *item;
    int is_exist = 0;
    list_for_each_entry(item, onl, link){
        if( is_exist = !(strcmp(oname, item->name)) ) break;
    }
    free_namelist(onl);
    if(!is_exist){
        printf("object %s does not exist!\n", oname);
        return 0;
    }
#endif

    // to delete the "oname" object
    struct curl_slist *headers = NULL;
    CURL *curl;
    CURLcode res;
    int iResult = 0;
    curl = curl_easy_init();
    char xtoken_str[80] = "X-Auth-token:";
    char xurl_str[128] = "";

    if(curl){
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
        curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

        headers = curl_slist_append(headers, "-X -H");
        sprintf(xtoken_str, "%s%s", xtoken_str, token);
        headers = curl_slist_append(headers, xtoken_str);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        sprintf(xurl_str, "%s%s", url, "/");
        sprintf(xurl_str, "%s%s", xurl_str, cname);
        //printf("xurl_str: %s\n",xurl_str);
        sprintf(xurl_str, "%s%s", xurl_str, "/");
        sprintf(xurl_str, "%s%s", xurl_str, oname);
        //printf("#delete xurl_str %s\n", xurl_str);
        curl_easy_setopt(curl, CURLOPT_URL, xurl_str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            iResult = -1;
        }

        curl_easy_cleanup(curl);

    }
    else{
        iResult = -1;
    }
    return iResult; 

}


struct list_head* download_cobjects(const char *token, const char *url, const char *cname)
{
    if( !token || !url || !cname ) return NULL;

    struct list_head *onl = get_olist(token, url, cname);
    if( !onl ) return NULL;

    struct list_head *col = (struct list_head *)calloc(1, sizeof(struct list_head)); 
    if( !col ) return NULL;
    INIT_LIST_HEAD(col);

    struct NameList *item;
    struct MemoryStruct *object;
    int ret;
    list_for_each_entry(item, onl, link){
        object = download_object(token, url, cname, item->name);

        ret = _register_object(object, item->name, col);
        if(ret) return NULL;
    }
    free_namelist(onl);

    return col;
} 

int delete_cobjects(const char *token, const char *url, const char *cname)
{
    if( !token || !url || !cname ) return -1;

    printf("Deleting all objects in container '%s'\n", cname);
    struct list_head *onl = get_olist(token, url, cname);
    if( !onl ) return -1;
    //print_namelist(onl);

    struct NameList *item;
    int ret;
    list_for_each_entry(item, onl, link){
        //printf("deleting object-%s\n", item->name);
        ret = delete_object(token, url, cname, item->name);
        if(ret) return -1;
    }
    free_namelist(onl);

    return 0;
}

struct list_head* get_olist(const char *token, const char *url, const char *cname)
{
    char cmd[COMMAND_LINE_SIZE];
    char buff[256];
    sprintf(cmd,"curl -v -X GET -H 'X-Auth-Token: %s' %s/%s",token, url, cname);
    //printf("get_olist cmd: %s\n",cmd);    
    //system(cmd);
    FILE *fp = popen(cmd,"r");

    // can be replaced by global var
    struct list_head *onl = (struct list_head *)calloc(1, sizeof(struct list_head)); 
    if( !onl ) return NULL;
    INIT_LIST_HEAD(onl);
    int ret; 
    while(fgets(buff, sizeof buff, fp) != NULL){
        buff[strlen(buff)-1] = '\0'; // to cancel out '\n'
        ret = _register_name(buff, onl);
        if(ret) return NULL;
        //printf("result: %s\n", buff);
    }
    pclose(fp);

    return onl;
}

struct list_head* get_clist(const char *token, const char *url)
{
    char cmd[COMMAND_LINE_SIZE];
    char buff[256];
    sprintf(cmd,"curl -v -X GET -H 'X-Auth-Token: %s' %s",token, url);
    //printf("get_clist cmd: %s\n",cmd);    
    //system(cmd);
    FILE *fp = popen(cmd,"r");

    // can be replaced by global var
    struct list_head *cnl = (struct list_head *)calloc(1, sizeof(struct list_head)); 
    if( !cnl ) return NULL;
    INIT_LIST_HEAD(cnl);
    int ret; 
    while(fgets(buff, sizeof buff, fp) != NULL){
        buff[strlen(buff)-1] = '\0'; // to cancel out '\n'
        ret = _register_name(buff, cnl);
        if(ret) return NULL;
        //printf("result: %s\n", buff);
    }
    pclose(fp);

    return cnl;
}

int free_namelist(struct list_head *head)
{
    if(!head) return -1;

    struct NameList *pos, *next;
    list_for_each_entry_safe(pos, next, head, link){
        free(pos->name);
        free(pos);
    }
    free(head);

    return 0;
}

// for debugging purpose
int print_namelist(struct list_head *head)
{
    if(!head) return -1;

    int count = 0;
    struct NameList *item;
    list_for_each_entry(item, head, link){
        count++;
        printf("name: %s\n", item->name);
    }
    printf("Object Count#: %d\n", count);

    return 0;
}

int free_objects(struct list_head *head)
{
    if(!head) return -1;

    struct ObjectList *pos, *next;
    list_for_each_entry_safe(pos, next, head, link){
        free(pos->object->memory);
        free(pos->object);
        free(pos);
    }
    free(head);

    return 0;
}

// for debugging purpose
int print_metaobjects(struct list_head *head)
{
  if(!head) return -1;

  struct ObjectList *item;
  struct StatusInfo *si;
  int i;
  list_for_each_entry(item, head, link){
    si = (struct StatusInfo*)(item->object->memory);
    printf("print object %s\n", item->name);
    printf("downloaded metaobject->pid: %d\n", si->pid);
    for(i=0; i<MSTONE_NUM; i++){
      printf("tasks[%d], estime[%d] = %d, %zu\n", i, i, si->tasks[i], si->estime[i]);
    }
  }

  return 0;
}

// for debugging purpose
int print_dataobjects(struct list_head *head)
{
    if(!head) return -1;

    int i;
    struct ObjectList *item;
    list_for_each_entry(item, head, link){
      printf("print object %s\n", item->name);
      print_object(item->object);
    }

    return 0;
}

int print_object(struct MemoryStruct *object)
{
  if(!object) return -1;
  
  int i;
  printf("Object size = %zu\n", object->bytesize/sizeof(double));
  int printsize = (55<(object->bytesize/sizeof(double))?55:(object->bytesize/sizeof(double)));
  for(i=0; i<printsize; i++){
    printf("downloaded dataobject[%d]: %.2f\n", i,((double*)(object->memory))[i]);
  }

  return 0;
}

/*==========================================================*/
// name can be destroyed after this func, since it is copied
static int _register_name(char *name, struct list_head *head)
{
    if( !name || !head) return -1;

    struct NameList *on = (struct NameList *)calloc(1, sizeof(struct NameList));
    if(!on) return -1;

    on->name= strdup(name);
    if(!on->name) return -1;
    list_add(&(on->link), head);

    return 0;
}

static int _register_object(struct MemoryStruct *object, char *oname, struct list_head *head)
{
    if( !object || !head) return -1;

    struct ObjectList *obj = (struct ObjectList *)calloc(1, sizeof(struct ObjectList));
    if(!obj) return -1;

    obj->object = object;
    obj->name   = strdup(oname);
    if(!obj->name) return -1;
    list_add(&(obj->link), head);

    return 0;
}

#if 0
int print_clist(const char *token, const char *url)
{
  char cmd[COMMAND_LINE_SIZE];
  char buff[256];
  sprintf(cmd,"curl -v -X GET -H 'X-Auth-Token: %s' %s",token, url);
  printf("print_clist cmd: %s\n",cmd);    
  //system(cmd);
  FILE *fp = popen(cmd,"r");
    while(fgets(buff, sizeof buff, fp) != NULL){
        printf("result: %s\n", buff);
    }
    pclose(fp);
    return 0;
}

int print_olist(const char *token, const char *url, const char *cname)
{
    char cmd[COMMAND_LINE_SIZE];
    char buff[256];
    sprintf(cmd,"curl -v -X GET -H 'X-Auth-Token: %s' %s/%s",token, url, cname);
    printf("print_olist cmd: %s\n",cmd);    
    //system(cmd);
    FILE *fp = popen(cmd,"r");
    while(fgets(buff, sizeof buff, fp) != NULL){
        printf("result: %s\n", buff);
    }
    pclose(fp);
    return 0;
}
#endif

#if 0
// Jian's implementation
// function returns address of local variable [-Wreturn-local-addr]
struct MemoryStruct* download_object(const char *token, const char *url, const char *cname, const char *oname)
{
    struct curl_slist *headers = NULL;
    CURL *curl;
    CURLcode res;
    int iResult = 0;
    curl = curl_easy_init();
    char xtoken_str[80] = "X-Auth-token:";
    char xurl_str[128] = "";

    struct MemoryStruct sData;
    sData.memory = malloc(1);
    sData.size = 0;    

    if(curl){

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
        curl_easy_setopt(curl, CURLOPT_CAPATH, NULL);

        headers = curl_slist_append(headers, "-X -H");
        sprintf(xtoken_str, "%s%s", xtoken_str, token);
        headers = curl_slist_append(headers, xtoken_str);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&sData);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        sprintf(xurl_str, "%s%s", url, "/");
        sprintf(xurl_str, "%s%s", xurl_str, cname);
        //printf("xurl_str: %s\n",xurl_str);
        sprintf(xurl_str, "%s%s", xurl_str, "/");
        sprintf(xurl_str, "%s%s", xurl_str, oname);
        curl_easy_setopt(curl, CURLOPT_URL, xurl_str);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            iResult = -1;
        }

        curl_easy_cleanup(curl);

    }
    else{
        iResult = -1;
    }
    if(iResult == 0) 
        return &sData;
    else{ 
        free(sData.memory);
        return NULL;
    }
}
#endif

#if 0
int download_file(const char *token, const char *url, const char *cname)
{
    char cmd[COMMAND_LINE_SIZE];
    char buff[256];
    char filename[128] = {0};

    int fd; 

    fd = open (cname, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE
            ,  S_IRUSR | S_IWUSR
            | S_IRGRP | S_IWGRP
            | S_IROTH | S_IWOTH );
    if(fd == -1){
        fprintf(stderr, "open file failed\n");
        return -1;
    }


    sprintf(filename,"%s.metadata", cname);

    sprintf(cmd,"curl -k -v -X GET -H 'X-Auth-Token: %s' %s/%s",token, url, filename);
    printf("get_list cmd: %s\n",cmd);
    //system(cmd);
    struct MemoryStruct *tmpData = NULL;
    FILE *fp = popen(cmd,"r");
    while(fgets(buff, sizeof buff, fp) != NULL){
        printf("result: %s\n", buff);
        tmpData = download_object(token, url, filename, buff);
        if(tmpData) {
            write(fd,tmpData->memory,tmpData->bytesize);
            free(tmpData->memory);
        }
    }
    //free(tmpData->memory);

    pclose(fp);
    close(fd);
    return 0;


}
#endif

#if 0
char* get_tokenid()
{
    char *token = malloc(TOKEN_STR_SIZE*sizeof(char));    
    FILE *file = fopen(filename, "r");
    if(file != NULL){
        if(fgets(token, TOKEN_STR_SIZE, file) != NULL){
            token[strlen(token)-1] = '\0';
            fclose(file);
            return token;
        }
        else{
            fclose(file);
            return NULL;
        }
    }
    else{
        fclose(file);
        return NULL;
    }
}

char* get_purl()
{
    char *purl = malloc(URL_STR_SIZE*sizeof(char));
    FILE *file = fopen(filename, "r");
    if(file != NULL){
        if(fgets(purl, TOKEN_STR_SIZE, file) != NULL){
            ;
        }
        else{
            fclose(file);
            return NULL;
        }
        if(fgets(purl, URL_STR_SIZE, file) != NULL){
            purl[strlen(purl)-1] = '\0';
            fclose(file);
            return purl;
        }
        else{
            fclose(file);
            return NULL;
        }
    }
    else{
        fclose(file);
        return NULL;
    }
}
#endif

/***************************************************************************************************/
#if 0
/*
 * This thread will monitor queue levels from all the mstones
 * check every 1-second period: only one thread per process
 *
 * check all mstones' queue level & every resource location will have one such thread
 * VERY AGGRESSIVE
 * Next: send out the "IDLE" info to all its downstreams, but NOT
 *       end site
 *       Including: 1) intra-resource location processes
 *                  2) inter-resource location processes
 *
 * This is a thread. 
 * */
  void *
upload_statdat(void *in)
{
  if( !getenv("ADAPTIVE") ){
    printf("*NONE ADAPTIVE MODE. @.@\n\n");
    return NULL;
  }
  // to control when the monitor action will start in the system
  _monitor_barrier(client_data);

  /* The following is to demonstrate a simple adaptive strategy */
  // to monitor the workflow execution progress
  adamsg adamsg_info; 
  qstat_report qr;
  struct MemoryStruct sdata;
  struct StatusInfo si;
  while(1){
    construct_statobj(&sdata, &si);
    upload_statusdat(token, purl, &si, &sdata);
    ADAPTIVE_MONITOR_PERIOD;

#if 0
    pthread_mutex_lock(&broadcast_lock);
    if(to_broadcast){
      // upload to cloud
      // to synchronize all the other peers?
      construct_statobj(&sdata, &si);
      upload_statusdat(token, purl, &si, &sdata);

      // normal peer to peer talk
      obtain_my_qstat_report(&qr);
      if( is_emptyQ(&qr) ){
        adamsg_info.resourceID = client_data->itself;
        adamsg_info.rankID = client_data->rank;
        adamsg_info.type = IDLE_BROADCAST;
        printf("!!!R#%d,r#%d Broadcasting idlestat\n", client_data->itself, client_data->rank);
        broadcast_idlestat(&adamsg_info, adamsg_format_list);
        to_broadcast = FALSE;
      }

    }
    pthread_mutex_unlock(&broadcast_lock);
    ADAPTIVE_MONITOR_PERIOD;
#endif

  }

  return NULL;
}
#endif

void *
download_statdat(void *in)
{
  char cname[] = "stat_con";
  struct list_head* olist = NULL;
  struct StatusInfo* si = NULL;
  int i;

  while(1){
    olist = _download_statusobjs(token, purl, cname);
    if(!olist) return NULL;
    si    = id_offloader(olist);
    if(!si) return NULL;

    for(i=0; i<MSTONE_NUM; i++){
      printf("tasks[%d], estime[%d] = %d, %zu\n", i, i, si->tasks[i], si->estime[i]);
    }

    sleep(6);
  }

  return NULL;
}

struct list_head* _download_statusobjs(const char *token, const char *url, const char *cname)
{
  struct list_head* olist = download_cobjects(token, url, cname);
  if(!olist) return NULL;

  return olist;
}

struct StatusInfo*
id_offloader(struct list_head* olist)
{
  if(!olist) return NULL;

  struct StatusInfo *progresses[MAX_INSTANCES];
  struct ObjectList *item;
  struct StatusInfo *si;
  int i, k = 0;
  list_for_each_entry(item, olist, link){
    si = (struct StatusInfo*)(item->object->memory);
    progresses[k++] = si;

    printf("print object %s\n", item->name);
    printf("downloaded metaobject->pid: %d\n", si->pid);
    for(i=0; i<MSTONE_NUM; i++){
      printf("tasks[%d], estime[%d] = %d, %zu\n", i, i, si->tasks[i], si->estime[i]);
    }
  }
  si = _id_offloader(k, progresses);

  return si;
}

struct StatusInfo*
_id_offloader(int size, struct StatusInfo **progresses)
{
  if(!progresses) return NULL;

  int i, j;
  int thisqlevel = 0, maxV = INVILID_INT, maxI = INVILID_INT;
  for(i=0; i<size; i++){
    for(j=0; j<MSTONE_NUM; j++){
      thisqlevel += progresses[i]->tasks[j];
    }
    if(thisqlevel > maxV){
      maxV = thisqlevel;
      maxI = i;
    }
    thisqlevel = 0;
  }

  return progresses[maxI];
}

