#pragma once

#include "duckdb.hpp"

// prevision
#include "interface/functions.h"

using namespace duckdb;
using namespace prevision;

const size_t BUFFER_SIZE = 1000;

PFpage *pvGetBuffer(string arrName, std::vector<uint64_t> &dcoords,
                    emptytile_template_type_t type);
void pvUnpinBuffer(string arrName, std::vector<uint64_t> &dcoords);

void t0_sigmoid(Chunk &opnd, Chunk &result);
void t0ConstructX(duckdb::Connection &dconn, int personSize, int tagSize,
                  uint64_t &tblTime, uint64_t &arrTime);
void t0ConstructY(duckdb::Connection &dconn, int personSize,
                  int favoriteBrandId, uint64_t &tblTime, uint64_t &arrTime);
void t2ConstructX(duckdb::Connection &dconn, int customerSize, int productSize,
                  int SF, uint64_t &tblTime, uint64_t &arrTime);

void t9ConstructD(duckdb::Connection &dconn, int drugSize,
                  int adverseEffectSize, uint64_t &tblTime, uint64_t &arrTime);
std::vector<pair<int, double>> t9GetValues(duckdb::Connection &dconn,
                                           PFpage *page, int id);
string ChunkProcessing(PolyglotConnection &conn,
                       std::shared_ptr<prevision::ArrayQuery> in, int start,
                       int end, int farStart, uint64_t &docTime,
                       uint64_t &arrTime);

inline string MakeRed() { return "\033[1;31m"; }
inline string MakeGreen() { return "\033[1;32m"; }
inline string MakeBlack() { return "\033[0m"; }

inline void DoTest(bool cond) {
  if (cond) return;
  throw runtime_error("Test Failed");
}

inline void DoValidation(function<void(void)> func) {
  try {
    func();
    cout << MakeGreen() << "Test Passed!" << MakeBlack() << endl;
  } catch (const std::exception &e) {
    cout << MakeRed() << "Test Failed!" << MakeBlack() << endl;
  }
}