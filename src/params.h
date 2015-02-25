// There are a couple of symbols that need to be #defined before
// #including all the headers.

#ifndef _PARAMS_H_
#define _PARAMS_H_
#define FUSE_USE_VERSION 26

#include <limits.h>
#include <stdio.h>
struct khan_state 
{
    char *rootdir;
    int uid;
    int gid;
    int pid;
};
#define KHAN_DATA ((struct khan_state *) fuse_get_context()->private_data)
#define UID (fuse_get_context()->uid)
#define PID (fuse_get_context()->pid)
#define GID (fuse_get_context()->gid)


#endif
