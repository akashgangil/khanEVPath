#ifndef __CLOUDUPLOAD_SUPPLEMENT_H
#define __CLOUDUPLOAD_SUPPLEMENT_H

#include "list.h"

struct PairStruct{
    char *name;
    char *value;
    struct list_head link;
};

/* to init all the key/value pairs regarding access to the object store*/
extern struct list_head PARSHEAD;
extern struct PairStruct pars[4];

extern char *token;
extern char *purl;

int prepare_cloudaccess();
int init_pairstructs(struct list_head *parshead);
int init_cloudaccess(struct list_head *parshead);

#endif
