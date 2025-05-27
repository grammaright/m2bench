#include "Connection/Connection.h"
#include "Polyglot/func.h"
#include "velocypack/vpack.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace std;
using namespace duckdb;
using namespace arangodb::velocypack;

/*
 * [Task 14] Sources of Fine Dust.
 *
 * Analyze fine dust hotspot by date between time Z1 and Z2.
 * Print the nearest building with time of the hotspot.
 * Use window aggregation with a size of 5. (Document, Array) -> Document
 *
 */
void T14(int SF, bool isValidation) {
  const int Z1 = 5 * SF;
  const int Z2 = 10 * SF;

  PolyglotConnection conn(true, "disaster", true);
  auto &dconn = conn.GetDuckdbConnection();

  auto finedust = prevision::OpenArray("finedust");
  auto pm10 = prevision::Project(finedust, {0});

  std::vector<uint32_t> _begin = {(uint32_t)5 * SF, 0, 0},
                        _end = {(uint32_t)10 * SF + 1, 523, 523},
                        _tilesize = {(uint32_t)10 * SF + 1 - (5 * SF), 523,
                                     523};
  auto A1 = prevision::Subarray(pm10, {_begin, _end}, _tilesize);
  auto A = prevision::WindowAvg(A1, {1, 5, 5});

  // date processing
  auto start = 5 * SF;
  auto len = 10 * SF - start;
  int a = 1;
  while (a * 8 < start) {
    a++;
  }

  dconn.Query("CREATE TEMP TABLE D1 (data VPACK)");

  int curr = (a * 8) % start;
  ChunkProcessing(conn, A, 0, curr - 1, start);
  while (curr + 8 <= len) {
    ChunkProcessing(conn, A, curr, curr + 7, start);
    curr += 8;
  }
  ChunkProcessing(conn, A, curr, len, start);

  dconn
      .Query(
          "SELECT '{\"date\": ' || doc_get_int32('date', data) || ', "
          "\"timestamp\": ' || doc_get_int32('timestamp', data) || ', "
          "\"longitude\": ' || doc_get_int32('longitude', data) || ', "
          "\"latitude\": ' || doc_get_int32('latitude', data) || ', "
          "\"pm10_avg\": ' || doc_get_double('pm10_avg', data) || ', "
          "\"site_id\": ' || "
          "doc_st_closest_object_id_composite_string('Site_centroid', "
          "'properties.type', "
          "[(doc_get_int32('longitude', data)::DOUBLE * 0.000216636 - "
          "118.34501002237936), (doc_get_int32('latitude', data)::DOUBLE * "
          "0.000172998 + 34.011898718557454)], 'building') || '}' "
          "FROM D1 "
          "ORDER BY doc_get_int32('date', data)")
      ->Print();

  cout << "[TASK14]: END" << endl;
}

/*
 * [Task15] Fine Dust Cleaning Vehicles.
 *
 * Recommend the route from the current coordinates by analyzing the hotspot
 * between time Z1 and Z2. Use window aggregation with a size of 5. (Graph,
 * Document, Array) -> Relational
 *
 */
