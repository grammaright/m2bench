
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

void t0_sigmoid(Chunk &opnd, Chunk &result) {
  if (opnd.array_type == TILESTORE_DENSE) {
    double *opnd_bufptr = (double *)bf_util_get_pagebuf(opnd.curpage);
    double *result_bufptr = (double *)bf_util_get_pagebuf(result.curpage);
    uint64_t numCells = bf_util_pagebuf_get_len(opnd.curpage) / sizeof(double);

    for (pos1d_t i = 0; i < numCells; i++) {
      result_bufptr[i] = 1 / (1 + exp(-(opnd_bufptr[i])));
    }
  } else {
    if (opnd.curpage->sparse_format == PFPAGE_SPARSE_FORMAT_COO) {
      throw std::runtime_error("Not implemented");
    }

    pos1d_t *opnd_idxptr =
        (pos1d_t *)bf_util_pagebuf_get_coords(opnd.curpage, 0);
    pos1d_t *opnd_indices =
        (pos1d_t *)bf_util_pagebuf_get_coords(opnd.curpage, 1);
    double *opnd_bufptr = (double *)bf_util_get_pagebuf(opnd.curpage);
    double *res_bufptr = (double *)bf_util_get_pagebuf(result.curpage);
    uint64_t numCells =
        bf_util_pagebuf_get_len(result.curpage) / sizeof(double);
    uint64_t nrows = opnd.tile_extents[0];

    uint64_t ncols = numCells / nrows;

    // zero value
    double _default = 1.0 / (1.0 + exp(0));
    for (uint64_t i = 0; i < numCells; i++) res_bufptr[i] = _default;

    for (uint64_t i = 0; i < nrows; i++)  // for each row in one tile
    {
      uint64_t opnd_pos = opnd_idxptr[i];
      uint64_t opnd_end = opnd_idxptr[i + 1];

      while (opnd_pos < opnd_end) {
        uint64_t opnd_col = opnd_indices[opnd_pos];

        double res_value =
            1.0 / (double)(1.0 + exp((double)-opnd_bufptr[opnd_pos]));
        uint64_t row_idx = i;
        uint64_t col_idx = opnd_indices[opnd_pos];
        res_bufptr[row_idx * ncols + col_idx] = res_value;

        opnd_pos++;
      }
    }
  }
}

void t0ConstructX(duckdb::Connection &dconn, int personSize, int tagSize,
                  uint64_t &tblTime, uint64_t &arrTime) {
  auto arrStart = system_clock::now();

  /* construct X */
  const char *arrname = "__X";
  int domain[] = {0, personSize - 1, 0, tagSize - 1};
  int tilesize[] = {personSize, tagSize};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_DENSE, domain, tilesize, 2, 1,
                            fm, TILESTORE_NOT_NULLABLE);

  // TODO: multiple tiles
  // assume that there is only one tile
  PFpage *page = NULL;
  uint64_t lastTileCoords[2];
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_DENSE;

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  // copy data
  auto tblStart = system_clock::now();
  auto aRes = dconn.Query("SELECT person_id, tag_id FROM TASK_NEW_A_TEMPTABLE");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  auto aChunk = aRes->Fetch();
  while (aChunk) {
    auto personIdVec = FlatVector::GetData<int>(aChunk->data[0]);
    auto tagIdVec = FlatVector::GetData<int>(aChunk->data[1]);

    for (int i = 0; i < aChunk->size(); ++i) {
      if (personIdVec[i] < 0 || personIdVec[i] >= personSize ||
          tagIdVec[i] < 0 || tagIdVec[i] >= tagSize) {
        // the matrix size is fixed even though varying scaling factor
        continue;
      }

      // compute tile coordinates and cell coordinates
      uint64_t tileCoords[2] = {(uint64_t)personIdVec[i] / tilesize[0],
                                (uint64_t)tagIdVec[i] / tilesize[1]};
      uint64_t cellCoords[2] = {(uint64_t)personIdVec[i] % tilesize[0],
                                (uint64_t)tagIdVec[i] % tilesize[1]};

      // caching GetBuf() for better performance
      if (page == NULL || !(tileCoords[0] != lastTileCoords[0] &&
                            tileCoords[1] != lastTileCoords[1])) {
        if (page != NULL) {
          BF_TouchBuf(key);
          BF_UnpinBuf(key);
        }

        key.dcoords = tileCoords;
        BF_GetBuf(key, &page);
      }

      double *xBuf = (double *)bf_util_get_pagebuf(page);
      uint64_t coord = cellCoords[0] * tagSize + cellCoords[1];
      xBuf[coord] = 1.f;
    }

    aChunk = aRes->Fetch();
  }

  if (page != NULL) {
    BF_TouchBuf(key);
    BF_UnpinBuf(key);
  }

  delete key.arrayname;
}

