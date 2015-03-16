#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "evpath.h"
#include "ev_dfg.h"
#include "dfg_functions.h"
#include "khan_ffs.h"

extern FMField simple_field_list[];
extern FMStructDescRec simple_format_list[]; 

EVclient test_client;
EVsource source_handle;

static int
inter_handler(CManager cm, void *vevent, void *client_data, attr_list attrs)
{
    printf("Received an event\n");
    return 1;
}

/* this file is evpath/examples/dfg_client3.c */
int main(int argc, char **argv)
{
    CManager cm;
    EVclient_sinks sink_capabilities;
    EVclient_sources source_capabilities;
    cm = CManager_create();
    CMlisten(cm);

    char master_address[200];
    dfg_get_master_contact_func(master_address,"master.info");
/*
**  LOCAL DFG SUPPORT   Sources and sinks that might or might not be utilized.
*/

    source_handle = EVcreate_submit_handle(cm, -1, simple_format_list);
    source_capabilities = EVclient_register_source("inter_source", source_handle);
    sink_capabilities = EVclient_register_sink_handler(cm, "sink_c", simple_format_list,
				(EVSimpleHandlerFunc) inter_handler, NULL);

    /* We're node argv[1] in the process set, contact list is argv[2] */
    test_client = EVclient_assoc(cm, argv[1], master_address, NULL, sink_capabilities);

	if (EVclient_ready_wait(test_client) != 1) {
	/* dfg initialization failed! */
	exit(1);
    }

    




    /*! [Shutdown code] */
	CMrun_network(cm);

	return 1;

}

