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

using namespace std;

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
  static unique_ptr<duckdb::Connection> CreateDuckdbConnection();

 private:
  unique_ptr<prevision::Engine> prevision;
  unique_ptr<duckdb::DuckDB> duckdb;
  unique_ptr<duckdb::Connection> duckdbConnection;

  int numThreads;
  bool withPvBfInit = true;
};