#include <iostream>
#include <list>

#include "Connection/Connection.h"
#include "spatialindex/SpatialIndex.h"
// #include "utils.hpp"
#include "velocypack/vpack.h"

//
#include "json_udf.h"

using namespace arangodb::velocypack;
using namespace SpatialIndex;

// Utils
list<string> split(const string& s, char delim = '.') {
  std::list<std::string> result;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    result.push_back(item);
  }
  return std::move(result);
}

// for velocypack
Slice getValue(Slice& s, list<string>& keys) {
  // pop front
  auto key = keys.front();
  keys.pop_front();

  auto val = s.get(key);
  if (val.isObject()) {
    return getValue(val, keys);
  }

  return val;
}

void docGetVectorized(DataChunk& args, ExpressionState& state, Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);
    if (v.isNull() || v.isNone()) {
      result_data[i] = StringVector::AddString(result, "null");
    } else {
      result_data[i] = StringVector::AddString(result, v.toJson());
    }
  }
}

void docGetVectorizedInt(DataChunk& args, ExpressionState& state,
                         Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<int>(result);

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);
    if (v.isInteger()) {
      result_data[i] = (int)v.getInt();
    } else {
      FlatVector::SetNull(result, i, true);
    }
  }
}

void docGetVectorizedUint(DataChunk& args, ExpressionState& state,
                          Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<int>(result);

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);
    if (v.isInteger()) {
      result_data[i] = (int)v.getUInt();
    } else {
      FlatVector::SetNull(result, i, true);
    }
  }
}

void docGetVectorizedDouble(DataChunk& args, ExpressionState& state,
                            Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<double>(result);

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);
    if (v.isDouble()) {
      result_data[i] = (double)v.getDouble();
    } else {
      FlatVector::SetNull(result, i, true);
    }
  }
}

void docGetVectorizedString(DataChunk& args, ExpressionState& state,
                            Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);
    if (v.isString()) {
      ValueLength len = v.getStringLength();
      auto str = string(v.getString(len), len);
      // std::cerr << str << std::endl;
      result_data[i] = StringVector::AddString(result, str);
      // inputVec.SetValue(i, str);
    } else {
      FlatVector::SetNull(result, i, true);
    }
  }
}

void recursiveAppend(Slice& v, std::list<duckdb::Value>& elements,
                     int currentDepth, int maxDepth) {
  for (auto it : ArrayIterator(v)) {
    if (it.isArray() && currentDepth < maxDepth) {
      recursiveAppend(it, elements, currentDepth + 1, maxDepth);
    } else {
      auto data = HexDump(it);
      auto value =
          duckdb::Value::BLOB((const_data_ptr_t)data.data, data.length);
      elements.push_back(value);
    }
  }
}

void docGetVectorizedList(DataChunk& args, ExpressionState& state,
                          Vector& result) {
  // FIXME: this makes physical_unnest.cpp to crash when the list is empty
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);
  auto& childVector = ListVector::GetEntry(result);  // child vector of list
  auto listData = ListVector::GetData(result);       // list data

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];
  auto& maxDepthVec = args.data[2];

  // max depth
  auto maxDepth = maxDepthVec.GetValue(0).GetValueUnsafe<int32_t>();

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  uint64_t accumulatedSize = 0;
  for (size_t i = 0; i < args.size(); i++) {
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);

    // std::cerr << val << std::endl;

    if (v.isArray()) {
      int currentDepth = 1;
      // iterate through array and construct a BLOB list
      std::list<duckdb::Value> elements;
      recursiveAppend(v, elements, currentDepth, maxDepth);

      // size up the child vector if necessary
      auto originalSize = ListVector::GetListSize(result);
      if (accumulatedSize + elements.size() > originalSize) {
        auto newSize =
            originalSize == 0 ? STANDARD_VECTOR_SIZE : originalSize * 2;
        childVector.Resize(originalSize, newSize);
        ListVector::SetListSize(result, newSize);
      }

      // push to the list
      int idx = 0;
      for (auto const& it : elements) {
        childVector.SetValue(accumulatedSize + idx++, it);
      }

      // update list data
      listData[i].offset = accumulatedSize;
      listData[i].length = elements.size();

      accumulatedSize += elements.size();
    } else {
      FlatVector::SetNull(result, i, true);
    }
  }

  // static uint64_t cnt = 0;
  // cnt += accumulatedSize;
  // std::cerr << "total accumulated size: " << cnt << std::endl;
  // std::cerr << "ListVector::GetListSize(result): "
  //           << ListVector::GetListSize(result) << std::endl;
  auto originalSize = ListVector::GetListSize(result);
  // childVector.Resize(originalSize, accumulatedSize);
  ListVector::SetListSize(result, accumulatedSize);
}

