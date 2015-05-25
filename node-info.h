#ifndef NODEINFO_INCL
#define NODEINFO_INCL

#include <string>

struct nodeinfo
{
  std::string dest_router;
  int source_portno;
  int cost;
};


#endif
