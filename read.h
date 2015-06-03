#ifndef READ_H
#define READ_H

#include <list>
#include <vector>
#include <string>

#include "router.h"

void getLCP(const std::string& lcpinfo,
	    std::list<std::pair<std::string, int>>& lcplist);

int readFile( const std::string& filename,
              std::vector<nodeinfo> &nodevec,
              const std::string& source);

#endif
