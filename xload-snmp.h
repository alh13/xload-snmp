
/* Common needs for doing SNMP */

#include <ucd-snmp/ucd-snmp-config.h>
#include <ucd-snmp/ucd-snmp-includes.h>

typedef struct {
  char* peername;
  char* community;
  char* oid;
  float factor;
} snmp_params;

