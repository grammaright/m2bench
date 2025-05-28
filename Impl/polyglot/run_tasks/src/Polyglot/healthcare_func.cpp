
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

void t9_invnorm(Chunk &opnd, Chunk &result) {
  // filter diagonal values and compute 1 / sqrt(val)
  // the matrix size is sqrt(numCells) x sqrt(numCells)

  if (opnd.array_type == TILESTORE_DENSE) {
    // not implemented
    throw std::runtime_error(
        "dense input processing by t9_invnorm is not implemented");
  } else {
    // if it is not diagonal tiles, skip the computation
    if (opnd.tile_coords[0] != opnd.tile_coords[1]) {
      return;
    }

    uint64_t row = opnd.tile_extents[0];

    // input buffers
    double *opnd_bufptr = (double *)bf_util_get_pagebuf(opnd.curpage);
    uint64_t *opnd_idxptr =
        (uint64_t *)bf_util_pagebuf_get_coords(opnd.curpage, 0);
    uint64_t *opnd_indices =
        (uint64_t *)bf_util_pagebuf_get_coords(opnd.curpage, 1);
    uint64_t inputNnz = bf_util_pagebuf_get_len(opnd.curpage) / sizeof(double);

    // resize buffer size; nnz could be up to row
    BF_ResizeBuf(result.curpage, row);

    // output buffers
    double *result_bufptr = (double *)bf_util_get_pagebuf(result.curpage);
    uint64_t *result_idxptr =
        (uint64_t *)bf_util_pagebuf_get_coords(result.curpage, 0);
    uint64_t *result_indices =
        (uint64_t *)bf_util_pagebuf_get_coords(result.curpage, 1);

    // clear the result_idxptr
    memset(result_idxptr, 0, (row + 1) * sizeof(uint64_t));

    // cout << "{" << result.tile_coords[0] << ", " << result.tile_coords[1]
    //      << "}=[";
    // just scanning would be better
    uint64_t resultIdx = 0;
    // iterate over the row idxptr
    for (uint64_t i = 0; i < row; i++) {
      // iterate over the column indices
      for (uint64_t j = opnd_idxptr[i]; j < opnd_idxptr[i + 1]; j++) {
        // check if it is diagonal cell (i.e., i == opnd_indices[j])
        if (opnd_indices[j] != i) continue;

        // fill the result buffer
        result_idxptr[i + 1]++;
        result_bufptr[resultIdx] = 1 / sqrt(opnd_bufptr[j]);
        result_indices[resultIdx] = i;
        resultIdx++;

        // cout << result_bufptr[resultIdx - 1] << "{" << i << ", "
        //      << opnd_indices[j] << "}, ";
      }
    }

    // cout << "]" << endl;

    // update the result_idxptr
    for (uint64_t i = 1; i <= row; i++) {
      result_idxptr[i] += result_idxptr[i - 1];
    }

    // set the unfilled_idx
    result.curpage->unfilled_idx = resultIdx;
    result.curpage->unfilled_pagebuf_offset = resultIdx * sizeof(double);
  }
}

void t9ConstructD(duckdb::Connection &dconn, int drugSize,
                  int adverseEffectSize, uint64_t &tblTime, uint64_t &arrTime) {
  auto arrStart = system_clock::now();

  const char *arrname = "__D";
  int domain[] = {0, drugSize - 1, 0, adverseEffectSize - 1};
  int tilesize[] = {drugSize, adverseEffectSize};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_SPARSE_CSR, domain, tilesize, 2,
                            1, fm, TILESTORE_NOT_NULLABLE);
  // assume that there is only one tile
  uint64_t dcoords[] = {0, 0};

  PFpage *page;
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_SPARSE_CSR;

  BF_GetBuf(key, &page);

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

  //  set CSR page
  arrStart = system_clock::now();
  int rowCount = aRes->RowCount();
  BF_ResizeBuf(page, rowCount);

  double *xBuf = (double *)bf_util_get_pagebuf(page);
  uint64_t *indptr = (uint64_t *)bf_util_pagebuf_get_coords(page, 0);
  uint64_t *indices = (uint64_t *)bf_util_pagebuf_get_coords(page, 1);
  int idx = 0;
  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  auto aChunk = aRes->Fetch();
  while (aChunk) {
    auto drugVec = FlatVector::GetData<int>(aChunk->data[0]);
    auto aeVec = FlatVector::GetData<int>(aChunk->data[1]);

    arrStart = system_clock::now();
    for (int i = 0; i < aChunk->size(); ++i) {
      int row = drugVec[i];
      int col = aeVec[i];

      indptr[row + 1]++;
      indices[idx] = col;
      xBuf[idx] = 1.f;
      ++idx;
    }
    arrTime +=
        duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

    aChunk = aRes->Fetch();
  }

  // finish touch for idxptr
  arrStart = system_clock::now();
  for (int i = 1; i < drugSize + 1; i++) {
    indptr[i] += indptr[i - 1];
  }

  bf_util_pagebuf_set_unfilled_idx(page, rowCount);
  bf_util_pagebuf_set_unfilled_pagebuf_offset(page, rowCount * sizeof(double));

  BF_TouchBuf(key);
  BF_UnpinBuf(key);
  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  delete key.arrayname;
}

PFpage *t9GetBuffer(string arrName) {
  // assume that there is only one tile
  uint64_t dcoords[] = {0, 0};

  PFpage *page;
  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_SPARSE_CSR;

  BF_GetBuf(key, &page);

  delete key.arrayname;

  return page;
}

void t9UnpinBuffer(string arrName) {
  uint64_t dcoords[] = {0, 0};

  array_key key;
  key.arrayname = new char[arrName.size()];
  memcpy(key.arrayname, arrName.c_str(), arrName.size() * sizeof(char));
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_SPARSE_CSR;
  BF_UnpinBuf(key);

  delete key.arrayname;
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