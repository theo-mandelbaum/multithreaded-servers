#include <stdio.h>
#include <arpa/inet.h>

#include "dhcp.h"
#include "format.h"
#include "math.h"

void address_print_helper (uint32_t);
char *htype_helper (int);
void ip_request (uint8_t *, bool, bool, bool, bool, bool, bool, bool);
void lease_time (uint8_t *, bool, bool, bool, bool, bool, bool, bool);
void server_id (uint8_t *, bool, bool, bool, bool, bool, bool, bool);

void
dump_msg (FILE *output, uint8_t *buffer, size_t size)
{
  msg_t *msg = buffer; // Initializes the msg_t with buffer to get all
                                // of the file data for BOOTP
  uint32_t *cookie = (uint32_t *)(buffer + sizeof (msg_t));

  printf ("------------------------------------------------------\n");
  printf ("BOOTP Options\n");
  printf ("------------------------------------------------------\n");

  char *opDesc = msg->op == BOOTREQUEST ? "BOOTREQUEST" : "BOOTREPLY";
  printf ("Op Code (op) = %d [%s]\n", msg->op, opDesc);

  char *htypeDesc = htype_helper (msg->htype);
  printf ("Hardware Type (htype) = %d [%s]\n", msg->htype, htypeDesc);
  printf ("Hardware Address Length (hlen) = %d\n", msg->hlen);
  printf ("Hops (hops) = %d\n", msg->hops);

  uint32_t xid = ntohl (msg->xid);
  printf ("Transaction ID (xid) = %u (0x%x)\n", xid, xid);

  uint16_t secs = ntohs (msg->secs);
  int days = secs / 86400;
  int hours = (secs % 86400) / 3600;
  int minutes = (secs % 3600) / 60;
  int seconds = secs % 60;
  printf ("Seconds (secs) = %d Days, %d:%02d:%02d\n", days, hours, minutes,
          seconds);

  printf ("Flags (flags) = %u\n", ntohs (msg->flags));

  printf ("Client IP Address (ciaddr) = ");
  address_print_helper (msg->ciaddr.s_addr);
  printf ("\n");

  printf ("Your IP Address (yiaddr) = ");
  address_print_helper (msg->yiaddr.s_addr);
  printf ("\n");

  printf ("Server IP Address (siaddr) = ");
  address_print_helper (msg->siaddr.s_addr);
  printf ("\n");

  printf ("Relay IP Address (giaddr) = ");
  address_print_helper (msg->giaddr.s_addr);
  printf ("\n");

  printf ("Client Ethernet Address (chaddr) = ");
  for (int i = 0; i < msg->hlen; i++)
    {
      printf ("%02x", msg->chaddr[i]);
    }
  printf ("\n");

  if (ntohl (*cookie) == MAGIC_COOKIE) // Checks if the first 4 bytes after
                                       // BOOTP are the magic cookie
    {
      printf ("------------------------------------------------------\n");
      printf ("DHCP Options\n");
      printf ("------------------------------------------------------\n");
      printf ("Magic Cookie = [OK]\n");
      bool discover, offer, request, decline, ack, nak, release;
      size_t offset
          = sizeof (msg_t) + sizeof (uint32_t); // Size of the BOOTP and the
                                                // cookie so it starts at DHCP
      while (offset < size)
        {
          uint8_t *option
              = (uint8_t *)(buffer + offset); // Make option pointer point to
                                              // the op field
          offset += sizeof (uint8_t);

          if (*option == 255) // End of DHCP
            break;
          if (*option == 0)
            continue; // ?

          uint8_t *length
              = (uint8_t *)(buffer + offset); // Make length pointer point to
                                              // len field
          offset += sizeof (uint8_t);
          uint8_t *data
              = (uint8_t *)(buffer + offset); // New pointer that points to the
                                              // rest of the request
          offset += *length * sizeof (uint8_t);
          switch (*option)
            { // Disgusting switch to parse options
            case 50:
              ip_request (data, discover, offer, request, decline, ack, nak,
                          release);
              break;
            case 51:
              lease_time (data, discover, offer, request, decline, ack, nak,
                          release);
              break;
            case 53:
              switch (data[0])
                { // Even more gross switch to parse the data for message types
                case DHCPDISCOVER:
                  printf ("Message Type = DHCP Discover\n");
                  discover = true;
                  offer = request = decline = ack = nak = release = false;
                  break;
                case DHCPOFFER:
                  printf ("Message Type = DHCP Offer\n");
                  offer = true;
                  discover = request = decline = ack = nak = release = false;
                  break;
                case DHCPREQUEST:
                  printf ("Message Type = DHCP Request\n");
                  request = true;
                  discover = offer = decline = ack = nak = release = false;
                  break;
                case DHCPDECLINE:
                  printf ("Message Type = DHCP Decline\n");
                  decline = true;
                  discover = offer = request = ack = nak = release = false;
                  break;
                case DHCPACK:
                  printf ("Message Type = DHCP ACK\n");
                  ack = true;
                  discover = offer = request = decline = nak = release = false;
                  break;
                case DHCPNAK:
                  printf ("Message Type = DHCP NAK\n");
                  nak = true;
                  discover = offer = request = decline = ack = release = false;
                  break;
                case DHCPRELEASE:
                  printf ("Message Type = DHCP Release\n");
                  release = true;
                  discover = offer = request = decline = ack = nak = false;
                  break;
                default:
                  break;
                }
              break;
            case 54:
              server_id (data, discover, offer, request, decline, ack, nak,
                         release);
              break;
            default:
              break;
            }
        }
    }
}

