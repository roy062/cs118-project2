#include <list>
#include <string>
#include <utility>
#include <cstdlib>
#include <iostream>
void get_lcp(const std::string &lcpinfo, std::list<std::pair<std::string, int> > &lcplist)
{
  
  int start_index = 0;
  int lcp_num;
  std::string name;
  std::string lcp;

  for (int i = 0; i < lcpinfo.length(); i++)
    {
      if (lcpinfo[i] == 0)
	{
	  name = lcpinfo.substr(start_index, i - start_index);
	  lcp = lcpinfo.substr(i + 1, 4);

	  lcp_num = (lcp[0] << 24) + (lcp[1] << 16) + (lcp[2] << 8) + lcp[3];
	  
	  std::pair<std::string, int> val_pair;
	  val_pair = std::make_pair( name, lcp_num );
	  lcplist.push_back(val_pair);

	  start_index = i + 5;
    i += 4;
	}
    }
}