void T15(int SF, bool isValidation) {
  int Z1 = 5 * SF, Z2 = 10 * SF;
  double lon = -118.0614431, lat = 34.068509;

  PolyglotConnection conn(true, "disaster", true);
  auto &dconn = conn.GetDuckdbConnection();
  auto pvEngine = conn.GetPrevisionEngine();

  auto finedust = prevision::OpenArray("finedust");
  auto pm10 = prevision::Project(finedust, {0});

  std::vector<uint32_t> _begin = {(uint32_t)Z1, 0, 0},
                        _end = {(uint32_t)Z2 + 1, 523, 523},
                        _tilesize = {(uint32_t)Z2 + 1 - Z1, 523, 523};

  auto A1 = prevision::Subarray(pm10, {_begin, _end}, _tilesize);
  auto A2 = prevision::Stack(prevision::Sum(A1, {1, 2}),
                             prevision::Count(A1, {1, 2}));

  // custom window aggregator
  // there is no way to "sum(sum) / sum(count)" in the current implementation of
  //   PreVision, so we use WindowCustom to calculate the average value.
  // The array output is double type, so the sum of average is store in the
  //   output cells and the count value is accumulated in the vector outside.
  uint64_t lowestDimSize = 1;
  unordered_map<uint64_t, std::vector<int>> count;
  auto A = WindowCustom(
      A2, {5, 5}, std::vector<tilestore_datatype_t>{TILESTORE_FLOAT64},
      [&count](Chunk &out) {},  // Nothing to do
      [&count, lowestDimSize](Chunk &in, uint64_t inIdx, Chunk &out,
                              uint64_t outIdx) {
        // accumulate cell value to the output buffer
        uint8_t *inBuf = (uint8_t *)bf_util_get_pagebuf(in.curpage);
        size_t inDataLen = in.curpage->pagebuf_len / in.curpage->max_idx;
        double *outBuf = (double *)bf_util_get_pagebuf(out.curpage);

        // get count vector
        uint64_t _1dc = in.tile_coords[0] * lowestDimSize + in.tile_coords[1];
        if (count.find(_1dc) == count.end()) {
          count[_1dc] = std::vector<int>(in.curpage->max_idx, 0);
        }

        // the first 4 bytes are the average value and the next 4 bytes are the
        // count value
        outBuf[outIdx] += *(float *)(inBuf + inDataLen * inIdx);
        count[_1dc][outIdx] +=
            *(int *)(inBuf + inDataLen * inIdx + sizeof(float));
      },
      [&count, lowestDimSize](Chunk &out) {
        // calculate the average value
        double *outBuf = (double *)bf_util_get_pagebuf(out.curpage);
        uint64_t _1dc = out.tile_coords[0] * lowestDimSize + out.tile_coords[1];
        for (uint64_t outIdx = 0; outIdx < out.curpage->max_idx; outIdx++) {
          if (bf_util_is_cell_null(out.curpage, outIdx)) {
            continue;
          }

          outBuf[outIdx] /= count[_1dc][outIdx];
        }
      });

  auto B1 = Topk(A, prevision::TopkType::MAX, 1);
  pvEngine->Execute(*B1);

  PFpage *page = t15t16GetBuffer(B1->getArrayName());

  uint64_t *latBuf = bf_util_pagebuf_get_coords(page, 0);
  uint64_t *lonBuf = bf_util_pagebuf_get_coords(page, 1);
  double *buf = (double *)bf_util_get_pagebuf(page);

  dconn
      .Query(
          "CREATE TEMP TABLE B1 AS "
          "SELECT doc_make('{\"longitude\": " +
          to_string(lonBuf[0]) +
          ", "
          "\"latitude\": " +
          to_string(latBuf[0]) +
          ", "
          "\"pm10_avg\": " +
          to_string(buf[0]) + "}') AS data")
      ->Print();

  auto startStr =
      "doc_st_closest_object_composite_string('Site_"
      "centroid', 'properties.type', [" +
      to_string(lon) + ", " + to_string(lat) + "], 'roadnode')";
  auto endStr =
      "doc_st_closest_object_composite_string('Site_centroid', '"
      "properties.type', "
      "[(doc_get_int32('longitude', "
      "B1.data) * 0.000216636 - 118.34501002237936)::DOUBLE, "
      "(doc_get_int32('latitude', B1.data) * 0.000172998 + "
      "34.011898718557454)::DOUBLE], 'roadnode')";

  dconn
      .Query("SELECT '{\"start\": ' || " + startStr +
             " || ', "
             "\"end\": ' || " +
             endStr +
             " || '}' "
             "FROM B1")
      ->Print();

  t15t16UnpinBuffer(B1->getArrayName());

  cout << "[TASK15]: END" << endl;
}

/**
 *  [Task16] Fine Dust Backtesting ([D, A]=> D).
 *  For a given timestamp Z, hindcast the pm10 values of the schools. (The Z is
 * teh number between min, max of the timestamp dimension.)
 *
 *  Z1 = (Z/TimeInterval)*TimeInterval
 *  Z2 = {(Z+TimeInterval-1)/TimeInterval}*TimeInterval
 *
 *  A: SELECT avg(pm10) FROM FineDust
 *      WHERE timestamp >= Z1  and timestamp <= Z2 group by lat, lon
 *
 *  B: SELECT Map.properties.osm_id AS id, location,
 *      FROM Map, A
 *      WHERE
 *          WithIN(Box(lat, lon, lat+e1, lon+e2), ST_Centroid(Map.geometry))
 *          Map.properties.building = 'school' //Document
 *
 */
