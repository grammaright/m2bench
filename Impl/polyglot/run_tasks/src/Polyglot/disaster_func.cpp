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

string ChunkProcessing(PolyglotConnection &conn,
                       std::shared_ptr<prevision::ArrayQuery> in, int start,
                       int end, int farStart, uint64_t &docTime,
                       uint64_t &arrTime) {
  auto &dconn = conn.GetDuckdbConnection();
  auto pvEngine = conn.GetPrevisionEngine();

  auto arrStart = system_clock::now();

  std::vector<uint32_t> _begin = {(uint32_t)start, 0, 0},
                        _end = {(uint32_t)end + 1, 523, 523},
                        _tilesize = {(uint32_t)end + 1 - start, 523, 523};

  auto B = prevision::Subarray(in, {_begin, _end}, _tilesize);
  auto C = prevision::Topk(B, prevision::TopkType::MAX, 1);
  pvEngine->Execute(*C);

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  // get buffer
  std::vector<uint64_t> dcoords = {0, 0, 0};
  PFpage *page = pvGetBuffer(C->getArrayName(), dcoords, BF_EMPTYTILE_NONE);

  // insert data to D1
  uint64_t *ts = bf_util_pagebuf_get_coords(page, 0);
  uint64_t *lat = bf_util_pagebuf_get_coords(page, 1);
  uint64_t *lon = bf_util_pagebuf_get_coords(page, 2);
  double *buf = (double *)bf_util_get_pagebuf(page);

  auto docStart = system_clock::now();
  static int num = 0;
  string tblname = "D1_" + to_string(num++);
  dconn
      .Query(R"(
    CREATE TEMP TABLE )" +
             tblname + R"( AS 
    SELECT doc_make('{"timestamp": ' || )" +
             to_string(ts[0] + start + farStart) +
             R"( || ', "latitude": ' || )" + to_string(lat[0]) +
             R"( || ', "longitude": ' || )" + to_string(lon[0]) +
             R"(|| ', "pm10_avg": ' || )" + to_string(buf[0]) +
             R"( || '}') AS data
  )")
      ->Print();
  docTime += duration_cast<nanoseconds>(system_clock::now() - docStart).count();

  arrStart = system_clock::now();
  // unpin buffer
  pvUnpinBuffer(C->getArrayName(), dcoords);
  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  return tblname;
}