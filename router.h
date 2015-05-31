#ifndef ROUTER_H
#define ROUTER_H

const int MAX_PACKET_SIZE = 4096;

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
