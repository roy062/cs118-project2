#include <map>
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <list>
#include "router.h"

using namespace std;

void writeTable(ofstream& fout, map<string, dv_entry> table, unsigned short current_port)
{
	// Columns 
	fout << "Destination" << "          " << "Cost" << "          " << "Outgoing Port" 
		 << "          " << "Destination Port\n";

	// Print table info
	for (map<string,dv_entry>::iterator it = table.begin(); it != table.end(); it++)
	{
		fout << left; 
		fout << setw(11) << it->first << "          ";
		fout << setw(10) << it->second.first << "    ";
		fout << setw(13) << current_port << "          ";
		fout << setw(16) << it->second.second;
		fout << endl;
	}

	fout << endl;
}

void writeTime(ofstream &fout)
{
  // ANSI C asctime format
  time_t rawtime;
  tm* timeinfo;

  time (&rawtime);
  timeinfo = localtime(&rawtime);
  string time = asctime(timeinfo);
  time = time.substr(0, time.length() - 1); // get rid of newline

  fout << "*----------------------------------*\n|     " 
	   << time  << "     |\n"
	   << "*----------------------------------*\n";
}

void writeDV(ofstream &fout, list<pair<string, int>> dv, string node_name)
{
	// Record change
	fout << "Distance vector of node " << node_name << " is size " << dv.size() << ":" <<endl;

	// Columns
	fout << "Destination" << "          " << "Cost\n";

	// Print table info
	for (list<pair<string, int>>::iterator it = dv.begin(); it != dv.end(); it++)
	{
		fout << left; 
		fout << setw(11) << it->first << "          ";
		fout << setw(10) << it->second << "    ";
		fout << endl;
	}

	fout << endl;
};


/*
int main()
{
	map<string, dv_entry> m;
	dv_entry dv1 = make_pair(3, 10001);
	m["B"] = dv1;
	dv_entry dv2 = make_pair(1, 10005);
	m["E"] = dv2;

	// timestamp, table, | dv, table
	ofstream fout;

	fout.open("test.txt");
	writeTime(fout);
	writeTable(m,10000,fout);

	dv_entry dv3 = make_pair(7, 10002);
	m["C"] = dv3;

	list<pair<string, int>> l;
	l.push_back(make_pair("B", 3));
	l.push_back(make_pair("E", 1));
	l.push_back(make_pair("C", 7));

	writeDV(fout, l, "A");
	writeTable(m, 10000, fout);

	writeTime(fout);
	writeTable(m,10000,fout);
	writeDV(fout, l, "A");
	writeTable(m, 10000, fout);
	fout.close();

}
*/
