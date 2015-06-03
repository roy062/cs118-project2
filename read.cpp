#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <utility>
#include <cstdlib>
#include "read.h"

using namespace std;

void getLCP(const string &lcpinfo, list<pair<string, int> > &lcplist)
{
  
   int start_index = 0;
   int lcp_num;
   string name;
   string lcp;

   for (int i = 0; i < lcpinfo.length(); i++)
   {
      if (lcpinfo[i] == 0)
      {
         name = lcpinfo.substr(start_index, i - start_index);

         // Break out to ignore this entry if there's not enough data left
         if (lcpinfo.length() < i + 5)
            break;

         lcp = lcpinfo.substr(i + 1, 4);

         lcp_num = ((unsigned char)lcp[0] << 24) | ((unsigned char)lcp[1] << 16) | ((unsigned char)lcp[2] << 8) | (unsigned char)lcp[3];
	  
         pair<string, int> val_pair;
         val_pair = make_pair( name, lcp_num );
         lcplist.push_back(val_pair);

         start_index = i + 5;
         i += 4;
      }
   }
}

int readFile(const string &filename, vector<nodeinfo> &nodevec, const string &source)
{
  ifstream fin;
  
  fin.open(filename.c_str());
  if (fin.fail())
    return -1;

  string line;
  nodeinfo n;

  // Read lines from file
  
  while ( getline(fin, line) )
    {
      int field = 0;
      int field_start = 0;

      // Parse line
      
      for (int i = 0; i < line.length(); i++)
	{
	  if (line[i] == ',' || i == line.length() - 1)
	    {
	      if (field == 0)
		{
		  if (line.substr(0,i) != source)
		      break;
		}
	      else if (field == 1)
		{
		  n.dest_router  = line.substr(field_start, i - field_start);
		}
	      else if (field == 2)
		{
		  string port = line.substr(field_start, i - field_start);
		  n.source_portno = atoi(port.c_str());

		}
	      else if (field == 3)
		{
		  string cost = line.substr(field_start, i + 1 - field_start);
		  n.cost = atoi(cost.c_str());
		}
	      else
		return -1;  // parsing error
	      
	      field++;
	      field_start = i + 1;
	    }
	}

      // Add link information to vector

      if (field == 4)
	nodevec.push_back(n);
    }

  fin.close();
  return 0;
}