void docGetVectorizedArray(DataChunk& args, ExpressionState& state,
                           Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    // convert string to velocypack
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);
    if (v.isArray()) {
      auto data = HexDump(v);
      auto value =
          duckdb::Value::BLOB((const_data_ptr_t)data.data, data.length);
      result_data[i] = StringVector::AddStringOrBlob(
          result, (const char*)data.data, data.length);
    } else {
      FlatVector::SetNull(result, i, true);
    }
  }
}

void docGetVectorizedListDouble(DataChunk& args, ExpressionState& state,
                                Vector& result) {
  // FIXME: this makes physical_unnest.cpp to crash when the list is empty
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);
  auto& childVector = ListVector::GetEntry(result);  // child vector of list
  auto listData = ListVector::GetData(result);       // list data

  // get input
  auto& exprVec = args.data[0];
  auto& inputVec = args.data[1];

  // expression should be a constant vector
  auto expression = exprVec.GetValue(0).GetValue<std::string>();
  auto _keys = split(expression);

  // iterate through input
  uint64_t accumulatedSize = 0;
  for (size_t i = 0; i < args.size(); i++) {
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // 2. velocypack
    // get a document from bytes
    Slice s((const uint8_t*)raw);

    // get value
    auto keys = std::list<std::string>(_keys);
    auto v = getValue(s, keys);

    // std::cerr << val << std::endl;

    if (v.isArray()) {
      // iterate through array and construct a BLOB list
      std::list<duckdb::Value> elements;
      for (auto const& it : ArrayIterator(v)) {
        // add
        if (!it.isDouble())
          throw std::runtime_error(
              "doc_get_list_double: an element is not a double");

        auto value = duckdb::Value(it.getDouble());
        elements.push_back(value);
      }

      // size up the child vector if necessary
      auto originalSize = ListVector::GetListSize(result);
      if (accumulatedSize + elements.size() > originalSize) {
        auto newSize =
            originalSize == 0 ? STANDARD_VECTOR_SIZE : originalSize * 2;
        childVector.Resize(originalSize, newSize);
        ListVector::SetListSize(result, newSize);
      }

      // push to the list
      int idx = 0;
      for (auto const& it : elements) {
        childVector.SetValue(accumulatedSize + idx++, it);
      }

      // update list data
      listData[i].offset = accumulatedSize;
      listData[i].length = elements.size();

      accumulatedSize += elements.size();
    } else {
      FlatVector::SetNull(result, i, true);
    }
  }

  // static uint64_t cnt = 0;
  // cnt += accumulatedSize;
  // std::cerr << "total accumulated size: " << cnt << std::endl;
  // std::cerr << "ListVector::GetListSize(result): "
  //           << ListVector::GetListSize(result) << std::endl;
  auto originalSize = ListVector::GetListSize(result);
  // childVector.Resize(originalSize, accumulatedSize);
  ListVector::SetListSize(result, accumulatedSize);
}

void docMakeVectorized(DataChunk& args, ExpressionState& state,
                       Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);

  // get input
  auto& inputVec = args.data[0];

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    // convert string to velocypack
    auto val = inputVec.GetValue(i).GetValue<std::string>();

    // velocypack parser
    Parser parser;

    // tring parsing
    try {
      parser.parse(val);
    } catch (VPackException& e) {
      std::cerr
          << "VPackException: " << e.what()
          << "; below is the value trying to parse. if NULL, the input value "
             "of doc_make() would be a problem such as concatinating with NULL."
          << std::endl;
      std::cerr << val << std::endl;
    }

    // making builder
    auto b = parser.steal();
    auto data = HexDump(b->slice());

    // add
    auto value = duckdb::Value::BLOB((const_data_ptr_t)data.data, data.length);
    result_data[i] = StringVector::AddStringOrBlob(
        result, (const char*)data.data, data.length);
  }
}

