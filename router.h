#ifndef ROUTER_H
#define ROUTER_H

#include <chrono>

const int MAX_PACKET_SIZE = 4096;
const std::chrono::seconds BROADCAST_PERIOD(5);
const std::chrono::seconds NODE_CHECK_PERIOD(11);
const time_t NODE_TTL = 10;

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
