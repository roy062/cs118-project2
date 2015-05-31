#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char **argv)
{
   int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (socket_fd == -1)
   {
      std::cerr << "my-router: Error creating socket" << std::endl;
      return 1;
   }

   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(10000);
   addr.sin_addr.s_addr = INADDR_ANY;

   std::string text;
   if (argc < 3)
      text = std::string("\x01" "A\0\0\0\0\0\0\0\0\0\0\0" "D\0\0\0\0\0\0\0\0\0\0\0" "\xff" "\xff" "\x00" "\x04" "doot", 33);
   else
   {
      text = argv[1];
   }

   sendto(socket_fd, text.c_str(), text.length(), 0, (sockaddr*)&addr, sizeof(addr));
   close(socket_fd);

   return 0;
}