void docInsertVectorized(DataChunk& args, ExpressionState& state,
                         Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);

  // get input
  auto& input1Vec = args.data[0];
  auto& input2Vec = args.data[1];

  // get column to be removed from the input1 (unnest target)
  // column expression should be a constant vector
  auto& colVec = args.data[2];
  auto column = colVec.GetValue(0).GetValue<std::string>();

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    // 1. get inputs
    auto val1 = input1Vec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw1 = val1.c_str();

    auto val2 = input2Vec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw2 = val2.c_str();

    // 2. get both velocypack and append rhs to lhs
    Slice s1((const uint8_t*)raw1);
    Slice s2((const uint8_t*)raw2);

    // 3. merge
    // tring appending
    try {
      Builder b;
      b(arangodb::velocypack::Value(ValueType::Object));

      // add all key-value pairs except the column
      for (auto const& it : ObjectIterator(s1)) {
        string key = it.key.copyString();
        if (key == column) {
          continue;
        }

        b.add(key, it.value);
      }

      // add object as a value of the column
      b.add(column, s2);
      b.close();

      // add to duckdb
      Slice res = b.slice();
      auto data = HexDump(res);
      result_data[i] = StringVector::AddStringOrBlob(
          result, (const char*)data.data, data.length);

      // debug
      // {
      //   // turn on pretty printing
      //   Options dumperOptions;
      //   dumperOptions.prettyPrint = true;

      //   // now dump the Slice into an std::string
      //   std::string qqq;
      //   StringSink sink(&qqq);
      //   Dumper dumper(&sink, &dumperOptions);
      //   dumper.dump(res);

      //   // and print it
      //   std::cerr << "Resulting JSON:" << std::endl << qqq << std::endl;
      // }
    } catch (VPackException& e) {
      std::cerr << "VPackException: " << e.what() << std::endl;
      FlatVector::SetNull(result, i, true);
    }
  }
}

void docMergeVectorized(DataChunk& args, ExpressionState& state,
                        Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);

  // get input
  auto& input1Vec = args.data[0];
  auto& input2Vec = args.data[1];

  // iterate through input
  for (size_t i = 0; i < args.size(); i++) {
    // 1. get inputs
    auto val1 = input1Vec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw1 = val1.c_str();

    auto val2 = input2Vec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw2 = val2.c_str();

    // 2. get both velocypack and append rhs to lhs
    Slice s1((const uint8_t*)raw1);
    Slice s2((const uint8_t*)raw2);

    // 3. merge
    // tring appending
    try {
      Builder b;
      b(arangodb::velocypack::Value(ValueType::Object));

      // add all key-value pairs in the first object
      for (auto const& it : ObjectIterator(s1)) {
        string key = it.key.copyString();
        b.add(key, it.value);
      }

      // add all key-value pairs in the second object
      for (auto const& it : ObjectIterator(s2)) {
        string key = it.key.copyString();
        b.add(key, it.value);
      }

      b.close();

      // add to duckdb
      Slice res = b.slice();
      auto data = HexDump(res);
      result_data[i] = StringVector::AddStringOrBlob(
          result, (const char*)data.data, data.length);

      // debug
      // {
      //   // turn on pretty printing
      //   Options dumperOptions;
      //   dumperOptions.prettyPrint = true;

      //   // now dump the Slice into an std::string
      //   std::string qqq;
      //   StringSink sink(&qqq);
      //   Dumper dumper(&sink, &dumperOptions);
      //   dumper.dump(res);

      //   // and print it
      //   std::cerr << "Resulting JSON:" << std::endl << qqq << std::endl;
      // }
    } catch (VPackException& e) {
      std::cerr << "VPackException: " << e.what() << std::endl;
      FlatVector::SetNull(result, i, true);
    }
  }
}

