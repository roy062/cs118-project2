#include <chrono>
#include <climits>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>

#include "router.h"
#include "read.h"
#include "write.h"

std::vector<nodeinfo> neighbor_info;
std::map<std::string, dv_entry> dv;
std::mutex dv_mutex;
std::map<unsigned short, int> link_costs;
std::map<unsigned short, std::string> port_to_node;
std::map<unsigned short, time_t> last_heard;
std::mutex last_heard_mutex;

std::ofstream fout;

std::string id;
unsigned short port;
int listen_socket;
int out_socket;

// Makes a control packet to advertise a distance vector. Applies a poisoned
// reverse for dest_port.
/*void makeControlPacket(const std::map<std::string, dv_entry>& dv,
                       unsigned short this_port,
                       unsigned short dest_port,
                       std::string& packet)*/
void makeControlPacket(unsigned short dest_port, std::string& packet)
{
   packet = std::string();
   
   // Write header bytes
   packet += CONTROL;

   unsigned short this_port = htons(port);
   packet += (char)(this_port & 0xFF);
   packet += (char)((this_port >> 8) & 0xFF);

   // Write payload bytes
   for (auto iter = dv.cbegin(); iter != dv.cend(); iter++)
   {
      // Write dest node
      packet += iter->first;
      packet += '\0';

      // Write cost in big-endian; apply poisoned reverse here if possible
      int cost;
      if (iter->second.second == dest_port)
         cost = htonl(INT_MAX);
      else
         cost = htonl(iter->second.first);
      std::cout << cost << std::endl;

      packet += (char)(cost & 0xFF);
      packet += (char)((cost >> 8) & 0xFF);
      packet += (char)((cost >> 16) & 0xFF);
      packet += (char)((cost >> 24) & 0xFF);
   }
}
/*
void broadcastDV(const std::map<std::string, dv_entry> *dv, 
                 const std::vector<nodeinfo>& neighbor_info,
                 unsigned short this_port,
                 int out_socket)
*/
void broadcastDV()
{
   std::string packet = std::string();

   sockaddr_in dest_addr;
   dest_addr.sin_family = AF_INET;
   dest_addr.sin_addr.s_addr = INADDR_ANY;
   for (auto iter = neighbor_info.begin(); iter != neighbor_info.end(); iter++)
   {
//      makeControlPacket(dv, this_port, iter->source_portno, packet);
      makeControlPacket(iter->source_portno, packet);
      dest_addr.sin_port = htons(iter->source_portno);
      sendto(out_socket, packet.c_str(), packet.size(), 0,
             (sockaddr*)&dest_addr, sizeof(dest_addr));
   }
}

/*
bool updateDV(std::map<std::string, dv_entry>& dv,
              const std::list<std::pair<std::string, int>>& lcp,
              std::map<unsigned short, std::string>& port_to_node,
              int link_cost,
              unsigned short source_port,
              unsigned short this_port,
              std::ofstream& fout)
*/
bool updateDV(const std::list<std::pair<std::string, int>>& lcp,
              int link_cost,
              unsigned short source_port)
{
   bool dv_changed = false;
   const std::string source_id = port_to_node[source_port];

   for (auto iter = lcp.begin(); iter != lcp.end(); iter++)
   {
      // INT_MAX implies infinite distance; this is used for poisoned reverse
      if (iter->second == INT_MAX)
         continue;

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
         writeTable(fout, dv, port);
         writeDV(fout, lcp, source_id);
         dv_changed = true;
      }
      dv[iter->first] = std::make_pair(path_cost, source_port);
   }

   return dv_changed;
}

/*void doBroadcast(const std::map<std::string, dv_entry> *dv,
                 const std::vector<nodeinfo>& neighbor_info,
                 unsigned short this_port,
                 int out_socket,
                 std::mutex *dv_mutex)*/
void doBroadcast()
{
   while (true)
   {
      dv_mutex.lock();
      broadcastDV();
      dv_mutex.unlock();
//      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::this_thread::sleep_for(BROADCAST_PERIOD);
   }
}

/*void checkExpiredRoutes(std::map<std::string, dv_entry> *dv,
                        const std::map<unsigned short, time_t> *last_heard,
                        std::map<unsigned short, std::string>& port_to_node,
                        unsigned short this_port,
                        std::mutex *dv_mutex,
                        std::mutex *last_heard_mutex,
                        std::ofstream *fout)*/
