#include "Connection/Connection.h"
#include "Polyglot/func.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace std;
using namespace duckdb;

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

PFpage *t15GetBuffer(string arrName) {
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

void t15UnpinBuffer(string arrName) {
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

/*
 * [Task 14] Sources of Fine Dust.
 *
 * Analyze fine dust hotspot by date between time Z1 and Z2.
 * Print the nearest building with time of the hotspot.
 * Use window aggregation with a size of 5. (Document, Array) -> Document
 *
 */
void T14(int z1, int z2) {
  const int SF = 1;
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
          "SELECT doc_make('{\"date\": ' || doc_get_int32('date', data) || ', "
          "\"timestamp\": ' || doc_get_int32('timestamp', data) || ', "
          "\"site_id\": ' || "
          "doc_st_closest_object_id('Site_centroid', "
          "[(doc_get_int32('longitude', data)::DOUBLE * 0.000216636 - "
          "118.34501002237936), (doc_get_int32('latitude', data)::DOUBLE * "
          "0.000172998 + 34.011898718557454)]) || '}') AS data "
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
void T15(int Z1, int Z2, double lon, double lat) {
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

  PFpage *page = t15GetBuffer(B1->getArrayName());

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
      .Query("SELECT doc_make('{\"start\": ' || " + startStr +
             " || ', "
             "\"end\": ' || " +
             endStr +
             " || '}') AS data "
             "FROM B1")
      ->Print();
  // for validation
  // dconn
  //     .Query("SELECT doc_make_json(doc_make('{\"start\": ' || " + startStr +
  //            " || ', "
  //            "\"end\": ' || " +
  //            endStr +
  //            " || '}')) AS data "
  //            "FROM B1")
  //     ->Print();

  t15UnpinBuffer(B1->getArrayName());

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
void T16(long timestamp) {
  //   auto mongodb = mongodb_connector("Disaster");
  //   auto map = mongodb.db["Site"];

  //   int arrayinfo_time_offset = 1600182000;
  //   int arrayinfo_time_grid_interval = 10800;

  //   double arrayinfo_lat_offset = 34.01189870;
  //   double arrayinfo_lat_grid_interval = 0.000172998;

  //   double arrayinfo_lon_offset = -118.3450100223;
  //   double arrayinfo_lon_grid_interval = 0.000216636;

  //   double lat_max = arrayinfo_lat_grid_interval * 522 +
  //   arrayinfo_lat_offset; double lon_max = arrayinfo_lon_grid_interval * 522
  //   + arrayinfo_lon_offset;

  //   int normZ1 =
  //       (timestamp - arrayinfo_time_offset) / arrayinfo_time_grid_interval;
  //   int normZ2 =
  //       (timestamp - arrayinfo_time_offset + arrayinfo_time_grid_interval -
  //       1) / arrayinfo_time_grid_interval;

  //   cout << "Z1:" << normZ1 << ", Z2:" << normZ2 << endl;

  //   unique_ptr<ScidbConnection> conn(
  //       new ScidbConnection(SCIDB_HOST_DISASTER + string(":8080")));

  //   //  aggregate(between(finedust, 0,null, null,1,null,null), avg(pm10),
  //   //  latitude, longitude)
  //   conn->exec("remove(finedust_temp)");
  //   conn->exec("store(aggregate(between(Finedust," + to_string(normZ1) +
  //              ",null, null," + to_string(normZ2) +
  //              ",null,null), avg(pm10), latitude,
  //              longitude),finedust_temp)");

  //   mongocxx::pipeline p{};
  //   p.match(make_document(kvp("properties.type", "building")));
  //   p.match(make_document(kvp("properties.description", "school")));
  //   p.project(make_document(kvp("building_id", "$_id"),
  //                           kvp("coordinates", "$geometry.coordinates"),
  //                           kvp("_id", 0)));

  //   mongocxx::options::aggregate options;
  //   options.allow_disk_use(true);

  //   int nrow = 0;
  //   ScidbSchema schema;
  //   schema.dims.push_back(ScidbDim("latitude", 0, INT32_MAX, 0, 1000000));
  //   schema.dims.push_back(ScidbDim("longitude", 0, INT32_MAX, 0, 1000000));
  //   schema.attrs.push_back(ScidbAttr("pm10", FLOAT));
  //   auto cursor = map.aggregate(p, options);

  //   int nschool = 0;
  //   for (auto school : cursor) {
  //     nschool++;
  //     auto json = Json::parse(bsoncxx::to_json(school));
  //     auto centroid = STcentroid(json["coordinates"]);
  //     auto school_lat = get<1>(centroid);
  //     auto school_lon = get<0>(centroid);

  //     if (school_lon <= lon_max && school_lon >= arrayinfo_lon_offset &&
  //         school_lat <= lat_max && school_lat >= arrayinfo_lat_offset) {
  //       auto cell1 = high_resolution_clock::now();
  //       int school_lon_norm =
  //           (school_lon - arrayinfo_lon_offset) /
  //           arrayinfo_lon_grid_interval;
  //       int school_lat_norm =
  //           (school_lat - arrayinfo_lat_offset) /
  //           arrayinfo_lat_grid_interval;

  //       string query = "between(finedust_temp," + to_string(school_lat_norm)
  //       +
  //                      "," + to_string(school_lon_norm) + "," +
  //                      to_string(school_lat_norm) + "," +
  //                      to_string(school_lon_norm) + ")";

  //       auto download = conn->download(query, schema);
  //       auto cell = download->readcell();
  //       while (cell.size() != 0) {
  //         double lat = get<int>(cell.at(0));
  //         double lon = get<int>(cell.at(1));
  //         float pm10 = get<float>(cell.at(2));
  //         double cell_lat =
  //             lat * arrayinfo_lat_grid_interval + arrayinfo_lat_offset;
  //         double cell_lon =
  //             lon * arrayinfo_lon_grid_interval + arrayinfo_lon_offset;

  //         nrow++;
  //         //                cout << query << endl;
  //         cell = download->readcell();
  //       }
  //       auto cell2 = high_resolution_clock::now();

  //       //            cout << duration_cast<microseconds>(cell2 -
  //       cell1).count()
  //       //            << endl;
  //     }
  //   }

  //   cout << "[TASK16]: TOTAL " << nrow << " ROWS ARE REPORTED" << endl;
}
