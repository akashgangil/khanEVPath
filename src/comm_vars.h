#ifndef COMM_VARS_H
#define COMM_VARS_H

#ifndef MATLAB_WRF
#define MATLAB_WRF
#endif

#ifndef M_DEBUG
#define M_DEBUG
#endif

#ifndef CORE_DUMP_DIR
#define CORE_DUMP_DIR
#endif

#ifndef DEBUG
#define DEBUG
#endif

#if 0
#ifndef TESTING
#define TESTING
#endif
#endif

#ifndef NO_SEP_FINISHED_MSGS
#define NO_SEP_FINISHED_MSGS
#endif

#define DOWNSTREAM 100
#define UPSTREAM 59

#define TRUE 1
#define FALSE 0

#define INVILID_INT (-1337)

#define TOEXEC 1111
#define TOOUTPUT 2222

#define PLUS 1
#define MINUS -1

#define MSTONE_NUM 3 /* the max num of mstones in the system */
#define MSTONE_NUM_STR "3" /* XXX update this with MSTONE_NUM in the system */

#define IMAGE_COUNT 654321 
#define GNUM 123456 /* total num of groups in the system */
#define GNUM_STR "123456" /* XXX update this with GNUM, used by evpath */
#define PERGSIZE 1 /* the num of members in each group */
#define OBJ_PER_STREAM 678910 

#define NUM_RESOURCES 4 /* to define the maximal capacity */
#define NUM_PROCESSES 100 /* to define the maximal processing instances per site */
#define NUM_DG 500 /* to define the maximal DG processes  */

#define MAX_INSTANCES (NUM_RESOURCES*NUM_PROCESSES) /* to bound the total num of processing instances in the whole system */

#define MAX_NUM_COMPLETE_GROUPS GNUM
//#define MAX_NUM_COMPLETE_GROUPS MAX_GNUM
#define QL_THRESHOLD 0.5

#define IDLE_BROADCAST 2345
#define REDIRECT_REQUEST 5678

#define AQLEVEL(q0, q1, q2) ( ((!q0) && (!q1) && (!q2)) == 1 ? 1:0 )


#define CONTACT_INFO_FILE "/net/hp100/ihpcae/yanwei/contactfiles/contact_info.txt"
#define TEMP_CONTACT_INFO_FILE "/net/hp100/ihpcae/yanwei/contactfiles/temp_contact_info.txt"
#define FORUPSTREAM_CONTACT_READY_FILE "/net/hp100/ihpcae/yanwei/contactfiles/forupstream_contact_ready.txt"

#define TOTAL_RESOURCES 3 /* pure processing sites + end sites */
enum Resource_Station{
  ENDSITE,
  SITEONE,
  SITETWO,
  DG
};
extern enum Resource_Station clusterid;

enum DataName{
  XPOS    = 0,
  YPOS    = 1,
  BWMEANC = 2,
  IM      = 3,
  
  BWS1    = 4,
  I1      = 5,

  EDGE    = 6,

  XFIT    = 7,
  YFIT    = 8,
  CFIT    = 9,
  SFIT    = 10,
  KAPPA   = 11
};

#define SIZES 6,\
              1,\
              5
extern int VARSIZES[3];

#define CHECK_GSTAT_PERIOD sleep(0)
#define ADAPTIVE_MONITOR_PERIOD sleep(5)

#define BUFSIZE 10240
#define MAX_MFIELD 15
#define MAX_MFIELD_STR "15"

#endif

