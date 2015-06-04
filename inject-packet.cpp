#include <iostream>
#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void buildMap(std::unordered_map<std::string, unsigned short>& my_map)
{
   my_map = std::unordered_map<std::string, unsigned short>();

   my_map["A"] = 10000;
   my_map["B"] = 10001;
   my_map["C"] = 10002;
   my_map["D"] = 10003;
   my_map["E"] = 10004;
   my_map["F"] = 10005;
}

int main(int argc, char **argv)
{
   int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (socket_fd == -1)
   {
      std::cerr << "my-router: Error creating socket" << std::endl;
      return 1;
   }

   std::unordered_map<std::string, unsigned short> my_map;
   buildMap(my_map);

   std::string src_id;
   std::string dest_id;
   unsigned short src_port;
   unsigned short dest_port;
   std::string message;
   if (argc == 1)
   {
      src_id = "A";
      dest_id = "F";
      src_port = 10000;
      dest_port = 10005;
      message = "doot doot";
   }
   else if (argc == 3)
   {
      if (my_map.count(argv[1]) == 0 || my_map.count(argv[2]) == 0)
      {
         std::cerr << "Unknown ports" << std::endl;
         return 1;
      }
      src_id = argv[1];
      dest_id = argv[2];
      src_port = my_map[src_id];
      dest_port = my_map[dest_id];
      message = "doot doot";
   }
   else if (argc == 4)
   {
      if (my_map.count(argv[1]) == 0 || my_map.count(argv[2]) == 0)
      {
         std::cerr << "Unknown ports" << std::endl;
         return 1;
      }
      src_id = argv[1];
      dest_id = argv[2];
      src_port = my_map[src_id];
      dest_port = my_map[dest_id];
      message = argv[3];
   }
   else
   {
      std::cerr << "Bad number of arguments" << std::endl;
      return 1;
   }

   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(src_port);
   addr.sin_addr.s_addr = INADDR_ANY;

   // Make the header
   std::string packet;
   // Write out control packet type
   packet += '\x01';

   // Write source and destination nodes
   for (int i = 0; i < 12; i++)
   {
      if (i < src_id.size() && i != 11)
         packet += src_id[i];
      else
         packet += '\0';
   }
   for (int i = 0; i < 12; i++)
   {
      if (i < dest_id.size() && i != 11)
         packet += dest_id[i];
      else
         packet += '\0';
   }

   // First hop has port number 65535, by convention
   packet += "\xff" "\xff";

   // Write out message length
   size_t msg_len = message.size();
   if (msg_len > 4096)
      msg_len = 4096;
   packet += (char)((msg_len >> 8) & 0xFF);
   packet += (char)(msg_len & 0xFF);

   // Write out message itself
   packet += message;
   
/*
   if (argc < 3)
      packet = std::string("\x01" "A\0\0\0\0\0\0\0\0\0\0\0" "D\0\0\0\0\0\0\0\0\0\0\0" "\xff" "\xff" "\x00" "\x04" "doot", 33);
*/

   sendto(socket_fd, packet.c_str(), 29 + msg_len, 0, (sockaddr*)&addr, sizeof(addr));
   close(socket_fd);

   return 0;
}
