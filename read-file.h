#ifndef READ_FILE_H
#define READ_FILE_H

#include <string>

int read_file(const std::string& filename,
              std::vector<nodeinfo> &nodevec,
              const std::string& source);

#endif