void T16(int SF, bool isValidation) {
  long ts = 1600182000 + 10800 * 3.5;
  int Z1 = 3 * SF;
  int Z2 = 4 * SF;

  PolyglotConnection conn(true, "disaster", true);
  auto &dconn = conn.GetDuckdbConnection();
  auto pvEngine = conn.GetPrevisionEngine();

  auto finedust = prevision::OpenArray("finedust");
  auto pm10 = prevision::Project(finedust, {0});

  /* A */
  // filter: timestamp >= Z1 AND timestamp <= Z2
  std::vector<uint32_t> _begin = {(uint32_t)Z1, 0, 0},
                        _end = {(uint32_t)Z2 + 1, 523, 523},
                        _tilesize = {(uint32_t)Z2 + 1 - Z1, 523, 523};
  auto ta1 = prevision::Subarray(pm10, {_begin, _end}, _tilesize);
  // AVG(pm10) AND GROUP BY latitude, longitude
  auto A = prevision::Avg(ta1, {1, 2});
  pvEngine->Execute(*A);

  /* B */
  string nested =
      "SELECT doc_make('{\"site_id\": ' || "
      "doc_get_int32('site_id', data) || ', \"coordinates\": ' || "
      "doc_make_json(doc_get_array('geometry.coordinates', data)) || "
      "'}') AS data "
      "FROM Site "
      "WHERE doc_get_string('properties.type', data) = 'building' "
      "AND "
      "doc_get_string('properties.description', data) = "
      "'school'";

  // manual unnesting three times
  string unnested =
      "SELECT doc_insert(data, unnest(doc_get_list('coordinates', "
      "data, 1)::VPack[])::VPack, 'coordinates')::VPack AS data FROM "
      "(" +
      nested + ") ";
  unnested =
      "SELECT doc_insert(data, unnest(doc_get_list('coordinates', "
      "data, 1)::VPack[])::VPack, 'coordinates')::VPack AS data FROM "
      "(" +
      unnested + ") ";
  unnested =
      "SELECT doc_insert(data, unnest(doc_get_list('coordinates', "
      "data, 1)::VPack[])::VPack, 'coordinates')::VPack AS data FROM "
      "(" +
      unnested + ") ";

  string refined =
      "SELECT doc_make('{\"site_id\": ' || "
      "doc_get_int32('site_id', data) || "
      "', \"longitude\": ' || "
      "FLOOR((((SUM(doc_get_list_double('coordinates', "
      "data)[1]::FLOAT) / "
      "COUNT(*)) + 118.3450100223) / 0.000216636))::INTEGER || "
      "', \"latitude\": ' || "
      "FLOOR((((SUM(doc_get_list_double('coordinates', "
      "data)[2]::FLOAT) / COUNT(*)) - 34.01189870) / "
      "0.000172998))::INTEGER || "
      "'}') AS data "
      "FROM (" +
      unnested +
      ") "
      "GROUP BY doc_get_int32('site_id', data)";
  string final =
      "SELECT doc_get_int32('site_id', data), "
      "doc_get_int32('longitude', data), "
      "doc_get_int32('latitude', data) "
      " FROM (" +
      refined +
      ") "
      "WHERE 0 <= doc_get_int32('longitude', data) AND "
      "doc_get_int32('longitude', data) <= 522 AND "
      "0 <= doc_get_int32('latitude', data) AND "
      "doc_get_int32('latitude', data) <= 522";

  PFpage *page = t15t16GetBuffer(A->getArrayName());

  int cnt = 0;
  auto res = dconn.Query(final);
  auto resChunk = res->Fetch();
  while (resChunk) {
    auto siteIdVec = FlatVector::GetData<int>(resChunk->data[0]);
    auto longitudeVec = FlatVector::GetData<int>(resChunk->data[1]);
    auto latitudeVec = FlatVector::GetData<int>(resChunk->data[2]);
    double *buf = (double *)bf_util_get_pagebuf(page);

    for (int i = 0; i < resChunk->size(); ++i) {
      int site_id = siteIdVec[i];
      int longitude = longitudeVec[i];
      int latitude = latitudeVec[i];

      uint64_t idx = latitude * 523 + longitude;
      if (bf_util_is_cell_null(page, idx)) continue;

      Builder b2;
      b2.add(arangodb::velocypack::Value(ValueType::Object));
      b2.add("site_id", arangodb::velocypack::Value(site_id));
      b2.add("latitude", arangodb::velocypack::Value(latitude));
      b2.add("longitude", arangodb::velocypack::Value(longitude));
      b2.add("pm10_avg", arangodb::velocypack::Value(buf[idx]));
      b2.close();

      auto data = HexDump(b2.slice());
      auto value =
          duckdb::Value::BLOB((const_data_ptr_t)data.data, data.length);

      cout << "site_id=" << site_id << ", latitude=" << latitude
           << ", longitude=" << longitude << ", val=" << buf[idx] << endl;
      ++cnt;
    }

    resChunk = res->Fetch();
  }

  t15t16UnpinBuffer(A->getArrayName());
  cout << "cnt=" << cnt << endl;

  cout << "[TASK16]: DONE" << endl;
}
