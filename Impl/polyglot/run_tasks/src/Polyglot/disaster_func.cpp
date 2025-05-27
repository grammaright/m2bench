
#include <string>
#include <tuple>

#include "Connection/Connection.h"
#include "Polyglot/func.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace duckdb;
using namespace prevision;

PFpage *t14GetBuffer(string arrName) {
  // assume that there is only one tile
  uint64_t dcoords[] = {0, 0, 0};

  PFpage *page;
  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords;
  key.dim_len = 3;
  key.emptytile_template = BF_EMPTYTILE_NONE;

  BF_GetBuf(key, &page);

  delete key.arrayname;

  return page;
}

void t14UnpinBuffer(string arrName) {
  uint64_t dcoords[] = {0, 0, 0};

  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords;
  key.dim_len = 3;
  key.emptytile_template = BF_EMPTYTILE_NONE;
  BF_UnpinBuf(key);

  delete key.arrayname;
}

PFpage *t15t16GetBuffer(string arrName) {
  // assume that there is only one tile
  uint64_t dcoords[] = {0, 0};

  PFpage *page;
  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_NONE;

  BF_GetBuf(key, &page);

  delete key.arrayname;

  return page;
}

void t15t16UnpinBuffer(string arrName) {
  uint64_t dcoords[] = {0, 0};

  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_NONE;
  BF_UnpinBuf(key);

  delete key.arrayname;
}

void ChunkProcessing(PolyglotConnection &conn,
                     std::shared_ptr<prevision::ArrayQuery> in, int start,
                     int end, int farStart) {
  auto &dconn = conn.GetDuckdbConnection();
  auto pvEngine = conn.GetPrevisionEngine();

  std::vector<uint32_t> _begin = {(uint32_t)start, 0, 0},
                        _end = {(uint32_t)end + 1, 523, 523},
                        _tilesize = {(uint32_t)end + 1 - start, 523, 523};

  auto B = prevision::Subarray(in, {_begin, _end}, _tilesize);
  auto C = prevision::Topk(B, prevision::TopkType::MAX, 1);
  pvEngine->Execute(*C);

  // get buffer
  PFpage *page = t14GetBuffer(C->getArrayName());

  // insert data to D1
  uint64_t *ts = bf_util_pagebuf_get_coords(page, 0);
  uint64_t *lat = bf_util_pagebuf_get_coords(page, 1);
  uint64_t *lon = bf_util_pagebuf_get_coords(page, 2);
  double *buf = (double *)bf_util_get_pagebuf(page);

  dconn
      .Query(
          "INSERT INTO D1 VALUES( "
          "doc_make('{\"longitude\": " +
          to_string(lon[0]) +
          ", "
          "\"latitude\": " +
          to_string(lat[0]) +
          ", "
          "\"date\": " +
          to_string(((int)ts[0] + farStart + start) / 8) +
          ", "
          "\"timestamp\": " +
          to_string(ts[0] + farStart + start) +
          ", "
          "\"pm10_avg\": " +
          to_string(buf[0]) + "}'))")
      ->Print();

  // unpin buffer
  t14UnpinBuffer(C->getArrayName());
}