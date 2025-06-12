#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// From prevision
#include "engine/engine.h"

// From duckdb
#include "duckdb.hpp"
#include "spatialindex/SpatialIndex.h"

using namespace std;
using namespace SpatialIndex;

class PolyglotConnection {
 public:
  PolyglotConnection(bool withPvBfInit = true, string dbpath = "",
                     bool readOnly = true);
  ~PolyglotConnection();

  duckdb::Connection& GetDuckdbConnection();
  prevision::Engine* GetPrevisionEngine();

  // THE ONLY WAY TO GET CURRENT ENGINE DIRECTLY
  static PolyglotConnection* GetCurrentEngine();
  static PolyglotConnection* currentEngine;

  // this function is allowed only for st_closest_object_id() now
  static std::unique_ptr<duckdb::Connection> CreateDuckdbConnection();

  std::shared_ptr<ISpatialIndex> getSpatialIdx(string name);

 private:
  std::unique_ptr<prevision::Engine> prevision;
  std::unique_ptr<duckdb::DuckDB> duckdb;
  std::unique_ptr<duckdb::Connection> duckdbConnection;

  unordered_map<string, std::shared_ptr<ISpatialIndex>> cachedSpatialIdx;

  int numThreads;
  bool withPvBfInit = true;
};