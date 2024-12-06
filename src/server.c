#include "semaphore.h"
#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dhcp.h"
#include "format.h"
#include "port_utils.h"
#include "server.h"

#define MAX_IPS 5

void server_reply_gen (msg_t *, options_t, int);
int address_len_helper (int);
struct sockaddr_in address_gen ();
void print_options_fields (options_t);
int socket_helper (int, struct sockaddr_in);
int socket_helper_thread (int, struct sockaddr_in);
void bad_server_reply_gen (msg_t *, int);
void handle_client_assignment (ip_record_t *, msg_t *, int *, int *, int *,
                               bool *);
void handle_tombstone (ip_record_t *, msg_t *, int *);
options_t create_options (uint8_t *, ssize_t);
void handle_nak_message (int, msg_t *, struct sockaddr_in, socklen_t);
int handle_valid_message (int, struct sockaddr_in, socklen_t, options_t,
                          msg_t *, int *, ip_record_t *);
int handle_dhcp_release (ip_record_t *, msg_t *, int *, uint8_t *, options_t);
thread_args_t thread_args_init (int, uint8_t **, ssize_t *, struct sockaddr_in,
                                socklen_t, bool, int);
void *child (void *);

int yiaddr_count = 1;
int count = 1;
ip_record_t ip_records[4];

void
echo_server_thread (int time)
{
  int socketfd = socket (AF_INET, SOCK_DGRAM, 0);

  // Create my address struct using a helper
  struct sockaddr_in addr = address_gen ();

  // Set reusable socket options
  int socket_option = 1;
  setsockopt (socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&socket_option,
              sizeof (int));

  // Set timeout socket options
  struct timeval timeout = { time, 0 };
  setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout,
              sizeof (timeout));

  // bind the socket to the client
  if (bind (socketfd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("Bind failed");
      close (socketfd);
      exit (EXIT_FAILURE);
    }

  socket_helper_thread (socketfd, addr);
}

int
socket_helper_thread (int socketfd, struct sockaddr_in addr)
{
  // Create count to keep track of the amount of IPS that have been logged
  for (int i = 0; i < 4; i++)
    {
      memset (ip_records[i].chaddr, 0, sizeof (ip_records[i].chaddr));
      ip_records[i].yiaddr_count = 0;
      ip_records[i].dhcp_type = 0;
      ip_records[i].is_tombstone = 0;
    }

  // int request_counter = 0;
  uint8_t *recv_buffer_array[4];
  // Set up the length of the request that the server receives
  size_t length = sizeof (msg_t) + sizeof (options_t);
  for (int i = 0; i < 4; i++)
    {
      recv_buffer_array[i] = calloc (1, length);
    }
  ssize_t nbytes_array[4];
  socklen_t addrlen = sizeof (addr);
  // First loop to get the queue for the 4 DISCOVERs
  for (int i = 0; i < 4; i++)
    {
      ssize_t nbytes = recvfrom (socketfd, recv_buffer_array[i], length, 0,
                                 (struct sockaddr *)&addr, &addrlen);

      if (nbytes < 0)
        {
          free (recv_buffer_array[i]);
          return 0;
        }

      nbytes_array[i] = nbytes;
    }

  thread_args_t *args = calloc (1, sizeof (thread_args_t));
  *args = thread_args_init (socketfd, recv_buffer_array, nbytes_array, addr,
                            addrlen, false, 4);
  pthread_t t;
  assert (pthread_create (&t, NULL, child, (void *)args) == 0);

  for (int i = 0; i < 4; i++)
    {
      free (recv_buffer_array[i]);
    }

  printf ("Should have finished sending offers\n");

  // options_t options = create_options (recv_buffer, nbytes);
  // if (*options.type == DHCPRELEASE)
  //   {
  //     recv_buffer_array[0] = recv_buffer;
  //     pthread_t t;
  //     thread_args_t *args = calloc (1, sizeof (thread_args_t));
  //     *args = thread_args_init (socketfd, recv_buffer_array, nbytes, addr,
  //     addrlen, true, 1); pthread_create (t, NULL, child, (void *)args);
  //     pthread_detach(t);
  //     last_type = DHCPRELEASE;
  //     last_last_type = last_type;
  //   }
  // else
  //   {
  //     pthread_t t[4];
  //     if (request_counter < 3)
  //       {
  //         recv_buffer_array[request_counter] = recv_buffer;
  //         request_counter++;
  //         continue;
  //       }
  //     else
  //       {
  //         recv_buffer_array[request_counter] = recv_buffer;
  //         request_counter++;
  //       }
  //     thread_args_t *args = calloc (1, sizeof (thread_args_t));
  //     *args = thread_args_init (socketfd, recv_buffer_array, nbytes, addr,
  //     addrlen, false, 4); for (int i = 0; i < 4; i++)
  //       {
  //         pthread_create (&t[i], NULL, child, (void *)args);
  //         pthread_detach(t[i]);
  //         free (recv_buffer_array[i]);
  //       }
  //     free (args);
  //     request_counter = 0;
  //  }
  // free_options (&options);
  close (socketfd);
  return 1;
}