void docMakeJSONVectorized(DataChunk& args, ExpressionState& state,
                           Vector& result) {
  // set result
  auto result_data = FlatVector::GetData<duckdb::string_t>(result);

  // get input
  auto& inputVec = args.data[0];

  // iterate through input
  Options dumperOptions;
  for (size_t i = 0; i < args.size(); i++) {
    // convert velocypack to string
    auto val = inputVec.GetValue(i).GetValueUnsafe<std::string>();
    auto raw = val.c_str();

    // make JSON string
    Slice s((const uint8_t*)raw);

    std::string strRes;
    StringSink sink(&strRes);
    Dumper dumper(&sink, &dumperOptions);
    dumper.dump(s);

    result_data[i] = StringVector::AddString(result, strRes);
  }
}

uint64_t idxOpenTime = 0;
uint64_t idxGetTime = 0;
uint64_t idxQueryTime = 0;

void docStClosestObjectVectorized(DataChunk& args, ExpressionState& state,
                                  Vector& result) {
  // get collection name; it is expected to a string literal (constant)
  auto& colNameVec = args.data[0];
  auto colName = colNameVec.GetValue(0).GetValue<std::string>();

  // get the index
  auto start = std::chrono::high_resolution_clock::now();
  string name = colName + ".sidx";
  auto engine = PolyglotConnection::GetCurrentEngine();
  auto tree = engine->getSpatialIdx(name);
  auto end = std::chrono::high_resolution_clock::now();
  idxOpenTime +=
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  // get input and output
  auto& inputVec = args.data[1];
  auto& inputChildVector = ListVector::GetEntry(inputVec);
  auto inputListData = ListVector::GetData(inputVec);

  auto result_data = FlatVector::GetData<string_t>(result);

  // iterate through input
  auto conn = PolyglotConnection::CreateDuckdbConnection();
  for (size_t i = 0; i < args.size(); i++) {
    // get query point
    double query[2];
    for (size_t j = inputListData[i].offset;
         j < inputListData[i].offset + inputListData[i].length; j++) {
      int idx = j - inputListData[i].offset;
      auto val = inputChildVector.GetValue(j).GetValue<double>();
      query[idx] = val;
    }

    // find the nearest object in the spatial index
    // TODO: assume that -1 indicates not found
    start = std::chrono::high_resolution_clock::now();
    RtreeTopOneVisitor vis;
    SpherePoint p = SpherePoint(query, 2);
    tree->nearestNeighborQuery(1, p, vis);

    end = std::chrono::high_resolution_clock::now();
    idxGetTime +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();

    if (vis.result == -1) {
      FlatVector::SetNull(result, i, true);
      continue;
    }

    start = std::chrono::high_resolution_clock::now();
    auto res = conn->Query("SELECT data FROM " + colName +
                           " WHERE id = " + std::to_string(vis.result));
    end = std::chrono::high_resolution_clock::now();
    idxQueryTime +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();

    if (res->RowCount() > 1) {
      throw std::runtime_error("Multiple objects with the same id");
    }

    auto d = res->Fetch();
    string data = d->GetValue(0, 0).GetValueUnsafe<string>();

    // if data == "", set NULL
    if (data == "") {
      FlatVector::SetNull(result, i, true);
      continue;
    }

    // add to the result
    auto raw = data.c_str();
    Slice s((const uint8_t*)raw);
    auto jsonStr = s.toJson();
    result_data[i] =
        StringVector::AddStringOrBlob(result, jsonStr.data(), jsonStr.size());
  }

  std::cerr << "idxOpenTime: " << idxOpenTime << std::endl;
  std::cerr << "idxGetTime: " << idxGetTime << std::endl;
  std::cerr << "idxQueryTime: " << idxQueryTime << std::endl;
}

