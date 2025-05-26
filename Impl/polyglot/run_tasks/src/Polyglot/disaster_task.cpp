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

  // delete key.arrayname;

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

  // delete key.arrayname;
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
          "SELECT doc_make('{\"date\": ' || doc_get_int32('date', data) || ',
          "
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
void T15(int z1, int z2, double lon, double lat) {
  //   size_t nrows = 0;

  //   unique_ptr<ScidbConnection> scidb(
  //       new ScidbConnection(SCIDB_HOST_DISASTER + string(":8080")));
  //   shared_ptr<neo4j_connector> neo4j(new neo4j_connector());
  //   unique_ptr<mongodb_connector> mongodb(new mongodb_connector("Disaster"));
  //   auto mdb = mongodb->db;
  //   auto mapCollection = mdb["Site"];

  //   // Query A and B
  //   int max_win_oneside = z2 - z1;

  //   ScidbSchema hotspotSchema;
  //   hotspotSchema.dims.push_back(ScidbDim("i", 0, INT32_MAX, 0, 1000000));
  //   hotspotSchema.attrs.push_back(ScidbAttr("pm10_avg", DOUBLE));
  //   hotspotSchema.attrs.push_back(ScidbAttr("latitude", INT64));
  //   hotspotSchema.attrs.push_back(ScidbAttr("longitude", INT64));

  //   // cout << to_string(z1) << "," << to_string(z2) << "," <<
  //   // to_string(max_win_oneside) << endl;
  //   auto hotspot = scidb->download(
  //       "limit(sort(redimension(apply(window(aggregate(between(Finedust, " +
  //           to_string(z1) + ", 0, 0, " + to_string(z2) +
  //           ", 522, 522), sum(pm10), count(pm10), latitude, longitude), 2, 2,
  //           2, " "2, sum(pm10_sum), sum(pm10_count)), pm10_avg, pm10_sum_sum
  //           / " "pm10_count_sum), <pm10_avg:double, latitude:int64,
  //           longitude: " "int64>[i=0:*:0:100000000]), pm10_avg desc), 1)",
  //       hotspotSchema);
  //   auto hotspotCells = hotspot->readcell();
  //   auto current = ST_ClosestObject_RoadNode(mapCollection, neo4j, lat,
  //                                            lon);  // Current coord
  //   int target = -1;
  //   while (hotspotCells.size() != 0) {
  //     auto targetLat = 34.011898718557454 +
  //                      (double)get<long long>(hotspotCells.at(2)) *
  //                      0.000172998;
  //     auto targetLon = -118.34501002237936 +
  //                      (double)get<long long>(hotspotCells.at(3)) *
  //                      0.000216636;
  //     // cout << targetLat << targetLon << (double)
  //     // get<double>(hotspotCells.at(1)) << endl;

  //     target =
  //         ST_ClosestObject_RoadNode(mapCollection, neo4j, targetLat,
  //         targetLon);
  //     break;
  //   }

  //   std::string neoq =
  //       "MATCH (source:Roadnode {roadnode_id: " + to_string(current) +
  //       "}), (target:Roadnode {roadnode_id: " + to_string(target) +
  //       "}) \
//         CALL gds.shortestPath.dijkstra.stream('road_network', { \
//             sourceNode: source, \
//             targetNode: target, \
//             relationshipWeightProperty: 'distance' \
//         }) \
//         YIELD index, sourceNode, targetNode, totalCost, nodeIds, costs,
  //         path \
//         RETURN \
//             index, \
//             sourceNode, \
//             targetNode, \
//             totalCost, \
//             costs, \
//             nodes(path) as path \
//         LIMIT 1";

  //   // std::cout << neoq << std::endl;

  //   auto neor = neo4j_run(neo4j->conn, neoq.c_str(), neo4j_null);
  //   auto neof = neo4j_fetch_next(neor);
  //   if (neof == NULL)
  //     std::cout << "No result!!!" << std::endl;
  //   else {
  //     auto nSource = neo4j_int_value(neo4j_result_field(neof, 1));
  //     auto nTarget = neo4j_int_value(neo4j_result_field(neof, 2));
  //     auto nTotalCost = neo4j_float_value(neo4j_result_field(neof, 3));
  //     char buf[65536];
  //     std::string nPath =
  //         neo4j_tostring(neo4j_result_field(neof, 5), buf, sizeof(buf));

  //     // cout << to_string(nSource) << ", " << to_string(nTarget) << ", " <<
  //     // to_string(nTotalCost) << ", " << nPath << endl;
  //     nrows++;
  //   }

  //   cout << "[TASK15]: TOTAL " << nrows << " ROWS ARE REPORTED" << endl;
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
