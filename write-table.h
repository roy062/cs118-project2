#ifndef WRITE_TABLE_H
#define WRITE_TABLE_H

#include <fstream>
#include <list>
#include <string>
#include <map>

#include "router.h"

void writeTable(std::ofstream& fout,
                std::map<std::string, dv_entry> table,
                unsigned short current_port);

void writeTime(std::ofstream& fout);

void writeDV(std::ofstream& fout,
             std::list<std::pair<std::string, int>> dv,
             std::string node_name);
             

#endif
