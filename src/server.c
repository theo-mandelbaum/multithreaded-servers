#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "dhcp.h"
#include "format.h"
#include "port_utils.h"
#include "server.h"

#define MAX_IPS 5

void
echo_server ()
{
  int socketfd = socket (AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons ((uint16_t)strtol (get_port (), NULL, 10));
  addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  bind (socketfd, (struct sockaddr *)&addr, sizeof (addr));
  ssize_t length = sizeof (msg_t) + sizeof (options_t);
  uint8_t *buffer = calloc (length, sizeof (uint8_t));
  socklen_t addrlen = sizeof(addr);
  ssize_t nbytes = recvfrom(socketfd, buffer, length, 0, (struct sockaddr *)&addr, &addrlen);
  msg_t *recv_msg = (msg_t *)buffer;
  msg_t *msg_response = server_reply_gen ();
  // dump_msg();
  options_t options;
  get_options (buffer, buffer + nbytes - 1, &options);

  free (buffer);
  close (socketfd);
  
}

msg_t
server_reply_gen ()
  {
    msg_t newMsg;
    memset (&newMsg, 0, sizeof (msg_t));
    newMsg.op = 2;
    newMsg.xid = htonl (42);
    newMsg.htype = ETH;
    return newMsg;
  }