void
echo_server (int time)
{
  int socketfd = socket (AF_INET, SOCK_DGRAM, 0);

  // Create my address struct using a helper
  struct sockaddr_in addr = address_gen ();

  // Set reusable socket options
  int socket_option = 1;
  setsockopt (socketfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&socket_option,
              sizeof (int));

  // Set timeout socket options
  struct timeval timeout = { time, 0 };
  setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout,
              sizeof (timeout));

  // bind the socket to the client
  if (bind (socketfd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("Bind failed");
      close (socketfd);
      exit (EXIT_FAILURE);
    }

  socket_helper (socketfd, addr);
}

int
socket_helper (int socketfd, struct sockaddr_in addr)
{
  // Create count to keep track of the amount of IPS that have been logged
  pthread_t threads[4];
  for (int i = 0; i < 4; i++)
    {
      memset (ip_records[i].chaddr, 0, sizeof (ip_records[i].chaddr));
      ip_records[i].yiaddr_count = 0;
      ip_records[i].dhcp_type = 0;
      ip_records[i].is_tombstone = 0;
    }

  while (1)
    {
      // Set up the length of the request that the server receives
      size_t length = sizeof (msg_t) + sizeof (options_t);
      uint8_t *recv_buffer = calloc (1, length);
      socklen_t addrlen = sizeof (addr);
      ssize_t nbytes = recvfrom (socketfd, recv_buffer, length, 0,
                                 (struct sockaddr *)&addr, &addrlen);

      // pthread_create ();

      if (nbytes < 0)
        {
          free (recv_buffer);
          return 0;
        }
      // Create our BOOTP data from the received request
      msg_t *recv_msg = (msg_t *)recv_buffer;

      if (count >= MAX_IPS
          && memcmp (ip_records[3].chaddr, recv_msg->chaddr,
                     sizeof (ip_records[3].chaddr))
                 != 0)
        {
          // Reject request
          // Construct the BOOTP responsex
          options_t options = create_options (recv_buffer, nbytes);

          uint8_t *fake_send_buffer = calloc (1, sizeof (uint8_t));
          if (handle_dhcp_release (ip_records, recv_msg, &count,
                                   fake_send_buffer, options)
              == 0)
            continue;

          bad_server_reply_gen (recv_msg, count);

          handle_nak_message (socketfd, recv_msg, addr, addrlen);
        }
      else
        {
          // Accept and handle request

          // Create an options struct, zero it out
          options_t options = create_options (recv_buffer, nbytes);

          bool check_tombstones = true;
          // Keeps track of the yiaddr count just for the server's reply
          int yiaddr_for_reply = 0;
          handle_client_assignment (ip_records, recv_msg, &yiaddr_count,
                                    &yiaddr_for_reply, &count,
                                    &check_tombstones);

          // Handle tombstone cases
          if (check_tombstones)
            {
              handle_tombstone (ip_records, recv_msg, &yiaddr_for_reply);
            }

          // Construct the BOOTP response
          server_reply_gen (recv_msg, options, yiaddr_for_reply);

          if (handle_valid_message (socketfd, addr, addrlen, options, recv_msg,
                                    &count, ip_records)
              == 0)
            continue;

          free_options (&options);
        }
      free (recv_buffer);
    }
  close (socketfd);
  return 1;
}

int
handle_dhcp_release (ip_record_t *ip_records, msg_t *recv_msg, int *count,
                     uint8_t *send_buffer, options_t options)
{
  // If the message type if DHCPRELEASE, we must clear up space in the array
  // for another request
  if (*options.type == (uint8_t)DHCPRELEASE)
    {
      for (int i = 0; i < 4; i++)
        {
          if (memcmp (ip_records[i].chaddr, recv_msg->chaddr,
                      sizeof (ip_records[i].chaddr))
              == 0)
            {
              ip_records[i].is_tombstone = 1;
              (*count)--;
            }
        }
      // Free all allocated memory
      free_options (&options);
      free (send_buffer);
      return 0;
    }
  return 1;
}

