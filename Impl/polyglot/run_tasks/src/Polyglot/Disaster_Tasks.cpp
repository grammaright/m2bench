#include <math.h>
#include <time.h>

#include <boost/geometry.hpp>
#include <chrono>

using std::chrono::high_resolution_clock;

#define BUFFER 1000

double distance(double lat, double lon, double lat2, double lon2) {
  return sqrt((lat - lat2) * (lat - lat2) + (lon - lon2) * (lon - lon2));
}

const double pi = acos(-1);
double deg2rad(double deg) { return deg / 180 * pi; }

double distance_haversine(double lat1, double lon1, double lat2, double lon2) {
  return 2 * 6378 *
         asin(sqrt(pow(sin(deg2rad((lat2 - lat1) / 2)), 2) +
                   cos(deg2rad(lat1)) * cos(deg2rad(lat2)) *
                       pow(sin(deg2rad((lon2 - lon1) / 2)), 2)));
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
  //   unique_ptr<ScidbConnection> scidb(
  //       new ScidbConnection(SCIDB_HOST_DISASTER + string(":8080")));
  //   unique_ptr<mongodb_connector> mongodb(new mongodb_connector("Disaster"));
  //   auto mdb = mongodb->db;
  //   auto mapCentroidCollection = mdb["Site_centroid"];

  //   size_t nrows = 0;

  //   // Query A and B
  //   // 8 is magic number for dataset
  //   scidb->exec("store(redimension(apply(window(between(Finedust, " +
  //               to_string(z1) + ", 0, 0, " + to_string(z2) +
  //               ", 522, 522), 0, 0, 2, 2, 2, 2, avg(pm10)), date,
  //               timestamp/8), "
  //               "<pm10_avg: double>[date=0:*:0:?; timestamp=0:*:0:?; "
  //               "latitude=0:*:0:?; longitude=0:*:0:?]), t14t1)");

  //   // Query C
  //   ScidbSchema t2Schema;
  //   t2Schema.dims.push_back(ScidbDim("$n", 0, INT32_MAX, 0, 1000000));
  //   t2Schema.attrs.push_back(ScidbAttr("pm10_avg_max", DOUBLE));
  //   t2Schema.attrs.push_back(ScidbAttr("date", INT64));

  //   ScidbSchema maxSchema;
  //   maxSchema.dims.push_back(ScidbDim("i", 0, INT32_MAX, 0, 1000000));
  //   maxSchema.attrs.push_back(ScidbAttr("pm10_avg", DOUBLE));
  //   maxSchema.attrs.push_back(ScidbAttr("latitude", INT64));
  //   maxSchema.attrs.push_back(ScidbAttr("longitude", INT64));
  //   maxSchema.attrs.push_back(ScidbAttr("timestamp", INT64));

  //   auto t2arr = scidb->download(
  //       "sort(redimension(aggregate(t14t1, max(pm10_avg), date),
  //       <pm10_avg_max: " "double, date: int64>[i=0:*:0:1000]), date)",
  //       t2Schema);
  //   auto t2arrVal = t2arr->readcell();
  //   while (t2arrVal.size() != 0) {
  //     // cout << get<double>(t2arrVal.at(1)) << " " << get<long
  //     // long>(t2arrVal.at(2)) << endl;
  //     long long date = get<long long>(t2arrVal.at(2));
  //     double maxVal = get<double>(t2arrVal.at(1));

  //     // get location of value
  //     // If you have a better solution, please improve it.
  //     // Another way to do this is that replace the query for t2arr to
  //     cross_join
  //     // (t14t1 and max aggregated one), but it is slow a little bit.
  //     auto maxArr = scidb->download(
  //         "sort(redimension(filter(t14t1, abs(pm10_avg - " +
  //         to_string(maxVal) +
  //             ") < 1 and timestamp / 8 = " + to_string(date) +
  //             "), <pm10_avg:double, latitude:int64, longitude: int64,
  //             timestamp: " "int64>[i=0:*:0:1000]), pm10_avg)",
  //         maxSchema);
  //     auto maxArrVal = maxArr->readcell();
  //     if (maxArrVal.size() == 0)
  //       throw std::runtime_error("equality check for floating point is
  //       failed!");

  //     auto closestValue = ST_ClosestObject_Map_building_centroid(
  //         mapCentroidCollection,
  //         34.011898718557454 +
  //             (double)get<long long>(maxArrVal.at(2)) * 0.000172998,
  //         -118.34501002237936 +
  //             (double)get<long long>(maxArrVal.at(3)) * 0.000216636);
  //     // cout << date << " " << get<long long>(maxArrVal.at(4)) << " " <<
  //     // to_string(closestValue) << " " << endl;

  //     t2arrVal = t2arr->readcell();
  //     nrows++;
  //   }

  //   scidb->exec("remove(t14t1)");

  //   cout << "[TASK14]: TOTAL " << nrows << " ROWS ARE REPORTED" << endl;
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