void docStClosestObjectIdVectorized(DataChunk& args, ExpressionState& state,
                                    Vector& result) {
  // get collection name; it is expected to a string literal (constant)
  auto& colNameVec = args.data[0];
  auto colName = colNameVec.GetValue(0).GetValue<std::string>();

  // get the index
  auto start = std::chrono::high_resolution_clock::now();
  string name = colName + ".sidx";
  auto engine = PolyglotConnection::GetCurrentEngine();
  auto tree = engine->getSpatialIdx(name);
  auto end = std::chrono::high_resolution_clock::now();
  idxOpenTime +=
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  // get input and output
  auto& inputVec = args.data[1];
  auto& inputChildVector = ListVector::GetEntry(inputVec);
  auto inputListData = ListVector::GetData(inputVec);

  auto result_data = FlatVector::GetData<unsigned int>(result);

  // iterate through input
  auto conn = PolyglotConnection::CreateDuckdbConnection();
  for (size_t i = 0; i < args.size(); i++) {
    // get query point
    double query[2];
    for (size_t j = inputListData[i].offset;
         j < inputListData[i].offset + inputListData[i].length; j++) {
      int idx = j - inputListData[i].offset;
      auto val = inputChildVector.GetValue(j).GetValue<double>();
      query[idx] = val;
    }

    // find the nearest object in the spatial index
    // TODO: assume that -1 indicates not found
    start = std::chrono::high_resolution_clock::now();
    RtreeTopOneVisitor vis;
    SpherePoint p = SpherePoint(query, 2);
    tree->nearestNeighborQuery(1, p, vis);

    end = std::chrono::high_resolution_clock::now();
    idxGetTime +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();

    if (vis.result == -1) {
      FlatVector::SetNull(result, i, true);
      continue;
    }

    // add to the result
    result_data[i] = vis.result;
  }

  std::cerr << "idxOpenTime: " << idxOpenTime << std::endl;
  std::cerr << "idxGetTime: " << idxGetTime << std::endl;
  std::cerr << "idxQueryTime: " << idxQueryTime << std::endl;
}

uint64_t idxOpenTime21 = 0;
uint64_t idxOpenTime22 = 0;
uint64_t idxOpenTime23 = 0;
uint64_t idxGetTime2 = 0;
uint64_t idxQueryTime2 = 0;

void docStClosestObjectIdCompositeStrVectorized(DataChunk& args,
                                                ExpressionState& state,
                                                Vector& result) {
  // get collection name, field name, and (equi) condition; they are expected
  // to a string literal (constant)
  auto& colNameVec = args.data[0];
  auto colName = colNameVec.GetValue(0).GetValue<std::string>();
  auto& fieldNameVec = args.data[1];
  auto fieldName = fieldNameVec.GetValue(0).GetValue<std::string>();
  auto eqcondVec = args.data[3];
  auto eqcond = eqcondVec.GetValue(0).GetValue<std::string>();

  // get the index
  auto start = std::chrono::high_resolution_clock::now();
  string name = colName + "_" + fieldName + ".sidx" + "/" + eqcond;
  auto engine = PolyglotConnection::GetCurrentEngine();
  auto tree = engine->getSpatialIdx(name);
  auto end = std::chrono::high_resolution_clock::now();
  idxOpenTime21 +=
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  // get input and output
  auto& inputVec = args.data[2];
  auto& inputChildVector = ListVector::GetEntry(inputVec);
  auto inputListData = ListVector::GetData(inputVec);

  auto result_data = FlatVector::GetData<unsigned int>(result);

  // iterate through input
  auto conn = PolyglotConnection::CreateDuckdbConnection();
  for (size_t i = 0; i < args.size(); i++) {
    // get query point
    double query[2];
    for (size_t j = inputListData[i].offset;
         j < inputListData[i].offset + inputListData[i].length; j++) {
      int idx = j - inputListData[i].offset;
      auto val = inputChildVector.GetValue(j).GetValue<double>();
      query[idx] = val;
    }

    // find the nearest object in the spatial index
    // TODO: assume that -1 indicates not found
    start = std::chrono::high_resolution_clock::now();
    RtreeTopOneVisitor vis;
    SpherePoint p = SpherePoint(query, 2);
    tree->nearestNeighborQuery(1, p, vis);

    end = std::chrono::high_resolution_clock::now();
    idxGetTime2 +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();

    if (vis.result == -1) {
      FlatVector::SetNull(result, i, true);
      continue;
    }

    result_data[i] = (unsigned int)vis.result;
  }

  std::cerr << "idxOpenTime - 1: " << idxOpenTime21 << std::endl;
  std::cerr << "idxOpenTime - 2: " << idxOpenTime22 << std::endl;
  std::cerr << "idxOpenTime - 3: " << idxOpenTime23 << std::endl;
  std::cerr << "idxGetTime: " << idxGetTime2 << std::endl;
  std::cerr << "idxQueryTime: " << idxQueryTime2 << std::endl;
}