void
address_print_helper (uint32_t address)
{
  struct in_addr ip_addr;
  ip_addr.s_addr = address;
  printf ("%s", inet_ntoa (ip_addr));
}

char *
htype_helper (int htype)
{
  switch (htype)
    {
    case ETH:
      return "Ethernet (10Mb)";
    case IEEE802:
      return "IEEE 802 Networks";
    case ARCNET:
      return "ARCNET";
    case FRAME_RELAY:
      return "Frame Relay";
    case FIBRE:
      return "Fibre Channel";
    default:
      return "Unknown";
    }
}

// Helper function to take the information from data and the flags
// and print out the ip request depending on which message type is flagged.
void
ip_request (uint8_t *data, bool discover, bool offer, bool request,
            bool decline, bool ack, bool nak, bool release)
{
  if (discover)
    {
      return;
    }
  else if (offer)
    {
      return;
    }
  else if (request)
    {
      printf ("Request = %d.%d.%d.%d\n", data[0], data[1], data[2], data[3]);
      return;
    }
  else if (decline)
    {
      printf ("Request = %d.%d.%d.%d\n", data[0], data[1], data[2], data[3]);
      return;
    }
  else if (ack)
    {
      return;
    }
  else if (nak)
    {
      return;
    }
  else if (release)
    {
      printf ("Request = %d.%d.%d.%d\n", data[0], data[1], data[2], data[3]);
      return;
    }
  else
    {
      return;
    }
}

// Helper function to take the information from data and the flags
// and print out the lease time depending on which message type is flagged.
void
lease_time (uint8_t *data, bool discover, bool offer, bool request,
            bool decline, bool ack, bool nak, bool release)
{
  uint32_t secs = 0;
  for (int i = 0; i < 4; i++)
    {
      secs = (secs << 8) | data[i];
    }

  int days = secs / 86400;
  int hours = (secs % 86400) / 3600;
  int minutes = (secs % 3600) / 60;
  int seconds = secs % 60;
  if (discover)
    {
      return;
    }
  else if (offer)
    {
      printf ("IP Address Lease Time = %d Days, %d:%02d:%02d\n", days, hours,
              minutes, seconds);
      return;
    }
  else if (request)
    {
      return;
    }
  else if (decline)
    {
      return;
    }
  else if (ack)
    {
      printf ("IP Address Lease Time = %d Days, %d:%02d:%02d\n", days, hours,
              minutes, seconds);
      return;
    }
  else if (nak)
    {
      return;
    }
  else if (release)
    {
      return;
    }
  else
    {
      return;
    }
}

// Helper function to take the information from data and the flags
// and print out the server id depending on which message type is flagged.
void
server_id (uint8_t *data, bool discover, bool offer, bool request,
           bool decline, bool ack, bool nak, bool release)
{
  if (discover)
    {
      return;
    }
  else if (offer)
    {
      printf ("Server Identifier = %d.%d.%d.%d\n", data[0], data[1], data[2],
              data[3]);
      return;
    }
  else if (request)
    {
      printf ("Server Identifier = %d.%d.%d.%d\n", data[0], data[1], data[2],
              data[3]);
      return;
    }
  else if (decline)
    {
      printf ("Server Identifier = %d.%d.%d.%d\n", data[0], data[1], data[2],
              data[3]);
      return;
    }
  else if (ack)
    {
      printf ("Server Identifier = %d.%d.%d.%d\n", data[0], data[1], data[2],
              data[3]);
      return;
    }
  else if (nak)
    {
      printf ("Server Identifier = %d.%d.%d.%d\n", data[0], data[1], data[2],
              data[3]);
      return;
    }
  else if (release)
    {
      printf ("Server Identifier = %d.%d.%d.%d\n", data[0], data[1], data[2],
              data[3]);
      return;
    }
  else
    {
      return;
    }
}