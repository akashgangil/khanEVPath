// Author: Jian Huang, Hongbo Zou
// Georgia Institue of Technology

#ifndef __CLOUDUPLOAD_H
#define __CLOUDUPLOAD_H

//#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "list.h"
#include "comm_vars.h" // for MSTONE_NUM

#define COMMAND_LINE_SIZE 210
#define TOKEN_STR_SIZE 64
#define URL_STR_SIZE 80
#define ONAME_STR_SIZE 35
#define CNAME_STR_SIZE 35

static const char filename[] = "tfile";

struct StatusInfo{ // meta-data
    int cid;
    int pid;
    int tasks[MSTONE_NUM]; // 3 is the total phase count
    size_t estime[MSTONE_NUM];
};

struct MemoryStruct{
    char *memory; // struct StatusInfo or inter-mediate data
    size_t bytesize;
};

struct ObjectList{
    struct MemoryStruct *object;
    char *name;
    struct list_head link;
};

struct NameList{
    char *name;
    struct list_head link;
};

int   get_token(const char *tname, const char *username, const char *pswd, const char *addr, char **token, char **url);
#if 0
char* get_tokenid();
char* get_purl();
#endif
int   create_container(const char *token, const char *url, const char *cname);
int   delete_container(const char *token, const char *url, const char *cname);
int   upload_object(const char *token, const char *url, const char *cname, const char *oname, struct MemoryStruct *sData);
#if 0
int   download_file(const char *token, const char *url, const char *cname);
#endif
#if 0
int print_olist(const char *token, const char *url, const char *cname);
int print_clist(const char *token, const char *url);
#endif
struct MemoryStruct* download_object(const char *token, const char *url, const char *cname, const char *oname);
struct list_head* download_cobjects(const char *token, const char *url, const char *cname);
int delete_object(const char *token, const char *url, const char *cname, const char *oname);
int delete_cobjects(const char *token, const char *url, const char *cname);

struct list_head* get_olist(const char *token, const char *url, const char *cname);
struct list_head* get_clist(const char *token, const char *url);
int print_namelist(struct list_head *head);
int free_namelist(struct list_head *head);

int print_metaobjects(struct list_head *head);
int print_dataobjects(struct list_head *head);
int free_objects(struct list_head *head);
int print_object(struct MemoryStruct *object);

/***************************************************************************************************/
void *download_statdat(void *in);
struct list_head* _download_statusobjs(const char *token, const char *url, const char *cname);
struct StatusInfo* id_offloader(struct list_head* olist);
struct StatusInfo* _id_offloader(int size, struct StatusInfo **progresses);

#endif
