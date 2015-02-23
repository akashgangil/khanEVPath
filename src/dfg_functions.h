
#ifndef DFG_FUNCTIONS_H
#define DFG_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include "evpath.h"
#include "ev_dfg.h"

#define MAXNODES 25
#define MAXSTONES 25

struct source_stone_unit {
  EVdfg_stone src;
  char* sourcename;
};

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
int dfg_finalize_func(void);
void dfg_get_master_contact_func(char *retvalue);

#endif