int
handle_valid_message (int socketfd, struct sockaddr_in addr, socklen_t addrlen,
                      options_t options, msg_t *recv_msg, int *count,
                      ip_record_t *ip_records)
{
  size_t send_size = sizeof (msg_t);
  uint8_t *send_buffer = calloc (send_size, sizeof (uint8_t));
  memcpy (send_buffer, recv_msg, sizeof (msg_t));

  // Add all of the dhcp data to the response buffer
  send_buffer = append_cookie (send_buffer, &send_size);

  uint8_t type_val;
  uint32_t lease_val = 0x008D2700;
  // If the message type is DHCPDISCOVER, then I send back an offer with a 30
  // day lease time
  if (*options.type == (uint8_t)DHCPDISCOVER)
    {
      type_val = DHCPOFFER;
      send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype,
                                   sizeof (uint8_t), &type_val);
      send_buffer = append_option (send_buffer, &send_size, DHCP_opt_lease,
                                   sizeof (uint32_t), (uint8_t *)&lease_val);
    }

  if (*options.type == (uint8_t)DHCPREQUEST)
    {
      // If message type is a DHCPREQUEST, then I check if the server id is
      // correct
      if (options.sid->s_addr == inet_addr ("192.168.1.0"))
        {
          // If the server id is correct, then I send a DHCPACK with a 30 day
          // lease time
          type_val = DHCPACK;
          send_buffer
              = append_option (send_buffer, &send_size, DHCP_opt_msgtype,
                               sizeof (uint8_t), &type_val);
          send_buffer
              = append_option (send_buffer, &send_size, DHCP_opt_lease,
                               sizeof (uint32_t), (uint8_t *)&lease_val);
        }
      else
        {
          // If the server id is incorrect, I send a DHCPNAK
          type_val = DHCPNAK;
          send_buffer
              = append_option (send_buffer, &send_size, DHCP_opt_msgtype,
                               sizeof (uint8_t), &type_val);
        }
    }

  if (handle_dhcp_release (ip_records, recv_msg, count, send_buffer, options)
      == 0)
    return 0;

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
  sid_val.s_addr = inet_addr ("192.168.1.0");
  send_buffer = append_option (send_buffer, &send_size, DHCP_opt_sid,
                               sizeof (struct in_addr), (uint8_t *)&sid_val);
  send_buffer = append_option (send_buffer, &send_size, DHCP_opt_end, 0, NULL);

  // Sending my response back to the client
  sendto (socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr,
          addrlen);
  free (send_buffer);
  return 1;
}

void
handle_nak_message (int socketfd, msg_t *recv_msg, struct sockaddr_in addr,
                    socklen_t addrlen)
{
  size_t send_size = sizeof (msg_t);
  uint8_t *send_buffer = calloc (send_size, sizeof (uint8_t));
  memcpy (send_buffer, recv_msg, sizeof (msg_t));

  // Add all of the dhcp data to the response buffer
  send_buffer = append_cookie (send_buffer, &send_size);

  // Append the options for a 5th, invalid, client
  uint8_t type_val = DHCPNAK;
  send_buffer = append_option (send_buffer, &send_size, DHCP_opt_msgtype,
                               sizeof (uint8_t), &type_val);

  struct in_addr sid_val;
  sid_val.s_addr = inet_addr ("192.168.1.0");
  send_buffer = append_option (send_buffer, &send_size, DHCP_opt_sid,
                               sizeof (struct in_addr), (uint8_t *)&sid_val);
  send_buffer = append_option (send_buffer, &send_size, DHCP_opt_end, 0, NULL);

  // Sending my response back to the client
  sendto (socketfd, send_buffer, send_size, 0, (struct sockaddr *)&addr,
          addrlen);
  free (send_buffer);
}

options_t
create_options (uint8_t *recv_buffer, ssize_t nbytes)
{
  printf ("reaching create_options\n");
  options_t options;
  options.request = calloc (1, sizeof (struct in_addr));
  options.lease = calloc (1, sizeof (uint32_t));
  options.type = calloc (1, sizeof (uint8_t));
  options.sid = calloc (1, sizeof (struct in_addr));

  // Get the options based on the received buffer
  printf ("about to get_options\n");
  get_options (recv_buffer + sizeof (msg_t) + sizeof (MAGIC_COOKIE),
               recv_buffer + nbytes - 1, &options);
  printf ("after get_options\n");
  printf ("%s\n", options.type);
  return options;
}