void checkExpiredRoutes()
{
   while (true)
   {
      std::this_thread::sleep_for(NODE_CHECK_PERIOD);

      dv_mutex.lock();
      last_heard_mutex.lock();
      time_t cur_time = time(NULL);
      bool node_died = false;
      bool dv_changed = false;
      auto iter = last_heard.begin();
      while (iter != last_heard.end())
      {
         if (cur_time - (iter->second) > NODE_TTL)
         {
            if (!node_died)
            {
               writeTime(fout);
               node_died = true;
            }
            // Delete any DV entries that use the dead node as the next hop
            auto dv_iter = dv.begin();
            while (dv_iter != dv.end())
            {
               if (dv_iter->second.second == iter->first)
               {
                  dv_changed = true;
                  dv_iter = dv.erase(dv_iter);
               }
               else
                  dv_iter++;
            }
            writeExpireMsg(fout, port_to_node[iter->first]);
            iter = last_heard.erase(iter);
         }
         else
            iter++;
      }

      if (dv_changed)
         writeTable(fout, dv, port);

      last_heard_mutex.unlock();
      dv_mutex.unlock();
   }
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

   id = argv[1];

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

   if (readFile(argv[3], neighbor_info, argv[1]) != 0)
   {
      std::cerr << "my-router: (Node " << id << ") Error reading " << argv[2]
                << std::endl;
      usage();
      return 1;
   }

   listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
   out_socket = socket(AF_INET, SOCK_DGRAM, 0);
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
   dv[id] = std::make_pair(0, port);
   for (auto iter = neighbor_info.begin(); iter != neighbor_info.end(); iter++)
   {
      dv[iter->dest_router] = std::make_pair(iter->cost, iter->source_portno);
      link_costs[iter->source_portno] = iter->cost;
      port_to_node[iter->source_portno] = iter->dest_router;
   }

   // Open a file for logging
   std::string filename = "routing-output" + id + ".txt";
   fout.open(filename, std::ofstream::out | std::ofstream::app);

   // Print out the initial distance vector
   writeTime(fout);
   writeTable(fout, dv, port);

   // Spawn a thread to broadcast DV periodically
//   std::thread broadcaster(doBroadcast, &dv, neighbor_info, port, out_socket,
//                           &dv_mutex);
   std::thread broadcaster(doBroadcast);

   // Spawn a thread to check for dead nodes periodically
//   std::thread route_expiration(checkExpiredRoutes, &dv, &last_heard,
//                                port_to_node, port, &dv_mutex,
//                                &last_heard_mutex, &fout);
   std::thread route_expiration(checkExpiredRoutes);

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
            getLCP(std::string(buf+3, n-1), lcp);

            last_heard[src_port] = time(NULL);

            dv_mutex.lock();
//            if (updateDV(dv, lcp, port_to_node, link_costs[src_port], src_port,
//                         port, fout))
            if (updateDV(lcp, link_costs[src_port], src_port))
               writeTable(fout, dv, port);
            dv_mutex.unlock();
            break;
         }
         case DATA: {
            // DATA packet header:
            // 1 byte   -- 1, to indicate data packet
            // 12 bytes -- source node, as a null-terminated string
            // 12 bytes -- destination node, as a null-terminated string
            // 2 bytes  -- src port
            // 2 bytes  -- packet length

            std::string src_node(buf+1);
            std::string dest_node(buf+13);
            unsigned short src_port = ((unsigned)buf[25] << 8) | (unsigned)buf[26];

            dv_mutex.lock();
            if (dest_node == id)  // Arrived at destination
               writePacketInfo(fout, src_node, dest_node, src_port, 0,
                               std::string(buf+29, n-29), FINAL_DEST);
            else if (dv.count(dest_node) > 0)  // Can forward packet
            {
               sockaddr_in dest_addr;
               dest_addr.sin_family = AF_INET;
               dest_addr.sin_addr.s_addr = INADDR_ANY;
               dest_addr.sin_port = htons(dv[dest_node].second);
               
               buf[25] = (port >> 8) & 0xFF;
               buf[26] = port & 0xFF;

               sendto(out_socket, buf, n, 0, (sockaddr*)&dest_addr,
                      sizeof(dest_addr));

               writePacketInfo(fout, src_node, dest_node, src_port,
                               dv[dest_node].second, "", NORMAL);
            }
            else  // Don't know how to forward packet
               writePacketInfo(fout, src_node, dest_node, src_port, 0, "",
                               ERROR);
            dv_mutex.unlock();

            break;
         }
      }
   }
}
