#ifndef WRITE_H
#define WRITE_H

#include <fstream>
#include <list>
#include <string>
#include <map>

#include "router.h"

void writeInitialization(std::ofstream& fout, const std::string& id,
                         unsigned short port);

void writeTable(std::ofstream& fout,
                std::map<std::string, dv_entry> table,
                unsigned short current_port);

void writeTime(std::ofstream& fout);

void writeDV(std::ofstream& fout,
             std::list<std::pair<std::string, int>> dv,
             const std::string &node_name);

void writePacketInfo(std::ofstream &fout, 
                     const std::string &source_node, 
                     const std::string &dest_node, 
                     unsigned short arrival_port, 
                     unsigned short outgoing_port, 
                     const std::string &payload, 
                     print_type ptype);

void writeExpireMsg(std::ofstream &fout,
		    const std::string &expired_node);


#endif
