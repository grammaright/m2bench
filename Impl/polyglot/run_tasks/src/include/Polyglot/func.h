#pragma once

#include "duckdb.hpp"

// prevision
#include "interface/functions.h"

using namespace duckdb;
using namespace prevision;

void t0_sigmoid(Chunk &opnd, Chunk &result);
void t0ConstructX(duckdb::Connection &dconn, int personSize, int tagSize);
void t0ConstructY(duckdb::Connection &dconn, int personSize,
                  int favoriteBrandId);
void t2ConstructX(duckdb::Connection &dconn, int customerSize, int productSize);