void
handle_tombstone (ip_record_t *ip_records, msg_t *recv_msg,
                  int *yiaddr_for_reply)
{
  bool new_chaddr_tombstone = true;
  for (int i = 0; i < 4; i++)
    {
      if (memcmp (ip_records[i].chaddr, recv_msg->chaddr,
                  sizeof (ip_records[i].chaddr))
          == 0)
        {

          ip_records[i].dhcp_type = DHCPDISCOVER;
          ip_records[i].is_tombstone = 0;
          *yiaddr_for_reply = ip_records[i].yiaddr_count;
          new_chaddr_tombstone = false;
          break;
        }
    }
  if (new_chaddr_tombstone)
    {
      for (int i = 0; i < 4; i++)
        {
          if (memcmp (ip_records[i].chaddr, recv_msg->chaddr,
                      sizeof (ip_records[i].chaddr))
                  != 0
              && ip_records[i].is_tombstone == 1)
            {
              memcpy (ip_records[i].chaddr, recv_msg->chaddr,
                      sizeof (ip_records[i].chaddr));
              ip_records[i].dhcp_type = DHCPDISCOVER;
              ip_records[i].is_tombstone = 0;
              printf ("%d\n", ip_records[i].yiaddr_count);
              *yiaddr_for_reply = ip_records[i].yiaddr_count;
              break;
            }
        }
    }
}

void
handle_client_assignment (ip_record_t *ip_records, msg_t *recv_msg,
                          int *yiaddr_count, int *yiaddr_for_reply, int *count,
                          bool *check_tombstones)
{
  for (int i = 0; i < 4; i++)
    {
      uint8_t zero_array[sizeof (ip_records[i].chaddr)] = { 0 };
      if (memcmp (ip_records[i].chaddr, recv_msg->chaddr,
                  sizeof (ip_records[i].chaddr))
          == 0)
        {
          // Client already exists
          *yiaddr_for_reply = ip_records[i].yiaddr_count;
          *check_tombstones = false;
          break;
        }
      if (memcmp (ip_records[i].chaddr, zero_array,
                  sizeof (ip_records[i].chaddr))
              == 0
          && ip_records[i].is_tombstone == 0)
        {
          // New client, assign an IP
          memcpy (ip_records[i].chaddr, recv_msg->chaddr,
                  sizeof (ip_records[i].chaddr));
          ip_records[i].is_tombstone = 0;
          ip_records[i].yiaddr_count = *yiaddr_count;
          (*yiaddr_count)++; // Assign the count as the current yiaddr_count
          *yiaddr_for_reply = ip_records[i].yiaddr_count;
          (*count)++; // Increment the count for new client
          *check_tombstones = false;
          break;
        }
    }
}

void
bad_server_reply_gen (msg_t *recv_msg, int count)
{
  // Change the op and the yiaddr of the msg_t that is passed in, for an
  // invalid client
  recv_msg->op = 2;
  inet_pton (AF_INET, "0.0.0.0", &recv_msg->yiaddr);
  inet_pton (AF_INET, "0.0.0.0", &recv_msg->ciaddr);
}

void
server_reply_gen (msg_t *recv_msg, options_t options, int count)
{
  // Change the op and the yiaddr of the msg_t that is passed in, for a valid
  // client
  recv_msg->op = 2;
  struct in_addr condition_addr;
  condition_addr.s_addr = inet_addr ("192.168.1.0");
  if (*(options.type) == (uint8_t)DHCPREQUEST
      && options.sid->s_addr != condition_addr.s_addr)
    {
      inet_pton (AF_INET, "0.0.0.0", &recv_msg->yiaddr);
    }
  else
    {
      char buffer[12];
      snprintf (buffer, 12, "192.168.1.%d", count);
      inet_pton (AF_INET, buffer, &recv_msg->yiaddr);
    }
  inet_pton (AF_INET, "0.0.0.0", &recv_msg->ciaddr);
}

