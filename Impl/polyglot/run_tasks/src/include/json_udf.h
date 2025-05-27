#pragma once

#include <duckdb.hpp>

#include "spatialindex/SpatialIndex.h"

using namespace duckdb;

class RtreeTopOneVisitor : public SpatialIndex::IVisitor {
 public:
  int result = -1;

 public:
  void visitNode(const SpatialIndex::INode& n) override {
    auto res = n.getIdentifier();
    // cerr << "(n)visitNode " << res << endl;
    // result = res;
  }

  void visitData(const SpatialIndex::IData& d) override {
    auto res = d.getIdentifier();
    cerr << "(d)visitNode " << res << endl;
    result = res;
  }

  void visitData(std::vector<const SpatialIndex::IData*>& v) override {
    throw std::runtime_error("Expected not to be called");
    // auto res = v.getIdentifier();
    // cerr << "(v)visitNode " << res << endl;
    // result = res;
  }
};

void docGetVectorized(DataChunk& args, ExpressionState& state,
                      Vector& result);  // return the string as JSON format
void docGetVectorizedInt(DataChunk& args, ExpressionState& state,
                         Vector& result);
void docGetVectorizedUint(DataChunk& args, ExpressionState& state,
                          Vector& result);
void docGetVectorizedDouble(DataChunk& args, ExpressionState& state,
                            Vector& result);
void docGetVectorizedString(DataChunk& args, ExpressionState& state,
                            Vector& result);
void docGetVectorizedList(DataChunk& args, ExpressionState& state,
                          Vector& result);
void docGetVectorizedListDouble(DataChunk& args, ExpressionState& state,
                                Vector& result);
void docGetVectorizedArray(DataChunk& args, ExpressionState& state,
                           Vector& result);
void docMakeVectorized(DataChunk& args, ExpressionState& state, Vector& result);
void docInsertVectorized(DataChunk& args, ExpressionState& state,
                         Vector& result);
void docMergeVectorized(DataChunk& args, ExpressionState& state,
                        Vector& result);
void docMakeJSONVectorized(DataChunk& args, ExpressionState& state,
                           Vector& result);
//  return ID
void docStClosestObjectIdVectorized(DataChunk& args, ExpressionState& state,
                                    Vector& result);
void docStClosestObjectIdCompositeStrVectorized(DataChunk& args,
                                                ExpressionState& state,
                                                Vector& result);
void docStClosestObjectVectorized(DataChunk& args, ExpressionState& state,
                                  Vector& result);
// return JSON string object
void docStClosestObjectCompositeStrVectorized(DataChunk& args,
                                              ExpressionState& state,
                                              Vector& result);
