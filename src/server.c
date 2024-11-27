#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>

#include "dhcp.h"
#include "format.h"
#include "port_utils.h"
#include "server.h"

#define MAX_IPS 5

msg_t *server_reply_gen (uint8_t *, uint8_t);
int address_len_helper (int);
struct sockaddr_in address_gen ();

void
echo_server ()
{
  int socketfd = socket (AF_INET, SOCK_DGRAM, 0);

  // Create my address struct using a helper
  struct sockaddr_in addr = address_gen ();

  // Set reusable socket options
  int socket_option = 1;
  setsockopt (socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &socket_option, sizeof (int));

  // Set timeout socket options
  struct timeval timeout = { 10, 0 };
  setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (const void *) &timeout, sizeof (timeout));

  // bind the socket to the client
  if (bind (socketfd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
        perror("Bind failed");
        close(socketfd);
        exit(EXIT_FAILURE);
    }

  // Set up the length of the request that the server receives
  size_t length = sizeof (msg_t) + sizeof (options_t);
  // size_t length = 1024;
  uint8_t *recv_buffer = calloc (length, sizeof (uint8_t));
  socklen_t addrlen = sizeof(addr);

  ssize_t nbytes = recvfrom (socketfd, recv_buffer, length, 0, (struct sockaddr *)&addr, &addrlen);

  // printf ("%ld\n", nbytes);

  // dump_packet (recv_buffer, length);

  // dump_msg (stdout, recv_buffer, nbytes);

  // Create our BOOTP data from the received request
  msg_t *recv_msg = (msg_t *)recv_buffer;

  // dump_msg (stdout, recv_buffer, length);

  options_t options;
  options.request = calloc (1, sizeof (struct in_addr));
  if (options.request == NULL) {
    perror("calloc failed");
    exit(1);
  }

  options.lease = calloc (1, sizeof(uint32_t));
  if (options.lease == NULL) {
    perror("calloc failed");
    exit(1);
  }

  options.type = calloc (1, sizeof(uint8_t));
  if (options.type == NULL) {
    perror("calloc failed");
    exit(1);
  }

  options.sid = calloc (1, sizeof(struct in_addr));
  if (options.sid == NULL) {
    perror("calloc failed");
    exit(1);
  }

  printf ("optopn type = %d\n", *options.type);

  printf ("lease time = %d\n", *options.lease);

  char req_buffer[INET_ADDRSTRLEN];
  inet_ntop (AF_INET, options.request, req_buffer, sizeof (req_buffer));
  printf ("requst ip = %s\n", req_buffer);

  char sid_buffer[INET_ADDRSTRLEN];
  inet_ntop (AF_INET, options.sid, sid_buffer, sizeof (sid_buffer));
  printf ("server id = %s\n", sid_buffer);

  // memset (&options, 0, sizeof (options));
  get_options (recv_buffer, recv_buffer + nbytes - 1, &options);

  // Construct the BOOTP response
  msg_t *msg_response = server_reply_gen (recv_msg->chaddr, recv_msg->htype);

  // dump_msg (stdout, msg_response, sizeof (msg_t));

  size_t send_size = sizeof (msg_t);
  uint8_t *send_buffer = calloc (send_size, sizeof (uint8_t));
  memcpy (send_buffer, msg_response, sizeof (msg_t));

  // Add all of the dhcp data to the response buffer
  send_buffer = append_cookie (send_buffer, &send_size);

  printf ("optopn type = %d\n", *options.type);

  printf ("lease time = %d\n", *options.lease);

  // char req_buffer[INET_ADDRSTRLEN];
  inet_ntop (AF_INET, options.request, req_buffer, sizeof (req_buffer));
  printf ("requst ip = %s\n", req_buffer);

  // char sid_buffer[INET_ADDRSTRLEN];
  inet_ntop (AF_INET, options.sid, sid_buffer, sizeof (sid_buffer));
  printf ("server id = %s\n", sid_buffer);


  // If the request id was set, then append it to the response
  if (options.request != NULL)
  {
    send_buffer = append_option(send_buffer, &send_size, DHCP_opt_reqip, sizeof (struct in_addr), (uint8_t *)options.request);
  }

  // If the lease time was set, then append it to the response
  if (options.lease != NULL)
  {
    send_buffer = append_option(send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)options.lease);
  }

  // If the message type was set, then append it to the response
  if (options.type != NULL)
  {
    send_buffer = append_option(send_buffer, &send_size, DHCP_opt_msgtype, 1, options.type);
  }

  // If the server id was set, then append it to the response
  if (options.sid != NULL)
  {
    send_buffer = append_option(send_buffer, &send_size, DHCP_opt_sid, sizeof (struct in_addr), (uint8_t *)options.sid);
  }


  send_buffer = append_option(send_buffer, &send_size, DHCP_opt_end, 0, NULL);

  // Sending my response back to the client
  sendto (socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr, addrlen);

  printf ("first free\n");
  free (msg_response);
  printf ("second free\n");
  free (recv_buffer);
  printf ("third free\n");
  free (send_buffer);
  close (socketfd);

}

msg_t*
server_reply_gen (uint8_t *recv_chaddr, uint8_t recv_htype)
  {
    msg_t *newMsg = calloc(sizeof(msg_t), sizeof (uint8_t));
    memset(newMsg, 0, sizeof(msg_t));
    newMsg->op = 2;
    newMsg->htype = recv_htype;
    newMsg->hlen = address_len_helper(newMsg->htype);
    newMsg->xid = htonl(0);
    inet_pton(AF_INET, "192.168.1.0", &newMsg->yiaddr);
    memcpy(newMsg->chaddr, recv_chaddr, 16); // Hardware address
    return newMsg;
  }

struct sockaddr_in
address_gen ()
  {
    struct sockaddr_in addr;
    memset (&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs ((uint16_t)strtol (get_port (), NULL, 10));
    addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
    return addr;
  }

int
address_len_helper (int htype)
{
  if (htype == ETH)
    return ETH_LEN;
  if (htype == IEEE802)
    return IEEE802_LEN;
  if (htype == ARCNET)
    return ARCNET_LEN;
  if (htype == FRAME_RELAY)
    return FRAME_LEN;
  if (htype == FIBRE)
    return FIBRE_LEN;
  return 0;
}