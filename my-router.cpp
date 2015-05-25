#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>

#include "node-info.h"
#include "read-file.h"

const int MAX_PACKET_SIZE = 4096;

enum PacketType {
  CONTROL = 0,
  DATA = 1
};

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
   if (listen_socket == -1)
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
      case CONTROL:
        break;
      case DATA:
        break;
      }

      std::cout << buf << std::endl;
   }
}
