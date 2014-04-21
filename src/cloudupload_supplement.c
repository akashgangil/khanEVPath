#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cloudupload_supplement.h"
#include "list.h"
#include "cloudupload.h"

char *token = NULL;
char *purl  = NULL;
struct PairStruct pars[4];


// to get the token & url for future cloud access
int prepare_cloudaccess()
{
    LIST_HEAD(PARSHEAD);
    init_pairstructs(&PARSHEAD);
    init_cloudaccess(&PARSHEAD);

    return 0;
}

int init_pairstructs(struct list_head *parshead)
{
    if(!parshead) return -1;

    pars[0].name = strdup("tname");
    pars[0].value = strdup("test");
    if(!pars[0].name || !pars[0].value){
        return -1;
    }

    pars[1].name = strdup("username");
    pars[1].value = strdup("tester");
    if(!pars[1].name || !pars[1].value){
        return -1;
    }

    pars[2].name = strdup("pswd");
    pars[2].value = strdup("testing");
    if(!pars[2].name || !pars[2].value){
        return -1;
    }

    pars[3].name = strdup("addr");
    pars[3].value = strdup("127.0.0.1");
    //pars[3].value = strdup("143.215.131.232");
    //pars[3].value = strdup("shale.cc.gt.atl.ga.us");
    //pars[3].value = strdup("http://127.0.0.1:8080/v1.0");
    if(!pars[3].name || !pars[3].value){
        return -1;
    }

    int i;
    for(i=0; i<4; i++){
        list_add(&(pars[i].link), parshead);
    }

    return 0;
    
}

int init_cloudaccess(struct list_head *parshead)
{
    if(!parshead) return -1;

    char *tname, *username, *pswd, *addr;

    struct PairStruct *item;
    list_for_each_entry(item, parshead, link){
        if (!strcasecmp (item->name, "tname")){
            tname = item->value;
        }
        else if(!strcasecmp(item->name, "username")){
            username = item->value;
        }
        else if(!strcasecmp(item->name, "pswd")){
            pswd = item->value;
        }
        else if(!strcasecmp(item->name, "addr")){
            addr = item->value;
        }
    }

    // cloudupload initialization
     
    if(get_token(tname, username, pswd, addr, &token, &purl) == 0){
        printf("[cloudupload] get_token success.\n");
        printf("[cloudupload] token: %s\n",token);
        printf("[cloudupload] purl: %s\n",purl);
    }
    else{
        printf("[cloudupload] Error: Cannot get token\n");
    }
     
    // cloudupload initialization end

    return 0;

}