void t0ConstructY(duckdb::Connection &dconn, int personSize,
                  int favoriteBrandId, uint64_t &tblTime, uint64_t &arrTime) {
  auto arrStart = system_clock::now();

  const char *arrname = "__y";
  int domain[] = {0, personSize - 1, 1, 1};
  int tilesize[] = {personSize, 1};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_DENSE, domain, tilesize, 2, 1,
                            fm, TILESTORE_NOT_NULLABLE);

  // TODO: multiple tiles
  // assume that there is only one tile
  PFpage *page = NULL;
  uint64_t lastTileCoords[2];
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_DENSE;

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  // copy data
  auto tblStart = system_clock::now();
  auto cRes =
      dconn.Query("SELECT person_id, brand_id FROM TASK_NEW_C_TEMPTABLE");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  auto cChunk = cRes->Fetch();
  while (cChunk) {
    auto personIdVec = FlatVector::GetData<int>(cChunk->data[0]);
    auto valVec = FlatVector::GetData<int>(cChunk->data[1]);

    for (int i = 0; i < cChunk->size(); ++i) {
      if (personIdVec[i] < 0 || personIdVec[i] >= personSize) {
        // the matrix size is fixed even though varying scaling factor
        continue;
      }

      // compute tile coordinates and cell coordinates
      uint64_t tileCoords[2] = {(uint64_t)personIdVec[i] / tilesize[0], 0};
      uint64_t cellCoords[2] = {(uint64_t)personIdVec[i] % tilesize[0], 0};

      // caching GetBuf() for better performance
      if (page == NULL || !(tileCoords[0] != lastTileCoords[0] &&
                            tileCoords[1] != lastTileCoords[1])) {
        if (page != NULL) {
          BF_TouchBuf(key);
          BF_UnpinBuf(key);
        }

        key.dcoords = tileCoords;
        BF_GetBuf(key, &page);
      }

      double *yBuf = (double *)bf_util_get_pagebuf(page);
      yBuf[cellCoords[0]] = valVec[i] == favoriteBrandId ? 1.f : 0.f;
    }

    cChunk = cRes->Fetch();
  }

  if (page != NULL) {
    BF_TouchBuf(key);
    BF_UnpinBuf(key);
  }

  delete key.arrayname;
}

void t2ConstructX(duckdb::Connection &dconn, int customerSize, int productSize,
                  uint64_t &tblTime, uint64_t &arrTime) {
  auto arrStart = system_clock::now();
  /* construct X */
  const char *arrname = "__X";
  int domain[] = {0, customerSize - 1, 0, productSize - 1};
  int tilesize[] = {customerSize, productSize};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_DENSE, domain, tilesize, 2, 1,
                            fm, TILESTORE_NOT_NULLABLE);

  // TODO: multiple tiles
  // assume that there is only one tile
  PFpage *page = NULL;
  uint64_t lastTileCoords[2];
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_DENSE;

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  // copy data
  auto tblStart = system_clock::now();
  auto aRes = dconn.Query(R"(
    SELECT customer_id_d as person, product_id_d as product, AVG(rating)::DOUBLE 
    FROM Rcustomer, Rproduct, Rating_history
    WHERE Rating_history.customer_id = Rcustomer.customer_id AND
    Rating_history.product_id = Rproduct.product_id
    GROUP BY customer_id_d, product_id_d
  )");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  auto aChunk = aRes->Fetch();
  while (aChunk) {
    auto customerIdVec = FlatVector::GetData<int>(aChunk->data[0]);
    auto productIdVec = FlatVector::GetData<int>(aChunk->data[1]);
    auto ratingVec = FlatVector::GetData<double>(aChunk->data[2]);

    for (int i = 0; i < aChunk->size(); ++i) {
      // compute tile coordinates and cell coordinates
      uint64_t tileCoords[2] = {(uint64_t)customerIdVec[i] / tilesize[0],
                                (uint64_t)productIdVec[i] / tilesize[1]};
      uint64_t cellCoords[2] = {(uint64_t)customerIdVec[i] % tilesize[0],
                                (uint64_t)productIdVec[i] % tilesize[1]};

      // caching GetBuf() for better performance
      if (page == NULL || !(tileCoords[0] != lastTileCoords[0] &&
                            tileCoords[1] != lastTileCoords[1])) {
        if (page != NULL) {
          BF_TouchBuf(key);
          BF_UnpinBuf(key);
        }

        key.dcoords = tileCoords;
        BF_GetBuf(key, &page);
      }

      double *xBuf = (double *)bf_util_get_pagebuf(page);
      uint64_t coord = cellCoords[0] * productSize + cellCoords[1];
      xBuf[coord] = ratingVec[i];
    }

    aChunk = aRes->Fetch();
  }

  if (page != NULL) {
    BF_TouchBuf(key);
    BF_UnpinBuf(key);
  }

  delete key.arrayname;
}
