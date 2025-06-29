#ifndef __cs361_dhcp_server_h__
#define __cs361_dhcp_server_h__

#include <stdbool.h>
#include <stdint.h>

#include "dhcp.h"


void echo_server(int);
void echo_server_thread(int);

extern bool debug;
extern struct in_addr THIS_SERVER;

#endif
