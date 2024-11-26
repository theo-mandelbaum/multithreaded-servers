#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

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
  if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(socketfd);
        exit(EXIT_FAILURE);
    }

  // Set up the length of the request that the server receives
  size_t length = sizeof (msg_t) + sizeof (options_t);
  uint8_t *recv_buffer = calloc (length, sizeof (uint8_t));
  socklen_t addrlen = sizeof(addr);

  // Receive the request
  size_t nbytes = recvfrom(socketfd, recv_buffer, length, 0, (struct sockaddr *)&addr, &addrlen);
  if (nbytes == -1)
    {
      perror ("recvfrom failed");
      close (socketfd);
      exit (EXIT_FAILURE);
    }
  printf ("recieved message\n");

  // Create our BOOTP data from the received request
  msg_t *recv_msg = (msg_t *)recv_buffer;

  // dump_packet (recv_buffer, length);

  // Construct the BOOTP response
  msg_t *msg_response = server_reply_gen (recv_msg->chaddr, recv_msg->htype);
  size_t send_size = sizeof (msg_t);
  uint8_t *send_buffer = calloc (send_size, sizeof (uint8_t));
  memcpy (send_buffer, (uint8_t *)msg_response, sizeof (msg_t));

  printf ("generated server reply\n");

  // printf ("");
  options_t options;
  get_options (recv_buffer, recv_buffer + nbytes - 1, &options);
  printf ("get_options succeeded\n");

  // Add all of the dhcp data to the response buffer
  send_buffer = append_cookie (send_buffer, &send_size);

  if (options.type != NULL)
  {
    append_option(send_buffer, &send_size, DHCP_opt_msgtype, 1, options.type);
  }

  if (options.request != NULL)
  {
    append_option(send_buffer, &send_size, DHCP_opt_reqip, sizeof(struct in_addr), (uint8_t *)options.request);
  }

  if (options.lease != NULL)
  {
    append_option(send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)options.lease);
  }

  if (options.sid != NULL)
  {
    append_option(send_buffer, &send_size, DHCP_opt_sid, sizeof(uint32_t), (uint8_t *)options.sid);
  }

  append_option(send_buffer, &send_size, DHCP_opt_end, 0, NULL);

  uint8_t *test_buffer = calloc (1, sizeof (uint8_t));
  uint8_t cpy_buf = 5;
  memcpy (test_buffer, &cpy_buf, 1);
  sendto (socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr, addrlen);
  // sendto (socketfd, test_buffer, 1, 0, (struct sockaddr *)&addr, addrlen);

  printf ("first free\n");
  free (msg_response);
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
    newMsg->xid = htonl(42);
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