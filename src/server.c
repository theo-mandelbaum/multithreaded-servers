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

msg_t *server_reply_gen(uint8_t *, uint8_t, uint32_t, options_t, int);
int address_len_helper(int);
struct sockaddr_in address_gen();
void print_options_fields(options_t options);
void reply_type_helper (options_t, uint8_t *, size_t);
int socket_helper (int, struct sockaddr_in, int);

void echo_server()
{
  int socketfd = socket(AF_INET, SOCK_DGRAM, 0);

  // Create my address struct using a helper
  struct sockaddr_in addr = address_gen();

  // Set reusable socket options
  int socket_option = 1;
  setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&socket_option, sizeof(int));

  // Set timeout socket options
  struct timeval timeout = {10, 0};
  setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout));

  // bind the socket to the client
  if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("Bind failed");
    close(socketfd);
    exit(EXIT_FAILURE);
  }


  int count = 1;
  while (count < MAX_IPS)
    {
      if (socket_helper (socketfd, addr, count) == 0)
        break;
      count++;
    }


  // socket_helper (socketfd, addr);

/*
  // Set up the length of the request that the server receives
  size_t length = sizeof(msg_t) + sizeof(options_t);
  // size_t length = 1024;
  uint8_t *recv_buffer = calloc(length, sizeof(uint8_t));
  socklen_t addrlen = sizeof(addr);

  ssize_t nbytes = recvfrom(socketfd, recv_buffer, length, 0, (struct sockaddr *)&addr, &addrlen);

  // Create our BOOTP data from the received request
  msg_t *recv_msg = (msg_t *)recv_buffer;

  options_t options;
  options.request = calloc(1, sizeof(struct in_addr));
  options.request = NULL;

  options.lease = calloc(1, sizeof(uint32_t));
  options.lease = NULL;

  options.type = calloc(1, sizeof(uint8_t));
  options.type = NULL;

  options.sid = calloc(1, sizeof(struct in_addr));
  options.sid = NULL;

  // memset (&options, 0, sizeof (options));
  get_options(recv_buffer + sizeof (msg_t) + sizeof (MAGIC_COOKIE), recv_buffer + nbytes - 1, &options);

  // Construct the BOOTP response
  msg_t *msg_response = server_reply_gen(recv_msg->chaddr, recv_msg->htype, recv_msg->xid, options);

  size_t send_size = sizeof(msg_t);
  uint8_t *send_buffer = calloc(send_size, sizeof(uint8_t));
  memcpy(send_buffer, msg_response, sizeof(msg_t));

  // Add all of the dhcp data to the response buffer
  send_buffer = append_cookie(send_buffer, &send_size);

  // If the request id was set, then append it to the response
  // if (options.request != NULL)
  // {
  //  send_buffer = append_option(send_buffer, &send_size, DHCP_opt_reqip, sizeof(struct in_addr), (uint8_t *)options.request);
  // }

  uint8_t type_val;
  uint32_t lease_val = 0x008D2700;
  if (*options.type == (uint8_t)DHCPDISCOVER)
    {
      type_val = DHCPOFFER;
      send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, sizeof (uint8_t), &type_val);
      send_buffer = append_option (send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)&lease_val);
    }

  if (*options.type == (uint8_t)DHCPREQUEST)
    {
      if (options.sid->s_addr == inet_addr ("192.168.1.0"))
        {
          type_val = DHCPACK;
          send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, sizeof (uint8_t), &type_val);

          send_buffer = append_option (send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)&lease_val);
        }
      else
        {
          type_val = DHCPNAK;
          send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, sizeof (uint8_t), &type_val);
        }
    }

  struct in_addr sid_val;
  sid_val.s_addr = inet_addr ("192.168.1.0");
  send_buffer = append_option (send_buffer, &send_size, DHCP_opt_sid, sizeof (struct in_addr), (uint8_t *)&sid_val);

  send_buffer = append_option (send_buffer, &send_size, DHCP_opt_end, 0, NULL);

  // Sending my response back to the client
  sendto(socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr, addrlen);

  free(msg_response);
  free(recv_buffer);
  free(send_buffer);
  close(socketfd);
  */
}

