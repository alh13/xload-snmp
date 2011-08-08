
/* Common needs for doing SNMP */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

typedef struct {
  char* peername;
  char* community;
  char* oid;
  float factor;
  Boolean delta;
} snmp_params;

