#include <fstream>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

#include <sys/socket.h>
#include <netinet/in.h>

#include "node-info.h"
#include "read-file.h"
#include "read-dv.h"
#include "router.h"
#include "write-table.h"

void broadcastDV(const std::map<std::string, dv_entry>& dv, 
                 const std::vector<nodeinfo>& neighbor_info,
                 unsigned short this_port,
                 int out_socket)
{
   std::string packet = std::string();

   // Write header bytes
   packet += CONTROL;

   this_port = htons(this_port);
   packet += (char)(this_port & 0xFF);
   packet += (char)((this_port >> 8) & 0xFF);

   // Write payload bytes
   for (auto iter = dv.cbegin(); iter != dv.cend(); iter++)
   {
      packet += iter->first;   // Write dest node name
      packet += '\0';

      // Write cost in big-endian
      int cost = htonl(iter->second.first);
      packet += (char)(cost & 0xFF);
      packet += (char)((cost >> 8) & 0xFF);
      packet += (char)((cost >> 16) & 0xFF);
      packet += (char)((cost >> 24) & 0xFF);
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

bool updateDV(std::map<std::string, dv_entry>& dv,
              const std::list<std::pair<std::string, int>>& lcp,
              std::map<unsigned short, std::string>& port_to_node,
              int link_cost,
              unsigned short source_port,
              unsigned short this_port,
              std::ofstream& fout)
{
   bool dv_changed = false;
   const std::string source_id = port_to_node[source_port];

   for (auto iter = lcp.begin(); iter != lcp.end(); iter++)
   {
      int path_cost = iter->second + link_cost;

      // Do not change if destination is already in table and has a better path
      if (dv.count(iter->first) > 0)
      {
         // Either the path is of lower cost
         if (dv[iter->first].first < path_cost)
            continue;

         // Or the paths are of equal cost but the next router has a lower ID
         if (dv[iter->first].first == path_cost
             && port_to_node[dv[iter->first].second] <= source_id)
            continue;
      }

      if (!dv_changed)
      {
         // DV is about to change, so print out the old one before modifying it
         writeTime(fout);
         writeTable(fout, dv, this_port);
         writeDV(fout, lcp, source_id);
         dv_changed = true;
      }
      dv[iter->first] = std::make_pair(path_cost, source_port);
   }

   return dv_changed;
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
      std::cerr << "my-router: (Node " << id << ") Invalid port: " << argv[1]
                << std::endl;
      usage();
      return 1;
   }

   std::vector<nodeinfo> neighbor_info;
   if (read_file(argv[3], neighbor_info, argv[1]) != 0)
   {
      std::cerr << "my-router: (Node " << id << ") Error reading " << argv[2]
                << std::endl;
      usage();
      return 1;
   }

   int listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
   int out_socket = socket(AF_INET, SOCK_DGRAM, 0);
   if (listen_socket == -1 || out_socket == -1)
   {
      std::cerr << "my-router: (Node " << id << ") Error creating socket"
                << std::endl;
      return 1;
   }

   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if (bind(listen_socket, (sockaddr*)&addr, sizeof(addr)) == -1)
   {
      std::cerr << "my-router: (Node " << id << ") Error binding socket"
                << std::endl;
      return 1;
   }

   // Initialize the distance vector
   std::map<std::string, dv_entry> dv;
   std::map<unsigned short, int> link_costs;
   std::map<unsigned short, std::string> port_to_node;
   dv[id] = std::make_pair(0, port);
   for (auto iter = neighbor_info.begin(); iter != neighbor_info.end(); iter++)
   {
      dv[iter->dest_router] = std::make_pair(iter->cost, iter->source_portno);
      link_costs[iter->source_portno] = iter->cost;
      port_to_node[iter->source_portno] = iter->dest_router;
   }

   // Open a file for logging
   std::ofstream fout;
   std::string filename = "routing-output" + id + ".txt";
   fout.open(filename, std::ofstream::out | std::ofstream::app);

   // Print out the initial distance vector
   writeTime(fout);
   writeTable(fout, dv, port);

   // Broadcast DV
   broadcastDV(dv, neighbor_info, port, out_socket);

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
            // CONTROL packet header:
            // 1 byte -- 0, to indicate control packet
            // 2 byte -- source port

            // Ignore control packets from non-registered neighbors
            unsigned short src_port = (buf[1] << 8) + buf[2];
            if (port_to_node.count(src_port) == 0)
               break;

            std::list<std::pair<std::string, int>> lcp;
            get_lcp(std::string(buf+3, n-1), lcp);

            if (updateDV(dv, lcp, port_to_node, link_costs[src_port], src_port,
                         port, fout))
            {
               writeTable(fout, dv, port);
               broadcastDV(dv, neighbor_info, port, out_socket);
            }
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