int
socket_helper (int socketfd, struct sockaddr_in addr, int count)
  {
    for (int i = 0; i < 2; i++)
      {
        // Set up the length of the request that the server receives
        size_t length = sizeof(msg_t) + sizeof(options_t);
        // size_t length = 1024;
        uint8_t *recv_buffer = calloc(length, sizeof(uint8_t));
        socklen_t addrlen = sizeof(addr);

        ssize_t nbytes = recvfrom(socketfd, recv_buffer, length, 0, (struct sockaddr *)&addr, &addrlen);

        if (nbytes < 0)
          return 0;
        // Create our BOOTP data from the received request
        msg_t *recv_msg = (msg_t *)recv_buffer;

        options_t options;
        options.request = calloc(1, sizeof(struct in_addr));
        options.request = NULL;

        options.lease = calloc(1, sizeof(uint32_t));
        options.lease = NULL;

        options.type = calloc(1, sizeof(uint8_t));
        options.type = NULL;

        options.sid = calloc(1, sizeof(struct in_addr));
        options.sid = NULL;

        // memset (&options, 0, sizeof (options));
        get_options(recv_buffer + sizeof (msg_t) + sizeof (MAGIC_COOKIE), recv_buffer + nbytes - 1, &options);

        // Construct the BOOTP response
        msg_t *msg_response = server_reply_gen(recv_msg->chaddr, recv_msg->htype, recv_msg->xid, options, count);

        size_t send_size = sizeof(msg_t);
        uint8_t *send_buffer = calloc(send_size, sizeof(uint8_t));
        memcpy(send_buffer, msg_response, sizeof(msg_t));

        // Add all of the dhcp data to the response buffer
        send_buffer = append_cookie(send_buffer, &send_size);

        // If the request id was set, then append it to the response
        // if (options.request != NULL)
        // {
        //  send_buffer = append_option(send_buffer, &send_size, DHCP_opt_reqip, sizeof(struct in_addr), (uint8_t *)options.request);
        // }
      
        uint8_t type_val;
        uint32_t lease_val = 0x008D2700;
        if (*options.type == (uint8_t)DHCPDISCOVER)
          {
            type_val = DHCPOFFER;
            send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, sizeof (uint8_t), &type_val);
            send_buffer = append_option (send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)&lease_val);
          }

        if (*options.type == (uint8_t)DHCPREQUEST)
          {
            if (options.sid->s_addr == inet_addr ("192.168.1.0"))
              {
                type_val = DHCPACK;
                send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, sizeof (uint8_t), &type_val);

                send_buffer = append_option (send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)&lease_val);
              }
            else
              {
                type_val = DHCPNAK;
                send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, sizeof (uint8_t), &type_val);
              }
          }

        struct in_addr sid_val;
        sid_val.s_addr = inet_addr ("192.168.1.0");
        send_buffer = append_option (send_buffer, &send_size, DHCP_opt_sid, sizeof (struct in_addr), (uint8_t *)&sid_val);

        send_buffer = append_option (send_buffer, &send_size, DHCP_opt_end, 0, NULL);

        // Sending my response back to the client
        sendto(socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr, addrlen);

        free(msg_response);
        free(recv_buffer);
        free(send_buffer);
        close(socketfd);
        return 1;
      }
  }

void
reply_type_helper (options_t options, uint8_t *send_buffer, size_t send_size)
  {
    if (*options.type == DHCPDISCOVER)
      send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, 1, (uint8_t *)DHCPOFFER);
    
    if (*options.type == DHCPREQUEST)
      send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype, 1, (uint8_t *)DHCPACK);
  }

msg_t *
server_reply_gen(uint8_t *recv_chaddr, uint8_t recv_htype, uint32_t recv_xid, options_t options, int count)
{
  msg_t *newMsg = calloc(sizeof(msg_t), sizeof(uint8_t));
  memset(newMsg, 0, sizeof(msg_t));
  newMsg->op = 2;
  newMsg->htype = recv_htype;
  newMsg->hlen = address_len_helper(newMsg->htype);
  newMsg->xid = recv_xid;
  struct in_addr condition_addr;
  condition_addr.s_addr = inet_addr ("192.168.1.0");
  if (*options.type == (uint8_t)DHCPREQUEST && options.sid->s_addr != condition_addr.s_addr)
    {
      inet_pton(AF_INET, "0.0.0.0", &newMsg->yiaddr);
    }
  else
    {
      char buffer[12];
      snprintf (buffer, 12, "192.168.1.%d", count);
      inet_pton(AF_INET, "192.168.1.1", &newMsg->yiaddr);
    }
  memcpy(newMsg->chaddr, recv_chaddr, 16); // Hardware address
  return newMsg;
}

struct sockaddr_in
address_gen()
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs((uint16_t)strtol(get_port(), NULL, 10));
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  return addr;
}

int address_len_helper(int htype)
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

void print_options_fields(options_t options)
{
  if (options.type == NULL)
  {
    printf("option type = 0\n");
  }
  else
  {
    printf("option type = %d\n", *options.type);
  }

  if (options.lease == NULL)
  {
    printf("lease time = 0\n");
  }
  else
  {
    printf("lease time = %d\n", *options.lease);
  }

  if (options.request == NULL)
  {
    printf("request ip = 0\n");
  }
  else
  {
    char req_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, options.request, req_buffer, sizeof(req_buffer));
    printf("requst ip = %s\n", req_buffer);
  }

  if (options.request == NULL)
  {
    printf("server id = 0\n");
  }
  else
  {
    char sid_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, options.sid, sid_buffer, sizeof(sid_buffer));
    printf("server id = %s\n", sid_buffer);
  }
}