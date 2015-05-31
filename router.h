#ifndef ROUTER_H
#define ROUTER_H

const int MAX_PACKET_SIZE = 4096;

enum PacketType : char {
   CONTROL = 0,
   DATA = 1
};

typedef std::pair<int, unsigned short> dv_entry;

#endif
