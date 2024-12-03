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

void server_reply_gen(msg_t *, options_t, int);
int address_len_helper(int);
struct sockaddr_in address_gen();
void print_options_fields(options_t);
int socket_helper(int, struct sockaddr_in);
void bad_server_reply_gen(msg_t *, int);

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

  socket_helper(socketfd, addr);
}

int socket_helper(int socketfd, struct sockaddr_in addr)
{
  // Create count to keep track of the amount of IPS that have been logged
  int yiaddr_count = 1; // Initialize yiaddr_count
  int count = 1;
  ip_record_t ip_records[4];
  for (int i = 0; i < 4; i++)
    {
      memset(ip_records[i].chaddr, 0, sizeof(ip_records[i].chaddr));
      ip_records[i].yiaddr_count = 0;
      ip_records[i].dhcp_type = 0;
      ip_records[i].is_tombstone = 0;
    }

  while (1)
  {
    // Set up the length of the request that the server receives
    size_t length = sizeof(msg_t) + sizeof(options_t);
    uint8_t *recv_buffer = calloc(1, length);
    socklen_t addrlen = sizeof(addr);
    ssize_t nbytes = recvfrom(socketfd, recv_buffer, length, 0, (struct sockaddr *)&addr, &addrlen);

    if (nbytes < 0)
    {
      free(recv_buffer);
      return 0;
    }
    // Create our BOOTP data from the received request
    msg_t *recv_msg = (msg_t *)recv_buffer;

    if (count >= MAX_IPS && memcmp(ip_records[3].chaddr, recv_msg->chaddr, sizeof(ip_records[3].chaddr)) != 0)
    {
      // Reject request

      // Construct the BOOTP response
      bad_server_reply_gen(recv_msg, count);

      size_t send_size = sizeof(msg_t);
      uint8_t *send_buffer = calloc(send_size, sizeof(uint8_t));
      memcpy(send_buffer, recv_msg, sizeof(msg_t));

      // Add all of the dhcp data to the response buffer
      send_buffer = append_cookie(send_buffer, &send_size);

      // Append the options for a 5th, invalid, client
      uint8_t type_val = DHCPNAK;
      send_buffer = append_option(send_buffer, &send_size, DHCP_opt_msgtype, sizeof(uint8_t), &type_val);

      struct in_addr sid_val;
      sid_val.s_addr = inet_addr("192.168.1.0");
      send_buffer = append_option(send_buffer, &send_size, DHCP_opt_sid, sizeof(struct in_addr), (uint8_t *)&sid_val);
      send_buffer = append_option(send_buffer, &send_size, DHCP_opt_end, 0, NULL);

      // Sending my response back to the client
      sendto(socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr, addrlen);
      free(send_buffer);
    }
    else
    {
      // Accept and handle request

      // Create an options struct, zero it out
      options_t options;
      options.request = calloc(1, sizeof(struct in_addr));
      options.lease = calloc(1, sizeof(uint32_t));
      options.type = calloc(1, sizeof(uint8_t));
      options.sid = calloc(1, sizeof(struct in_addr));

      // Get the options based on the received buffer
      get_options (recv_buffer + sizeof(msg_t) + sizeof(MAGIC_COOKIE), recv_buffer + nbytes - 1, &options);

      bool check_tombstones = true;
      // Keeps track of the yiaddr count just for the server's reply
      int yiaddr_for_reply = 0;
      for (int i = 0; i < 4; i++)
        {
          uint8_t zero_array[sizeof(ip_records[i].chaddr)] = {0};
          if (memcmp (ip_records[i].chaddr, recv_msg->chaddr, sizeof(ip_records[i].chaddr)) == 0)
            {
              // Client already exists
              yiaddr_for_reply = ip_records[i].yiaddr_count;
              check_tombstones = false;
              break;
            }
          if (memcmp (ip_records[i].chaddr, zero_array, sizeof(ip_records[i].chaddr)) == 0 && ip_records[i].is_tombstone == 0)
            {
              // New client, assign an IP
              memcpy (ip_records[i].chaddr, recv_msg->chaddr, sizeof(ip_records[i].chaddr));
              ip_records[i].is_tombstone = 0;
              ip_records[i].yiaddr_count = yiaddr_count;
              yiaddr_count++; // Assign the count as the current yiaddr_count
              yiaddr_for_reply = ip_records[i].yiaddr_count;
              count++; // Increment the count for new client
              check_tombstones = false;
              break;
            }
        }
      
      if (check_tombstones)
        {
          printf ("entering tombstone\n");
          bool new_chaddr_tombstone = false;
          for (int i = 0; i < 4; i++)
            {
              if (memcmp (ip_records[i].chaddr, recv_msg->chaddr, sizeof(ip_records[i].chaddr)) == 0)
                {

                  ip_records[i].dhcp_type = DHCPDISCOVER;
                  ip_records[i].is_tombstone = 0;
                  yiaddr_for_reply = ip_records[i].yiaddr_count;
                  new_chaddr_tombstone = true;
                  break;
                }
            }
          if (new_chaddr_tombstone)
            {
              for (int i = 0; i < 4; i++)
                {
                  if (memcmp (ip_records[i].chaddr, recv_msg->chaddr, sizeof(ip_records[i].chaddr)) == 0 && ip_records[i].is_tombstone == 1)
                    {
                      memcpy (ip_records[i].chaddr, recv_msg->chaddr, sizeof(ip_records[i].chaddr));
                      ip_records[i].dhcp_type = DHCPDISCOVER;
                      ip_records[i].is_tombstone = 0;
                      yiaddr_for_reply = ip_records[i].yiaddr_count;
                      break;
                    }
                }
            }
        }

      // Construct the BOOTP response
      server_reply_gen(recv_msg, options, yiaddr_for_reply);

      size_t send_size = sizeof(msg_t);
      uint8_t *send_buffer = calloc(send_size, sizeof(uint8_t));
      memcpy(send_buffer, recv_msg, sizeof(msg_t));

      // Add all of the dhcp data to the response buffer
      send_buffer = append_cookie(send_buffer, &send_size);

      uint8_t type_val;
      uint32_t lease_val = 0x008D2700;
      // If the message type is DHCPDISCOVER, then I send back an offer with a 30 day lease time
      if (*options.type == (uint8_t)DHCPDISCOVER)
      {
        type_val = DHCPOFFER;
        send_buffer = append_option(send_buffer, &send_size, DHCP_opt_msgtype, sizeof(uint8_t), &type_val);
        send_buffer = append_option(send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)&lease_val);
      }

      if (*options.type == (uint8_t)DHCPREQUEST)
      {
        // If message type is a DHCPREQUEST, then I check if the server id is correct
        if (options.sid->s_addr == inet_addr("192.168.1.0"))
        {
          // If the server id is correct, then I send a DHCPACK with a 30 day lease time
          type_val = DHCPACK;
          send_buffer = append_option(send_buffer, &send_size, DHCP_opt_msgtype, sizeof(uint8_t), &type_val);
          send_buffer = append_option(send_buffer, &send_size, DHCP_opt_lease, sizeof(uint32_t), (uint8_t *)&lease_val);
        }
        else
        {
          // If the server id is incorrect, I send a DHCPNAK
          type_val = DHCPNAK;
          send_buffer = append_option(send_buffer, &send_size, DHCP_opt_msgtype, sizeof(uint8_t), &type_val);
        }
      }

      // If the message type if DHCPRELEASE, we must clear up space in the array for another request
      if (*options.type == (uint8_t)DHCPRELEASE)
        {
          for (int i = 0; i < 4; i++)
            {
              if (memcmp (ip_records[i].chaddr, recv_msg->chaddr, sizeof (ip_records[i].chaddr)) == 0)
                {
                  ip_records[i].is_tombstone = 1;
                  // memset(ip_records[i].chaddr, 0, sizeof(ip_records[i].chaddr));
                  count--;
                }
            }
          // Free all allocated memory
          free(send_buffer);
          free_options(&options);
          continue;
        }

      // Track where each request is in its DHCPDISCOVER-->DHCPREQUEST cycle
      for (int i = 0; i < 4; i++)
        {
          if (ip_records[i].dhcp_type == 0)
            {
              ip_records[i].dhcp_type = DHCPDISCOVER;
            }
          else if (ip_records[i].dhcp_type == DHCPDISCOVER)
            {
              ip_records[i].dhcp_type = DHCPREQUEST;
            }
        }

      // Send back a server id of 192.168.1.0
      struct in_addr sid_val;
      sid_val.s_addr = inet_addr("192.168.1.0");
      send_buffer = append_option(send_buffer, &send_size, DHCP_opt_sid, sizeof(struct in_addr), (uint8_t *)&sid_val);
      send_buffer = append_option(send_buffer, &send_size, DHCP_opt_end, 0, NULL);

      // Sending my response back to the client
      sendto(socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr, addrlen);
      free(send_buffer);
      free_options(&options);
    }
    free(recv_buffer);
  }
  close(socketfd);
  return 1;
}

