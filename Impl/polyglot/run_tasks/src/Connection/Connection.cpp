#include "Connection/Connection.h"

#include <duckdb.hpp>

#include "json_udf.h"

using namespace duckdb;

PolyglotConnection* PolyglotConnection::currentEngine = nullptr;

PolyglotConnection::PolyglotConnection(bool withPvBfInit, string dbpath,
                                       bool readOnly) {
  // initialize
  prevision = std::make_unique<prevision::Engine>(withPvBfInit);

  // make duckdb connection (embedded)
  // if read database is used, use read-only mode
  DBConfig config(readOnly);
  config.options.allow_unsigned_extensions = true;
  duckdb = std::make_unique<duckdb::DuckDB>(
      dbpath == "" ? nullptr : dbpath.c_str(), &config);
  duckdbConnection = std::make_unique<duckdb::Connection>(*duckdb);

  // configuration
  numThreads = 1;
  duckdbConnection->Query("SET threads = " + to_string(numThreads));
  duckdbConnection->Query("SET memory_limit = '16GB';")->Print();
  duckdbConnection->Query("SELECT current_setting('memory_limit')")->Print();

  // VPack type
  duckdbConnection->Query("CREATE TYPE VPACK AS BINARY");

  // add UDFs for JSON support
  duckdbConnection->CreateVectorizedFunction(
      "doc_get", {LogicalType::VARCHAR, LogicalType::BLOB},
      LogicalType::VARCHAR, docGetVectorized);
  duckdbConnection->CreateVectorizedFunction(
      "doc_get_int32", {LogicalType::VARCHAR, LogicalType::BLOB},
      LogicalType::INTEGER, docGetVectorizedInt);
  duckdbConnection->CreateVectorizedFunction(
      "doc_get_uint32", {LogicalType::VARCHAR, LogicalType::BLOB},
      LogicalType::UINTEGER, docGetVectorizedUint);
  duckdbConnection->CreateVectorizedFunction(
      "doc_get_double", {LogicalType::VARCHAR, LogicalType::BLOB},
      LogicalType::DOUBLE, docGetVectorizedDouble);
  duckdbConnection->CreateVectorizedFunction(
      "doc_get_string", {LogicalType::VARCHAR, LogicalType::BLOB},
      LogicalType::VARCHAR, docGetVectorizedString);
  // doc_get_list converts an array in VPack to DuckDB list while
  //    doc_get_array just returns the array as a VPack
  duckdbConnection->CreateVectorizedFunction(
      "doc_get_list",
      {LogicalType::VARCHAR, LogicalType::BLOB, LogicalType::INTEGER},
      LogicalType::LIST(LogicalType::BLOB), docGetVectorizedList);
  duckdbConnection->CreateVectorizedFunction(
      "doc_get_array", {LogicalType::VARCHAR, LogicalType::BLOB},
      LogicalType::BLOB, docGetVectorizedArray);
  duckdbConnection->CreateVectorizedFunction(
      "doc_get_list_double", {LogicalType::VARCHAR, LogicalType::BLOB},
      LogicalType::LIST(LogicalType::DOUBLE), docGetVectorizedListDouble);

  duckdbConnection->CreateVectorizedFunction(
      "doc_make", {LogicalType::VARCHAR}, LogicalType::BLOB, docMakeVectorized);
  duckdbConnection->CreateVectorizedFunction(
      "doc_insert",
      {LogicalType::BLOB, LogicalType::BLOB, LogicalType::VARCHAR},
      LogicalType::BLOB, docInsertVectorized);
  duckdbConnection->CreateVectorizedFunction(
      "doc_merge", {LogicalType::BLOB, LogicalType::BLOB}, LogicalType::BLOB,
      docMergeVectorized);
  duckdbConnection->CreateVectorizedFunction(
      "doc_make_json", {LogicalType::BLOB}, LogicalType::VARCHAR,
      docMakeJSONVectorized);
  // duckdbConnection->CreateVectorizedFunction(
  //     "doc_make_array", {LogicalType::BLOB},
  //     LogicalType::LIST(LogicalType::BLOB), docMakeArrayVectorized);

  duckdbConnection->CreateVectorizedFunction(
      "doc_st_closest_object",
      {LogicalType::VARCHAR, LogicalType::LIST(LogicalType::DOUBLE)},
      LogicalType::VARCHAR, docStClosestObjectVectorized);
  duckdbConnection->CreateVectorizedFunction(
      "doc_st_closest_object_id",
      {LogicalType::VARCHAR, LogicalType::LIST(LogicalType::DOUBLE)},
      LogicalType::UINTEGER, docStClosestObjectIdVectorized);
  duckdbConnection->CreateVectorizedFunction(
      "doc_st_closest_object_composite_string",
      {LogicalType::VARCHAR, LogicalType::VARCHAR,
       LogicalType::LIST(LogicalType::DOUBLE), LogicalType::VARCHAR},
      LogicalType::VARCHAR, docStClosestObjectCompositeStrVectorized);
  duckdbConnection->CreateVectorizedFunction(
      "doc_st_closest_object_id_composite_string",
      {LogicalType::VARCHAR, LogicalType::VARCHAR,
       LogicalType::LIST(LogicalType::DOUBLE), LogicalType::VARCHAR},
      LogicalType::UINTEGER, docStClosestObjectIdCompositeStrVectorized);

  // load spatial extension
  duckdbConnection->Query("LOAD 'spatial'");

  // set current engine
  currentEngine = this;
}

PolyglotConnection::~PolyglotConnection() {
  // close connection
  // free
  currentEngine = nullptr;
}

PolyglotConnection* PolyglotConnection::GetCurrentEngine() {
  return PolyglotConnection::currentEngine;
}

duckdb::Connection& PolyglotConnection::GetDuckdbConnection() {
  return *duckdbConnection;
}

prevision::Engine* PolyglotConnection::GetPrevisionEngine() {
  return prevision.get();
}

std::unique_ptr<duckdb::Connection>
PolyglotConnection::CreateDuckdbConnection() {
  return std::make_unique<duckdb::Connection>(*(currentEngine->duckdb));
}

std::shared_ptr<ISpatialIndex> PolyglotConnection::getSpatialIdx(string name) {
  if (cachedSpatialIdx.find(name) != cachedSpatialIdx.end()) {
    return cachedSpatialIdx[name];
  }

  IStorageManager* diskfile =
      SpatialIndex::StorageManager::loadDiskStorageManager(name);
  SpatialIndex::StorageManager::IBuffer* file =
      SpatialIndex::StorageManager::createNewRandomEvictionsBuffer(
          *diskfile, INT32_MAX, false);
  ISpatialIndex* tree = RTree::loadRTree(*file, 1);
  auto sTree = std::shared_ptr<SpatialIndex::ISpatialIndex>(tree);
  cachedSpatialIdx[name] = sTree;
  return sTree;
}