void docStClosestObjectCompositeStrVectorized(DataChunk& args,
                                              ExpressionState& state,
                                              Vector& result) {
  // get collection name, field name, and (equi) condition; they are expected
  // to a string literal (constant)
  auto& colNameVec = args.data[0];
  auto colName = colNameVec.GetValue(0).GetValue<std::string>();
  auto& fieldNameVec = args.data[1];
  auto fieldName = fieldNameVec.GetValue(0).GetValue<std::string>();
  auto eqcondVec = args.data[3];
  auto eqcond = eqcondVec.GetValue(0).GetValue<std::string>();

  // get the index
  auto start = std::chrono::high_resolution_clock::now();
  string name = colName + "_" + fieldName + ".sidx" + "/" + eqcond;
  auto engine = PolyglotConnection::GetCurrentEngine();
  auto tree = engine->getSpatialIdx(name);
  auto end = std::chrono::high_resolution_clock::now();
  idxOpenTime21 +=
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  // get input and output
  auto& inputVec = args.data[2];
  auto& inputChildVector = ListVector::GetEntry(inputVec);
  auto inputListData = ListVector::GetData(inputVec);

  auto result_data = FlatVector::GetData<string_t>(result);

  // iterate through input
  auto conn = PolyglotConnection::CreateDuckdbConnection();
  for (size_t i = 0; i < args.size(); i++) {
    // get query point
    double query[2];
    for (size_t j = inputListData[i].offset;
         j < inputListData[i].offset + inputListData[i].length; j++) {
      int idx = j - inputListData[i].offset;
      auto val = inputChildVector.GetValue(j).GetValue<double>();
      query[idx] = val;
    }

    // find the nearest object in the spatial index
    // TODO: assume that -1 indicates not found
    start = std::chrono::high_resolution_clock::now();
    RtreeTopOneVisitor vis;
    SpherePoint p = SpherePoint(query, 2);
    tree->nearestNeighborQuery(1, p, vis);

    end = std::chrono::high_resolution_clock::now();
    idxGetTime2 +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();

    if (vis.result == -1) {
      FlatVector::SetNull(result, i, true);
      continue;
    }

    start = std::chrono::high_resolution_clock::now();
    auto res = conn->Query("SELECT data FROM " + colName +
                           " WHERE id = " + std::to_string(vis.result));
    end = std::chrono::high_resolution_clock::now();
    idxQueryTime2 +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    if (res->RowCount() > 1) {
      throw std::runtime_error("Multiple objects with the same id");
    }

    auto d = res->Fetch();
    string data = d->GetValue(0, 0).GetValueUnsafe<string>();

    // if data == "", set NULL
    if (data == "") {
      FlatVector::SetNull(result, i, true);
      continue;
    }

    // add to the result
    auto raw = data.c_str();
    Slice s((const uint8_t*)raw);
    auto jsonStr = s.toJson();
    result_data[i] =
        StringVector::AddStringOrBlob(result, jsonStr.data(), jsonStr.size());
  }

  std::cerr << "idxOpenTime - 1: " << idxOpenTime21 << std::endl;
  std::cerr << "idxOpenTime - 2: " << idxOpenTime22 << std::endl;
  std::cerr << "idxOpenTime - 3: " << idxOpenTime23 << std::endl;
  std::cerr << "idxGetTime: " << idxGetTime2 << std::endl;
  std::cerr << "idxQueryTime: " << idxQueryTime2 << std::endl;
}
