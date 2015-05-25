#include "node-info.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <vector>

using namespace std;

void read_file(string filename, vector<nodeinfo> &nodevec, string source)
{
  ifstream fin;
  fin.open(filename.c_str());

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
		    {
		      field_start = -1;
		      break;
		    }
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
	      
	      field++;
	      field_start = i + 1;
	    }
	}

      // Add link information to vector

      if (field_start != -1)
	nodevec.push_back(n);
    }

  fin.close();
}

/*
int main()
{
  vector<nodeinfo> nodelist;
  read_file("nodes.txt", nodelist, "C");

  for (int i = 0; i < nodelist.size(); i++)
    {
      cout << nodelist[i].dest_router << " ";
      cout << nodelist[i].source_portno << " ";
      cout << nodelist[i].cost << endl;
    }

  return 0;

}
*/
