#pragma once

#include "duckdb.hpp"

// prevision
#include "interface/functions.h"

using namespace duckdb;
using namespace prevision;

void t0_sigmoid(Chunk &opnd, Chunk &result);
void t0ConstructX(duckdb::Connection &dconn, int personSize, int tagSize,
                  uint64_t &tblTime, uint64_t &arrTime);
void t0ConstructY(duckdb::Connection &dconn, int personSize,
                  int favoriteBrandId, uint64_t &tblTime, uint64_t &arrTime);
void t2ConstructX(duckdb::Connection &dconn, int customerSize, int productSize,
                  uint64_t &tblTime, uint64_t &arrTime);

void t9_invnorm(Chunk &opnd, Chunk &result);
void t9ConstructD(duckdb::Connection &dconn, int drugSize,
                  int adverseEffectSize, uint64_t &tblTime, uint64_t &arrTime);
PFpage *t9GetBuffer(string arrName);
void t9UnpinBuffer(string arrName);
std::vector<pair<int, double>> t9GetValues(duckdb::Connection &dconn,
                                           PFpage *page, int id);
PFpage *t14GetBuffer(string arrName);
void t14UnpinBuffer(string arrName);
PFpage *t15t16GetBuffer(string arrName);
void t15t16UnpinBuffer(string arrName);
void ChunkProcessing(PolyglotConnection &conn,
                     std::shared_ptr<prevision::ArrayQuery> in, int start,
                     int end, int farStart, uint64_t &docTime,
                     uint64_t &arrTime);