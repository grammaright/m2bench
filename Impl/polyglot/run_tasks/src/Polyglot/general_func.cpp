
#include <chrono>
#include <string>
#include <tuple>

#include "Connection/Connection.h"
#include "Polyglot/func.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace duckdb;
using namespace prevision;
using namespace std::chrono;
using namespace std::chrono::_V2;

PFpage *pvGetBuffer(string arrName, std::vector<uint64_t> &dcoords,
                    emptytile_template_type_t type) {
  PFpage *page;
  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords.data();
  key.dim_len = dcoords.size();
  key.emptytile_template = type;

  BF_GetBuf(key, &page);

  delete key.arrayname;

  return page;
}

void pvUnpinBuffer(string arrName, std::vector<uint64_t> &dcoords) {
  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords.data();
  key.dim_len = dcoords.size();
  key.emptytile_template = BF_EMPTYTILE_NONE;
  BF_UnpinBuf(key);

  delete key.arrayname;
}
