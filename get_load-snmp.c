/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*** Do I need to give copyright to the X Consortium? ***/

/*
 * get_load-snmp - get system load using SNMP queries (so not actually
 * limited to load monitoring)
 * Heavily influenced by the original get_load.c from xload
 *
 * Author: Adrian Hosey <ahosey@systhug.com>
 *
 * Call InitLoadPoint() to initialize.
 * GetLoadPoint() is a callback for the StripChart widget.  */

#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#include <stdio.h>

#include "xload-snmp.h"

/* These are all for doing SNMP. We'll hold them up here because they
 * only need to be initialized once in the life of the program. 
 */
static struct snmp_session session;
static struct snmp_session *ss = NULL;
static snmp_params *my_params;
static oid anOID[MAX_OID_LEN];
static size_t anOID_len = MAX_OID_LEN;

#define COMMUNITY_STR_LEN 256

static xload_error(str1, str2)
     char *str1, *str2;
{
  (void) fprintf(stderr,"xload: %s %s\n", str1, str2);
#ifdef __bsdi__
  if (kd)
    kvm_close(kd);
#endif
  exit(-1);
}


/* Read the community string out of the given filename. */
char* slurp_community_string(char* filename) {
    char* stringbuf;
    FILE* fp = NULL;
    int i;

    if ((stringbuf = (char*)malloc(sizeof(char) * (COMMUNITY_STR_LEN + 1)))
	== NULL) {
	xload_error("Couldn't allocate memory for community", "string");
	exit(-1);
    }
    if ((fp = fopen(filename, "r")) == NULL) {
	xload_error("Couldn't read community string from", filename);
	exit(-1);
    }
    while (fgets(stringbuf, COMMUNITY_STR_LEN + 1, fp))
	if (stringbuf[0] != '#') 
	    break;
    
    for (i = 0; i < COMMUNITY_STR_LEN + 1; i++)
	if (stringbuf[i] == '\n') {
	    stringbuf[i] = '\0';
	    break;
	}

    return stringbuf;
}
	

/* This should return something other than void. We're being rather
 * blithe about error checking right now. */
void InitLoadPoint(snmp_params *closure) {

    /* Initialize the library */
    init_snmp("xload-snmp");
    
    /* Set up some defaults for the session */
    my_params = closure;
    snmp_sess_init(&session);
    session.version = SNMP_VERSION_1;
    session.peername = my_params->peername;
    session.community = slurp_community_string(my_params->community);
    session.community_len = strlen(session.community);
    get_node(my_params->oid, anOID, &anOID_len);
    
    ss = snmp_open(&session);
    return;
}


void GetLoadPoint_SNMP( w, closure, call_data)
     Widget   w;                   /* unused */
     caddr_t  closure;             /* pointer to snmp_params struct */
     caddr_t  call_data;           /* pointer to (double) return value */
{
    struct snmp_pdu *pdu, *response;
    struct variable_list *vars;
    int status;

    /* if (ss == NULL) initialize_xload_snmp((snmp_params*)closure); */
    if (ss == NULL) {
      xload_error("SNMP session didn't get initialized, bail", "out");
      exit(-1);
    }

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, anOID, anOID_len);
  
    status = snmp_synch_response(ss, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
	vars = response->variables;
	*(double*)call_data = *(vars->val.floatVal);
    }
    else {
	if (status == STAT_SUCCESS)
	    fprintf(stderr, "Error in packet\nReason: %s\n",
		    snmp_errstring(response->errstat));
	else
	    snmp_sess_perror("snmpget", ss);
	*(double*)call_data = 0.0; /* There was a problem, so plot 0.0 */
    }
  
    if (response)
	snmp_free_pdu(response);
    /* Since we don't want snmp_close() here in the callback, where do
     * we want it? Back in main()? Pudoo. */
    return;
}


