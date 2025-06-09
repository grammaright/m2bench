
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
  int tilesize[] = {1500, 9000};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_SPARSE_CSR, domain, tilesize, 2,
                            1, fm, TILESTORE_NOT_NULLABLE);

  // assume that there is only one tile
  PFpage *page = NULL;
  uint64_t lastTileCoords[2];
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_SPARSE_CSR;

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

  auto aChunk = aRes->Fetch();
  while (aChunk) {
    auto drugVec = FlatVector::GetData<int>(aChunk->data[0]);
    auto aeVec = FlatVector::GetData<int>(aChunk->data[1]);

    for (int i = 0; i < aChunk->size(); ++i) {
      // compute tile coordinates and cell coordinates
      uint64_t tileCoords[2] = {(uint64_t)drugVec[i] / tilesize[0],
                                (uint64_t)aeVec[i] / tilesize[1]};
      int cellCoords[2] = {drugVec[i] % (int)tilesize[0],
                           aeVec[i] % (int)tilesize[1]};

      // caching GetBuf() for better performance
      if (page == NULL || !(tileCoords[0] == lastTileCoords[0] &&
                            tileCoords[1] == lastTileCoords[1])) {
        if (page != NULL) {
          BF_TouchBuf(key);
          BF_UnpinBuf(key);
        }

        key.dcoords = tileCoords;
        BF_GetBuf(key, &page);

        lastTileCoords[0] = tileCoords[0];
        lastTileCoords[1] = tileCoords[1];
      }

      // resize if small page
      uint64_t idx = bf_util_pagebuf_get_unfilled_idx(page);
      if (page->max_idx == idx) {
        BF_ResizeBuf(page, idx * 2);
      }

      double *xBuf = (double *)bf_util_get_pagebuf(page);
      uint64_t *indptr = (uint64_t *)bf_util_pagebuf_get_coords(page, 0);
      uint64_t *indices = (uint64_t *)bf_util_pagebuf_get_coords(page, 1);

      int row = cellCoords[0];
      int col = cellCoords[1];

      indptr[row + 1]++;
      indices[idx] = col;
      xBuf[idx] = 1.f;
      ++idx;

      bf_util_pagebuf_set_unfilled_idx(page, idx);
      bf_util_pagebuf_set_unfilled_pagebuf_offset(page, idx * sizeof(double));
    }

    aChunk = aRes->Fetch();
  }

  if (page != NULL) {
    BF_TouchBuf(key);
    BF_UnpinBuf(key);
  }

  // finish touch for idxptr
  uint64_t totalNumTiles =
      ((drugSize + tilesize[0] - 1) / tilesize[0]) *
      ((adverseEffectSize + tilesize[1] - 1) / tilesize[1]);
  // iterate over tiles
  for (uint64_t idx = 0; idx < totalNumTiles; idx++) {
    uint64_t tileCoords[2] = {idx / 10, idx % 10};
    key.dcoords = tileCoords;
    key.emptytile_template = BF_EMPTYTILE_NONE;
    BF_GetBuf(key, &page);
    if (page == NULL) {
      BF_UnpinBuf(key);
      continue;
    }

    uint64_t *indptr = (uint64_t *)bf_util_pagebuf_get_coords(page, 0);
    for (int i = 1; i < 1501; i++) {
      indptr[i] += indptr[i - 1];
    }

    BF_TouchBuf(key);
    BF_UnpinBuf(key);
  }

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