
#ifndef DFG_FUNCTIONS_H
#define DFG_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <string.h>
#include <sys/time.h>
#include <vector>

#include "evpath.h"
#include "ev_dfg.h"

#define MAXNODES 25
#define MAXSTONES 25
// The code below is to be merged with Akash's code
typedef enum {SOURCE, SINK} stone_type_t;
typedef enum {COD, PYTHON} code_type_t;
typedef struct _stone_struct {
    std::string node_name;
    std::string stone_name;
    std::string src_sink_handler_name;
    stone_type_t stone_type;
    std::vector<std::string> incoming_stones;
    code_type_t code_type;
    std::string code1;
    std::string code2;

} stone_struct, * stone_struct_ptr;

// The code above is to be merged


typedef struct source_stone_unit {
  EVdfg_stone src;
  EVdfg_stone router;
  int port;
  char* sourcename;
} ss_unit, * ss_unit_ptr;

struct dfg_unit {
  CManager cm;
  EVdfg dfg;
  EVmaster dfg_master;
  char *str_contact;
  int node_count;
  char *nodes[MAXNODES];
  int numsourcestones;
  struct source_stone_unit* srcstone;
  EVclient test_client;
};



int dfg_init_func(void);
int dfg_create_func(char *mode, int n, char **nodelist, EVmasterJoinHandlerFunc func);
int dfg_create_assign_source_stones_func(char *nodename, char *sourcestone);
int dfg_create_assign_link_sink_stones_func(char *nodename, char *handler, int numsources, char **sourcename);
int dfg_finalize_func_static(void);
void dfg_get_master_contact_func(char *retvalue, char *contact_file);
EVdfg_stone create_stone(const stone_struct &stone_info);

#endif

