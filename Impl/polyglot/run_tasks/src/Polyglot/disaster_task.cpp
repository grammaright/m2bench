#include <chrono>
#include <iomanip>

#include "Connection/Connection.h"
#include "Polyglot/func.h"
#include "velocypack/vpack.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace std;
using namespace duckdb;
using namespace arangodb::velocypack;

using namespace std::chrono;
using namespace std::chrono::_V2;

/*
 * [Task 14] Sources of Fine Dust.
 *
 * Analyze fine dust hotspot by date between time Z1 and Z2.
 * Print the nearest building with time of the hotspot.
 * Use window aggregation with a size of 5. (Document, Array) -> Document
 *
 */
void T14(int SF, bool isValidation) {
  uint64_t totalTime = 0, tblTime = 0, docTime = 0, arrTime = 0;
  system_clock::time_point tblStart, docStart, arrStart;
  auto totalStart = system_clock::now();

  const int Z1 = 5 * SF;
  const int Z2 = 10 * SF;

  PolyglotConnection conn(true, "disaster", true);
  auto &dconn = conn.GetDuckdbConnection();

  arrStart = system_clock::now();
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

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  std::vector<std::string> tblVec;
  int curr = (a * 8) % start;
  tblVec.push_back(
      ChunkProcessing(conn, A, 0, curr - 1, start, docTime, arrTime));
  while (curr + 8 <= len) {
    tblVec.push_back(
        ChunkProcessing(conn, A, curr, curr + 7, start, docTime, arrTime));
    curr += 8;
  }
  tblVec.push_back(
      ChunkProcessing(conn, A, curr, len, start, docTime, arrTime));

  docStart = system_clock::now();
  string unionString;
  for (int i = 0; i < tblVec.size(); ++i) {
    string tblName = tblVec[i];
    unionString += R"(
      SELECT data FROM (
        SELECT doc_make('{"date": ' || FLOOR(doc_get('timestamp', data)::INTEGER /8)::INTEGER || ',"timestamp": ' || (doc_get('timestamp', data))::INTEGER || ',"latitude": ' || doc_get('latitude', data) || ',"longitude": ' || doc_get('longitude', data) || ',"pm10_avg": ' || doc_get('pm10_avg', data) || '}')::VPACK AS data FROM )" +
                   tblName + R"(
      )
    )";

    if (i < tblVec.size() - 1) {
      unionString += " UNION ALL ";
    }
  }

  auto finalRes = dconn.Query(R"(
    CREATE TEMP TABLE DOC_RESULT AS
    SELECT data FROM (
      SELECT doc_make('{"date": ' || doc_get('date', data) || ',"timestamp": ' || doc_get('timestamp', data) || ',"site_id": ' || doc_st_closest_object_id_composite_string('Site_centroid','properties.type',[doc_get('longitude', data)::DOUBLE*0.000216636-118.34501002237936,doc_get('latitude', data)::DOUBLE*0.000172998+34.011898718557454],'building')::INTEGER || '}')::VPACK AS data 
      FROM ( )" + unionString +
                              R"(
      )
    ) AS unnamed_12 
    ORDER BY doc_get_int32('date', data)
    )");

  if (isValidation) {
    dconn.Query("SELECT doc_make_json(data) FROM DOC_RESULT")->Print();

    auto res = dconn.Query(
        "SELECT doc_get_int32('date', data), "
        "doc_get_int32('timestamp', data), doc_get_int32('site_id', data) FROM "
        "DOC_RESULT");
    res->Print();

    if (SF != 1) {
      cout << "Auto validation is only for SF=1" << endl;
    } else {
      DoValidation([&]() {
        DoTest(res->RowCount() == 2);

        auto resChunk = res->Fetch();
        auto dateVec = FlatVector::GetData<int32_t>(resChunk->data[0]);
        auto tsVec = FlatVector::GetData<int32_t>(resChunk->data[1]);
        auto idVec = FlatVector::GetData<int32_t>(resChunk->data[2]);

        DoTest(dateVec[0] == 0);
        DoTest(tsVec[0] == 7);
        DoTest(idVec[0] == 11918491);

        DoTest(dateVec[1] == 1);
        DoTest(tsVec[1] == 8);
        DoTest(idVec[1] == 11943192);
      });
    }
  }

  docTime += duration_cast<nanoseconds>(system_clock::now() - docStart).count();
  totalTime =
      duration_cast<nanoseconds>(system_clock::now() - totalStart).count();

  cout << "[TASK14]: END" << endl;
  cout << "totalTime =" << setw(12) << totalTime << " ns" << endl;
  cout << "tblTime   =" << setw(12) << tblTime << " ns" << endl;
  cout << "docTime   =" << setw(12) << docTime << " ns" << endl;
  cout << "arrTime   =" << setw(12) << arrTime << " ns" << endl;
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
  uint64_t totalTime = 0, tblTime = 0, docTime = 0, arrTime = 0;
  system_clock::time_point tblStart, docStart, arrStart;
  auto totalStart = system_clock::now();

  int Z1 = 5 * SF, Z2 = 10 * SF;
  double lon = -118.0614431, lat = 34.068509;

  PolyglotConnection conn(true, "disaster", true);
  auto &dconn = conn.GetDuckdbConnection();
  auto pvEngine = conn.GetPrevisionEngine();

  arrStart = system_clock::now();
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
  uint64_t dimSize[] = {
      (uint64_t)ceil((double)(_end[1] - _begin[1]) / _tilesize[1]),
      (uint64_t)ceil((double)(_end[2] - _begin[2]) / _tilesize[2])};
  uint64_t numTiles = dimSize[0] * dimSize[1];
  std::vector<std::vector<int>> count(
      numTiles, std::vector<int>(_tilesize[1] * _tilesize[2], 0));
  auto A = WindowCustom(
      A2, {5, 5}, std::vector<tilestore_datatype_t>{TILESTORE_FLOAT64},
      [&](Chunk &out) {},  // Nothing to do
      [&](Chunk &in, uint64_t inIdx, Chunk &out, uint64_t outIdx) {
        // accumulate cell value to the output buffer
        uint8_t *inBuf = (uint8_t *)bf_util_get_pagebuf(in.curpage);
        size_t cntOffset = sizeof(float) * in.curpage->max_idx;
        double *outBuf = (double *)bf_util_get_pagebuf(out.curpage);

        // get count vector
        uint64_t _1dc = out.tile_coords[0] * dimSize[1] + out.tile_coords[1];

        // the first 4 bytes are the average value and the next 4 bytes are the
        // count value
        outBuf[outIdx] += *(float *)(inBuf + inIdx * sizeof(float));
        count[_1dc][outIdx] +=
            *(int *)(inBuf + cntOffset + inIdx * sizeof(int));
      },
      [&](Chunk &out) {
        // calculate the average value
        double *outBuf = (double *)bf_util_get_pagebuf(out.curpage);
        uint64_t _1dc = out.tile_coords[0] * dimSize[1] + out.tile_coords[1];
        for (uint64_t outIdx = 0; outIdx < out.curpage->max_idx; outIdx++) {
          if (bf_util_is_cell_null(out.curpage, outIdx)) {
            continue;
          }

          outBuf[outIdx] /= count[_1dc][outIdx];
        }
      });

  auto B1 = Topk(A, prevision::TopkType::MAX, 1);
  pvEngine->Execute(*B1);

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  auto res = ReadValidFirstKCellsFromCoo(B1->getArrayName(), 1);

  docStart = system_clock::now();
  dconn.Query(
      "CREATE TEMP TABLE B1 AS "
      "SELECT doc_make('{\"longitude\": " +
      to_string(res[0].pos[1]) +
      ", "
      "\"latitude\": " +
      to_string(res[0].pos[0]) +
      ", "
      "\"pm10_avg\": " +
      to_string(res[0].valDouble) + "}') AS data");

  if (!isValidation) {
    dconn.Query(R"(
    CREATE TEMP TABLE DOC_RES AS
    SELECT doc_make('{"start": ' || doc_st_closest_object_composite_string('Site_centroid','properties.type',[-118.061443,34.068509],'roadnode')::VARCHAR || ',"end": ' || doc_st_closest_object_composite_string('Site_centroid','properties.type',[(doc_get('longitude', data)::INTEGER*0.000216636-118.34501002237936)::DOUBLE,(doc_get('latitude', data)::INTEGER*0.000172998+34.011898718557454)::DOUBLE],'roadnode')::VARCHAR || '}')::VPACK AS data FROM B1
    )");
  } else {
    dconn
        .Query(R"(
    SELECT '{"start": ' || doc_st_closest_object_composite_string('Site_centroid','properties.type',[-118.061443,34.068509],'roadnode')::VARCHAR || ',"end": ' || doc_st_closest_object_composite_string('Site_centroid','properties.type',[(doc_get('longitude', data)::INTEGER*0.000216636-118.34501002237936)::DOUBLE,(doc_get('latitude', data)::INTEGER*0.000172998+34.011898718557454)::DOUBLE],'roadnode')::VARCHAR || '}' FROM B1
    )")
        ->Print();

    auto res = dconn.Query(R"(
    SELECT  doc_st_closest_object_id_composite_string('Site_centroid','properties.type',[-118.061443,34.068509],'roadnode')::INTEGER, doc_st_closest_object_id_composite_string('Site_centroid','properties.type',[(doc_get('longitude', data)::INTEGER*0.000216636-118.34501002237936)::DOUBLE,(doc_get('latitude', data)::INTEGER*0.000172998+34.011898718557454)::DOUBLE],'roadnode')::INTEGER FROM B1
    )");
    res->Print();

    if (SF != 1) {
      cout << "Auto validation is only for SF=1" << endl;
    } else {
      DoValidation([&]() {
        DoTest(res->RowCount() == 1);

        auto resChunk = res->Fetch();
        auto startVec = FlatVector::GetData<int32_t>(resChunk->data[0]);
        auto endVec = FlatVector::GetData<int32_t>(resChunk->data[1]);

        DoTest(startVec[0] == 100279313);
        DoTest(endVec[0] == 100242938);
      });
    }
  }
  docTime += duration_cast<nanoseconds>(system_clock::now() - docStart).count();

  totalTime =
      duration_cast<nanoseconds>(system_clock::now() - totalStart).count();

  cout << "[TASK15]: END" << endl;
  cout << "totalTime =" << setw(12) << totalTime << " ns" << endl;
  cout << "tblTime   =" << setw(12) << tblTime << " ns" << endl;
  cout << "docTime   =" << setw(12) << docTime << " ns" << endl;
  cout << "arrTime   =" << setw(12) << arrTime << " ns" << endl;
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
  uint64_t totalTime = 0, tblTime = 0, docTime = 0, arrTime = 0;
  system_clock::time_point tblStart, docStart, arrStart;
  auto totalStart = system_clock::now();

  long ts = 1600182000 + 10800 * 3.5;
  int Z1 = 3 * SF;
  int Z2 = 4 * SF;

  PolyglotConnection conn(true, "disaster", true);
  auto &dconn = conn.GetDuckdbConnection();
  auto pvEngine = conn.GetPrevisionEngine();

  arrStart = system_clock::now();
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
  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  /* B */
  docStart = system_clock::now();
  string finalPrepare = R"(
    CREATE TEMP TABLE target_buildings AS
    SELECT * FROM (
      SELECT doc_make('{"site_id": ' || doc_get('site_id', data) || ',"longitude": ' || FLOOR((((SUM((doc_get('coordinates', data)::FLOAT[])[1]))/COUNT(*))+118.3450100223)/0.000216636)::INTEGER || ',"latitude": ' || FLOOR((((SUM((doc_get('coordinates', data)::FLOAT[])[2]))/COUNT(*))-34.01189870)/0.000172998)::INTEGER || '}')::VPACK AS data FROM (SELECT doc_insert(data, unnest(doc_get_list('coordinates', data, 3)::VPack[])::VPack, 'coordinates')::VPack AS data FROM (SELECT doc_make('{"site_id": ' || doc_get('site_id', data) || ',"coordinates": ' || doc_get('geometry.coordinates', data) || '}')::VPACK AS data FROM (SELECT * FROM Site WHERE doc_get_string('properties.type', Site.data) = 'building' AND doc_get_string('properties.description', Site.data) = 'school') AS unnamed_3) AS unnamed_4) AS unnamed_5 GROUP BY doc_get('site_id', data)) AS unnamed_6 WHERE 0 <= doc_get_int32('longitude', data) AND doc_get_int32('longitude', data) <= 522 AND 0 <= doc_get_int32('latitude', data) AND doc_get_int32('latitude', data) <= 522
  )";
  string final = R"(
    SELECT doc_get_int32('site_id', data), doc_get_int32('longitude', data), doc_get_int32('latitude', data)
    FROM target_buildings
  )";
  string createTbl = R"(
    CREATE TEMP TABLE DOC_RESULT (data VPACK)
  )";
  docTime += duration_cast<nanoseconds>(system_clock::now() - docStart).count();

  int cnt = 0;
  if (isValidation) {
    if (SF != 1) {
      cout << "Auto validation is only for SF=1" << endl;
    }

    DoValidation([&]() {
      docStart = system_clock::now();
      dconn.Query(finalPrepare);
      dconn.Query(createTbl);
      docTime +=
          duration_cast<nanoseconds>(system_clock::now() - docStart).count();
      auto res = dconn.Query(final);

      cout << A->getArrayName() << endl;
      cout << res->RowCount() << endl;

      duckdb::Appender appender(dconn, "DOC_RESULT");
      auto resChunk = res->Fetch();
      while (resChunk) {
        auto siteIdVec = FlatVector::GetData<int>(resChunk->data[0]);
        auto longitudeVec = FlatVector::GetData<int>(resChunk->data[1]);
        auto latitudeVec = FlatVector::GetData<int>(resChunk->data[2]);

        for (int i = 0; i < resChunk->size(); ++i) {
          int site_id = siteIdVec[i];
          int longitude = longitudeVec[i];
          int latitude = latitudeVec[i];

          auto res = ReadCell(A->getArrayName(),
                              {(uint32_t)latitude, (uint32_t)longitude});
          // cout << site_id << "," << latitude << "," << longitude << endl;
          if (res.isNull) continue;

          // explicit processing
          Builder b2;
          b2.add(arangodb::velocypack::Value(ValueType::Object));
          b2.add("site_id", arangodb::velocypack::Value(site_id));
          b2.add("latitude", arangodb::velocypack::Value(latitude));
          b2.add("longitude", arangodb::velocypack::Value(longitude));
          b2.add("pm10_avg", arangodb::velocypack::Value(res.valDouble));
          b2.close();

          // making builder
          auto s = b2.slice();
          auto data = HexDump(s);

          // append
          auto value =
              duckdb::Value::BLOB((const_data_ptr_t)data.data, data.length);
          appender.BeginRow();
          appender.Append(value);
          appender.EndRow();

          if (s.length() > 0) {
            ++cnt;
            cout << "site_id=" << site_id << ", latitude=" << latitude
                 << ", longitude=" << longitude << ", val=" << res.valDouble
                 << endl;

            if (SF == 1) {
              // do some of them
              if (site_id == 6821876)
                DoTest(res.valDouble - 26 < 0.001);
              else if (site_id == 11918731)
                DoTest(res.valDouble - 633.6800000000001 < 0.001);
              else if (site_id == 11918698)
                DoTest(res.valDouble - 505.04499999999996 < 0.001);
              else if (site_id == 11692069)
                DoTest(res.valDouble - 23.85 < 0.001);
            }
          }
        }
        appender.Flush();
        resChunk = res->Fetch();
      }

      if (SF == 1) DoTest(cnt == 141);
    });

  } else {
    docStart = system_clock::now();
    dconn.Query(finalPrepare);
    dconn.Query(createTbl);
    docTime +=
        duration_cast<nanoseconds>(system_clock::now() - docStart).count();
    auto res = dconn.Query(final);

    cout << A->getArrayName() << endl;

    duckdb::Appender appender(dconn, "DOC_RESULT");
    auto resChunk = res->Fetch();
    while (resChunk) {
      auto siteIdVec = FlatVector::GetData<int>(resChunk->data[0]);
      auto longitudeVec = FlatVector::GetData<int>(resChunk->data[1]);
      auto latitudeVec = FlatVector::GetData<int>(resChunk->data[2]);

      for (int i = 0; i < resChunk->size(); ++i) {
        int site_id = siteIdVec[i];
        int longitude = longitudeVec[i];
        int latitude = latitudeVec[i];

        auto res = ReadCell(A->getArrayName(),
                            {(uint32_t)latitude, (uint32_t)longitude});
        if (res.isNull) continue;

        // explicit processing
        Builder b2;
        b2.add(arangodb::velocypack::Value(ValueType::Object));
        b2.add("site_id", arangodb::velocypack::Value(site_id));
        b2.add("latitude", arangodb::velocypack::Value(latitude));
        b2.add("longitude", arangodb::velocypack::Value(longitude));
        b2.add("pm10_avg", arangodb::velocypack::Value(res.valDouble));
        b2.close();

        auto s = b2.slice();
        auto data = HexDump(s);

        // append
        auto value =
            duckdb::Value::BLOB((const_data_ptr_t)data.data, data.length);
        appender.BeginRow();
        appender.Append(value);
        appender.EndRow();

        if (s.length() > 0) {
          ++cnt;
        }
      }

      appender.Flush();
      resChunk = res->Fetch();
    }
  }

  cout << "cnt=" << cnt << endl;

  totalTime =
      duration_cast<nanoseconds>(system_clock::now() - totalStart).count();

  cout << "[TASK16]: DONE" << endl;
  cout << "totalTime =" << setw(12) << totalTime << " ns" << endl;
  cout << "tblTime   =" << setw(12) << tblTime << " ns" << endl;
  cout << "docTime   =" << setw(12) << docTime << " ns" << endl;
  cout << "arrTime   =" << setw(12) << arrTime << " ns" << endl;
}
