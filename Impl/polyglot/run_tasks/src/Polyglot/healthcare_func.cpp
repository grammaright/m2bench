
#include <chrono>
#include <string>
#include <tuple>

#include "Connection/Connection.h"
#include "Polyglot/func.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace duckdb;
using namespace prevision;
using namespace std::chrono;
using namespace std::chrono::_V2;

void t9ConstructD(duckdb::Connection &dconn, int drugSize,
                  int adverseEffectSize, uint64_t &tblTime, uint64_t &arrTime) {
  auto arrStart = system_clock::now();

  const char *arrname = "__D";
  int domain[] = {0, drugSize - 1, 0, adverseEffectSize - 1};
  int tilesize[] = {1600, 400};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_SPARSE_CSR, domain, tilesize, 2,
                            1, fm, TILESTORE_NOT_NULLABLE);

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  // copy data
  auto tblStart = system_clock::now();
  auto aRes = dconn.Query(
      "SELECT drug_d, adverse_effect_d "
      " From Rdrug, Radverse_effect, D2A "
      " Where D2A.drug = Rdrug.drug "
      " and D2A.adverse_effect = Radverse_effect.adverse_effect "
      "ORDER BY drug_d ASC, adverse_effect_d ASC");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  std::vector<posnd_t> bufCoords(BUFFER_SIZE);
  double *bufVal = new double[BUFFER_SIZE];
  size_t idx = 0;

  auto aChunk = aRes->Fetch();
  while (aChunk) {
    auto drugVec = FlatVector::GetData<int>(aChunk->data[0]);
    auto aeVec = FlatVector::GetData<int>(aChunk->data[1]);

    for (int i = 0; i < aChunk->size(); ++i) {
      bufCoords[idx] = {(uint32_t)drugVec[i], (uint32_t)aeVec[i]};
      bufVal[idx] = 1.f;
      ++idx;

      if (idx >= BUFFER_SIZE) {
        BulkWriteCells(arrname, bufCoords, bufVal, sizeof(double), idx);
        idx = 0;
      }
    }

    aChunk = aRes->Fetch();
  }

  if (idx > 0) {
    BulkWriteCells(arrname, bufCoords, bufVal, sizeof(double), idx);
  }

  delete bufVal;
}

std::vector<pair<int, double>> t9GetValues(duckdb::Connection &dconn,
                                           PFpage *page, int id) {
  double *xBuf = (double *)bf_util_get_pagebuf(page);
  uint64_t *indptr = (uint64_t *)bf_util_pagebuf_get_coords(page, 0);
  uint64_t *indices = (uint64_t *)bf_util_pagebuf_get_coords(page, 1);

  int size = indptr[id + 1] - indptr[id];
  std::vector<pair<int, double>> res(size);
  for (int i = 0; i < size; ++i) {
    uint64_t col = indices[indptr[id] + i];
    double val = xBuf[indptr[id] + i];
    res[i] = make_pair((int)col, val);
  }

  return std::move(res);
}