void bad_server_reply_gen(msg_t *recv_msg, int count)
{
  // Change the op and the yiaddr of the msg_t that is passed in, for an invalid client
  recv_msg->op = 2;
  inet_pton(AF_INET, "0.0.0.0", &recv_msg->yiaddr);
}

void server_reply_gen(msg_t *recv_msg, options_t options, int count)
{
  // Change the op and the yiaddr of the msg_t that is passed in, for a valid client
  recv_msg->op = 2;
  struct in_addr condition_addr;
  condition_addr.s_addr = inet_addr("192.168.1.0");
  if (*(options.type) == (uint8_t)DHCPREQUEST && options.sid->s_addr != condition_addr.s_addr)
  {
    inet_pton(AF_INET, "0.0.0.0", &recv_msg->yiaddr);
  }
  else
  {
    char buffer[12];
    snprintf(buffer, 12, "192.168.1.%d", count);
    inet_pton(AF_INET, buffer, &recv_msg->yiaddr);
  }
  inet_pton(AF_INET, "0.0.0.0", &recv_msg->ciaddr);
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

/*
static action_t const ip_[][NUM_EVENTS] = {
  { DHCPDISCOVER, DHCPREQUEST },
  { DHCPDISCOVER, DHCPREQUEST },
  { DHCPDISCOVER, DHCPREQUEST },
  { DHCPDISCOVER, DHCPREQUEST },
  { DHCPDISCOVER, DHCPREQUEST }
};
*/
