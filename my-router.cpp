#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

#include <sys/socket.h>
#include <netinet/in.h>

#include "node-info.h"
#include "read-file.h"
#include "read-dv.h"

const int MAX_PACKET_SIZE = 4096;

enum PacketType : char {
   CONTROL = 0,
   DATA = 1
};

typedef std::pair<int, short> dv_entry;

void broadcastDV(const std::unordered_map<std::string, dv_entry>& dv, 
                 const std::vector<nodeinfo> neighbor_info,
                 int out_socket)
{
   std::string packet = std::string();

   // Write header bytes
   packet += CONTROL;

   // Write payload bytes
   for (auto iter = dv.cbegin(); iter != dv.cend(); iter++)
   {
      packet += iter->first;   // Write dest node name
      packet += '\0';

      // Write cost in big-endian
      int cost = iter->second.first;
      packet += (char)((cost & 0xff000000) >> 24);
      packet += (char)((cost & 0x00ff0000) >> 16);
      packet += (char)((cost & 0x0000ff00) >> 8);
      packet += (char)(cost & 0x000000ff);
   }

   sockaddr_in dest_addr;
   dest_addr.sin_family = AF_INET;
   dest_addr.sin_addr.s_addr = INADDR_ANY;
   for (auto iter = neighbor_info.begin(); iter != neighbor_info.end(); iter++)
   {
      dest_addr.sin_port = htons(iter->source_portno);
      sendto(out_socket, packet.c_str(), packet.size(), 0,
             (sockaddr*)&dest_addr, sizeof(dest_addr));
   }
}

int updateDV(std::unordered_map<std::string, dv_entry>& dv,
             short source_port,
             const std::list<std::pair<std::string, int>>& lcp)
{
}

void usage()
{
   std::cerr << "Usage: my-router [ID] [PORT] [INIT-FILE]" << std::endl;
}

int main(int argc, char **argv)
{
   if (argc != 4)
   {
      std::cerr << "my-router: Invalid number of arguments" << std::endl;
      usage();
      return 1;
   }

   int port;
   try
   {
      port = std::stoi(argv[2], nullptr);
   }
   catch (const std::invalid_argument& e)
   {
      std::cerr << "my-router: Invalid port: " << argv[1] << std::endl;
      usage();
      return 1;
   }

   std::vector<nodeinfo> neighbor_info;
   if (read_file(argv[3], neighbor_info, argv[1]) != 0)
   {
      std::cerr << "my-router: Error reading " << argv[2] << std::endl;
      usage();
      return 1;
   }

   int listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
   int out_socket = socket(AF_INET, SOCK_DGRAM, 0);
   if (listen_socket == -1 || out_socket == -1)
   {
      std::cerr << "my-router: Error creating socket" << std::endl;
      return 1;
   }

   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if (bind(listen_socket, (sockaddr*)&addr, sizeof(addr)) == -1)
   {
      std::cerr << "my-router: Error binding socket" << std::endl;
      return 1;
   }

   // Initialize the distance vector
   std::unordered_map<std::string, dv_entry> dv;
   std::unordered_map<short, std::string> portToNode;
   for (auto iter = neighbor_info.begin(); iter != neighbor_info.end(); iter++)
   {
      dv[iter->dest_router] = std::make_pair(iter->cost, iter->source_portno);
      portToNode[iter->source_portno] = iter->dest_router;
   }

   // Broadcast DV
   broadcastDV(dv, neighbor_info, out_socket);

   while (true)
   {
      char buf[MAX_PACKET_SIZE];
      sockaddr_in src_addr;
      socklen_t addr_len = sizeof(src_addr);
      int n = recvfrom(listen_socket, buf, MAX_PACKET_SIZE, 0,
                       (sockaddr*)&src_addr, &addr_len);
      if (n <= 0)
         continue;

      char packet_type = buf[0];
      switch (packet_type)
      {
         case CONTROL: {
            std::list<std::pair<std::string, int>> lcp;
            get_lcp(std::string(buf+1, n-1), lcp);

            updateDV(dv, 0, lcp);
            break;
         }
         case DATA:
            break;
      }
   }
}
