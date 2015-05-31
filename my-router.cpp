#include <fstream>
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
#include "router.h"

void write_table(std::ofstream& fout,
                 const std::unordered_map<std::string, dv_entry>& dv,
                 unsigned short source_port,
                 unsigned short this_port)
{
}

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
             const std::list<std::pair<std::string, int>>& lcp,
             std::unordered_map<unsigned short, std::string>& port_to_node,
             unsigned short source_port,
             unsigned short this_port,
             std::ofstream& fout)
{
   bool dv_changed = false;
   const std::string source_id = port_to_node[source_port];

   for (auto iter = lcp.begin(); iter != lcp.end(); iter++)
   {
      int path_cost = iter->second + dv[source_id].first;

      // Do not change if destination is already in table and has a better path
      if (dv.count(iter->first) > 0)
      {
         // Either the path is of lower cost
         if (dv[iter->first].first < path_cost)
            continue;

         // Or the paths are of equal cost but the next router has a lower ID
         if (dv[iter->first].first == path_cost
             && port_to_node[dv[iter->first].second] <= iter->first)
            continue;
      }

      if (!dv_changed)
      {
         // DV is about to change, so print out the old one before modifying it
         write_table(fout, dv, source_port, this_port);
         dv_changed = true;
      }
      dv[iter->first] = std::make_pair(path_cost, source_port);
   }

   if (dv_changed)
      write_table(fout, dv, source_port, this_port);
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

   std::string id = argv[1];

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
   std::unordered_map<unsigned short, std::string> port_to_node;
   for (auto iter = neighbor_info.begin(); iter != neighbor_info.end(); iter++)
   {
      dv[iter->dest_router] = std::make_pair(iter->cost, iter->source_portno);
      port_to_node[iter->source_portno] = iter->dest_router;
   }

   // Broadcast DV
   broadcastDV(dv, neighbor_info, out_socket);

   // Open a file for logging
   std::ofstream fout;
   std::string filename = "routing-output" + id + ".txt";
   fout.open(filename, std::ofstream::out | std::ofstream::app);

   while (true)
   {
      char buf[MAX_PACKET_SIZE];
      sockaddr_in src_addr;
      socklen_t addr_len = sizeof(src_addr);
      int n = recvfrom(listen_socket, buf, MAX_PACKET_SIZE, 0,
                       (sockaddr*)&src_addr, &addr_len);
      if (n <= 0)
         continue;

      unsigned short src_port = ntohs(src_addr.sin_port);
      char packet_type = buf[0];
      switch (packet_type)
      {
         case CONTROL: {
            // CONTROL packet header:
            // 1 byte -- 0, to indicate control packet


            // Ignore control packets from non-registered neighbors
            if (port_to_node.count(src_port) == 0)
               break;

            std::list<std::pair<std::string, int>> lcp;
            get_lcp(std::string(buf+1, n-1), lcp);

            updateDV(dv, lcp, port_to_node, src_port, port, fout);
            break;
         }
         case DATA:
            // DATA packet header:
            // 1 byte   -- 1, to indicate data packet
            // 12 bytes -- destination node, as a null-terminated string
            // 2 bytes  -- packet length
            std::string dest_node(buf+1);

            if (dest_node == id)  // Arrived at destination
            {
               std::string payload(buf+13);
               /// TODO: write payload
            }
            else if (dv.count(dest_node) > 0)  // Can forward packet
            {
                  sockaddr_in dest_addr;
                  dest_addr.sin_family = AF_INET;
                  dest_addr.sin_addr.s_addr = INADDR_ANY;
                  dest_addr.sin_port = htons(dv[dest_node].second);
                  sendto(out_socket, buf, n, 0, (sockaddr*)&dest_addr,
                         sizeof(dest_addr));

                  /// TODO: write to log
            }
            else  // Don't know how to forward packet
            {
               /// TODO: write to log
               ;
            }

            break;
      }
   }
}