struct sockaddr_in
address_gen ()
{
  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
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

thread_args_t
thread_args_init (int socket, uint8_t **buffer_array, ssize_t *bytes,
                  struct sockaddr_in addr, socklen_t addrlen,
                  bool release_flag, int iterations)
{
  thread_args_t args;
  args.sfd = socket;
  args.recvbuffer_size = 0;
  for (int i = 0; i < iterations; i++)
    {
      args.recvbuffer_size++;
      args.recvbuffer[i] = calloc (1, bytes[i]);
      memcpy (args.recvbuffer[i], buffer_array[i], bytes[i]);
    }
  args.nbytes = bytes[0];
  args.addr = addr;
  args.addrlen = addrlen;
  args.release_flag = release_flag;
  return args;
}

// void *
// child (void *args)
// {
//   // Create our BOOTP data from the received request
//   thread_args_t *thread_stuff = (thread_args_t *) args;
//   for (int i = 0; i < thread_stuff->recvbuffer_size; i++)
//     {
//       msg_t *recv_msg = (msg_t *)thread_stuff->recvbuffer[i];

//       if (count >= MAX_IPS && memcmp(ip_records[3].chaddr, recv_msg->chaddr,
//       sizeof(ip_records[3].chaddr)) != 0)
//       {
//         // Reject request
//         // Construct the BOOTP response
//         options_t options = create_options (thread_stuff->recvbuffer[i],
//         thread_stuff->nbytes);

//         uint8_t *fake_send_buffer = calloc (1, sizeof (uint8_t));
//         if (handle_dhcp_release (ip_records, recv_msg, &count,
//         fake_send_buffer, options) == 0)
//           pthread_exit (NULL);

//         bad_server_reply_gen(recv_msg, count);

//         handle_nak_message (thread_stuff->sfd, recv_msg, thread_stuff->addr,
//         thread_stuff->addrlen);

//       }
//       else
//       {
//         // Accept and handle request

//         // Create an options struct, zero it out
//         options_t options = create_options (thread_stuff->recvbuffer[i],
//         thread_stuff->nbytes);

//         bool check_tombstones = true;
//         // Keeps track of the yiaddr count just for the server's reply
//         int yiaddr_for_reply = 0;
//         handle_client_assignment (ip_records, recv_msg, &yiaddr_count,
//         &yiaddr_for_reply, &count, &check_tombstones);

//         // Handle tombstone cases
//         if (check_tombstones)
//           {
//             handle_tombstone (ip_records, recv_msg, &yiaddr_for_reply);
//           }

//         // Construct the BOOTP response
//         server_reply_gen(recv_msg, options, yiaddr_for_reply);

//         if (handle_valid_message (thread_stuff->sfd, thread_stuff->addr,
//         thread_stuff->addrlen, options, recv_msg, &count, ip_records) == 0)
//           pthread_exit (NULL);

//         free_options(&options);
//       }
//       free (thread_stuff->recvbuffer[i]);
//     }
//     pthread_exit (NULL);
// }

void *
child (void *args)
{
  printf ("Child function reached\n");
  thread_args_t *thread_stuff = (thread_args_t *)args;

  for (int i = 0; i < thread_stuff->recvbuffer_size; i++)
    {
      printf ("entering for loop\n");
      msg_t *recv_msg = (msg_t *)thread_stuff->recvbuffer[i];
      printf ("created recv_msg\n");
      options_t options
          = create_options (thread_stuff->recvbuffer[i], thread_stuff->nbytes);
      printf ("Activating if statement\n");
      if (count >= MAX_IPS
          && memcmp (ip_records[3].chaddr, recv_msg->chaddr,
                     sizeof (ip_records[3].chaddr))
                 != 0)
        {
          printf ("Entering nak condition\n");
          uint8_t *fake_send_buffer = calloc (1, sizeof (uint8_t));
          if (handle_dhcp_release (ip_records, recv_msg, &count,
                                   fake_send_buffer, options)
              == 0)
            {
              free (fake_send_buffer);
              continue;
            }
          free (fake_send_buffer);
          bad_server_reply_gen (recv_msg, count);
          handle_nak_message (thread_stuff->sfd, recv_msg, thread_stuff->addr,
                              thread_stuff->addrlen);
        }
      else
        {
          printf ("Entering else condition\n");
          int yiaddr_for_reply = 0;
          bool check_tombstones = true;

          handle_client_assignment (ip_records, recv_msg, &yiaddr_count,
                                    &yiaddr_for_reply, &count,
                                    &check_tombstones);
          if (check_tombstones)
            handle_tombstone (ip_records, recv_msg, &yiaddr_for_reply);

          server_reply_gen (recv_msg, options, yiaddr_for_reply);

          if (handle_valid_message (thread_stuff->sfd, thread_stuff->addr,
                                    thread_stuff->addrlen, options, recv_msg,
                                    &count, ip_records)
              == 0)
            continue;
        }
      free_options (&options);
      free (thread_stuff->recvbuffer[i]); // Free buffer if owned by thread
    }
  printf ("exiting thread\n");
  pthread_exit (NULL);
}
