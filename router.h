#ifndef ROUTER_H
#define ROUTER_H

#include <chrono>

// Maximum packet size for any type of transmission
const int MAX_PACKET_SIZE = 4096;

// Number of seconds to wait between broadcasts
const std::chrono::seconds BROADCAST_PERIOD(5);

// Number of seconds to wait between checking for expired routes
const std::chrono::seconds NODE_CHECK_PERIOD(11);

// Number of seconds for which a node can be inactive for without being
// declared dead
const time_t NODE_TTL = 10;

// Path cost that is considered to be "infinity", which places a max path
// limitation on any routes to prevent endless routing loops
const int INFINITY = 64;

enum PacketType : char {
   CONTROL = 0,
   DATA = 1
};

enum print_type {NORMAL, FINAL_DEST, ERROR};

struct nodeinfo
{
  std::string dest_router;
  int source_portno;
  int cost;
};

typedef std::pair<int, unsigned short> dv_entry;

#endif
