#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
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
  
  if (bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) == -1)
    {
      std::cerr << "inject-packet: Error binding socket" << std::endl;
      return 1;
    }

  std::string text = "bunch of data in a packet...";
  write(socket_fd, text.c_str(), text.length()); 
  close(socket_fd);

  return 0;

}
