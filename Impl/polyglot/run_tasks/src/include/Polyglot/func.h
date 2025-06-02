#pragma once

#include "duckdb.hpp"

// prevision
#include "interface/functions.h"

using namespace duckdb;
using namespace prevision;

PFpage *pvGetBuffer(string arrName, std::vector<uint64_t> &dcoords,
                    emptytile_template_type_t type);
void pvUnpinBuffer(string arrName, std::vector<uint64_t> &dcoords);

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
std::vector<pair<int, double>> t9GetValues(duckdb::Connection &dconn,
                                           PFpage *page, int id);
void ChunkProcessing(PolyglotConnection &conn,
                     std::shared_ptr<prevision::ArrayQuery> in, int start,
                     int end, int farStart, uint64_t &docTime,
                     uint64_t &arrTime);