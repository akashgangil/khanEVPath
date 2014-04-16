#define MAX_LEN 4096 

#define FUSE_USE_VERSION 26
#define MAX_PATH_LENGTH 2048

static char command[MAX_PATH_LENGTH];
static struct khan_state *khan_data=NULL;

time_t time_now;
char * fpath=NULL;
mode_t khan_mode=S_ISUID | S_ISGID | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
char * temp=NULL;
char * temp2=NULL;
char *args=NULL;
char msg[4096];
int timestamp;

