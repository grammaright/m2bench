#include <string>
#include <tuple>

#include "Connection/Connection.h"
#include "boost/fusion/include/pair.hpp"
#include "boost/fusion/support/pair.hpp"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace boost::fusion;
using namespace duckdb;
using namespace prevision;

template <typename T>
bool cmp(const boost::fusion::pair<T, double> &a,
         const boost::fusion::pair<T, double> &b) {
  return a.second > b.second;
};

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

void t0ConstructX(duckdb::Connection &dconn, int personSize, int tagSize) {
  /* construct X */
  const char *arrname = "__X";
  int domain[] = {0, personSize - 1, 0, tagSize - 1};
  int tilesize[] = {personSize, tagSize};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_DENSE, domain, tilesize, 2, 1,
                            fm, TILESTORE_NOT_NULLABLE);
  // assume that there is only one tile
  uint64_t dcoords[] = {0, 0};

  PFpage *page;
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_DENSE;

  BF_GetBuf(key, &page);
  double *xBuf = (double *)bf_util_get_pagebuf(page);

  // copy data
  auto aRes = dconn.Query("SELECT person_id, tag_id FROM TASK_NEW_A_TEMPTABLE");
  auto aChunk = aRes->Fetch();
  while (aChunk) {
    auto personIdVec = FlatVector::GetData<int>(aChunk->data[0]);
    auto tagIdVec = FlatVector::GetData<int>(aChunk->data[1]);
    for (int i = 0; i < aChunk->size(); ++i) {
      uint64_t coord = personIdVec[i] * tagSize + tagIdVec[i];
      xBuf[coord] = 1.f;
    }

    aChunk = aRes->Fetch();
  }

  BF_TouchBuf(key);
  BF_UnpinBuf(key);

  delete key.arrayname;
}

void t0ConstructY(duckdb::Connection &dconn, int personSize,
                  int favoriteBrandId) {
  const char *arrname = "__y";
  int domain[] = {0, personSize - 1, 1, 1};
  int tilesize[] = {personSize, 1};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_DENSE, domain, tilesize, 2, 1,
                            fm, TILESTORE_NOT_NULLABLE);

  uint64_t dcoords[] = {0, 0};
  PFpage *page;
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_DENSE;

  BF_GetBuf(key, &page);
  double *yBuf = (double *)bf_util_get_pagebuf(page);

  // copy data
  auto cRes =
      dconn.Query("SELECT person_id, brand_id FROM TASK_NEW_C_TEMPTABLE");
  auto cChunk = cRes->Fetch();
  while (cChunk) {
    auto personIdVec = FlatVector::GetData<int>(cChunk->data[0]);
    auto valVec = FlatVector::GetData<int>(cChunk->data[1]);
    for (int i = 0; i < cChunk->size(); ++i) {
      yBuf[personIdVec[i]] = valVec[i] == favoriteBrandId ? 1.f : 0.f;
    }

    cChunk = cRes->Fetch();
  }

  BF_TouchBuf(key);
  BF_UnpinBuf(key);

  delete key.arrayname;
}

#define BUFFER 1000

#include <unistd.h>

/**
 *  [Task 0] Building a Logistic Model ([R, D, G, A] => A).
 *  Build a logistic regression model to predict if a user prefers the given
 * brand.
 */
void T0(int brand_id) {
  PolyglotConnection conn(true, "ecommerce", true);
  auto &dconn = conn.GetDuckdbConnection();

  // A
  dconn.Query("DROP TABLE IF EXISTS TASK_NEW_B2_TEMPTABLE");
  dconn.Query(
      "CREATE TEMPORARY TABLE TASK_NEW_A_TEMPTABLE AS "
      "SELECT p.person_id, h.tag_id "
      "FROM Person p "
      "JOIN Interested_in i ON p.person_id = i._from "
      "JOIN Hashtag h ON i._to = h.tag_id");

  // B
  // Get pairs of customer_id and product_id that the customer gives the
  // highest rating score.
  auto res = dconn.Query(
      "SELECT doc_get_string('customer_id', order_.data), "
      "doc_get_string('product_id', review.data) "
      "FROM review, order_ "
      "WHERE doc_get_string('order_id', review.data) = "
      "doc_get_string('order_id', "
      "order_.data) AND "
      "doc_get_int32('rating', review.data) = 5");

  dconn.Query(
      "CREATE TEMPORARY TABLE TASK_NEW_B2_TEMPTABLE_2 ("
      "person_id INT, "
      "brand_id INT)");
  Appender b2t2Appender(dconn, "TASK_NEW_B2_TEMPTABLE_2");

  auto resChunk = res->Fetch();
  while (resChunk) {
    auto customerIdVec = FlatVector::GetData<string_t>(resChunk->data[0]);
    auto productIdVec = FlatVector::GetData<string_t>(resChunk->data[1]);
    for (int i = 0; i < resChunk->size(); i++) {
      auto customerId = customerIdVec[i];
      auto productId = productIdVec[i];

      auto cRes =
          dconn.Query("SELECT person_id FROM Customer WHERE customer_id = '" +
                      customerId.GetString() + "'");
      auto personId = FlatVector::GetData<int>(cRes->Fetch()->data[0])[0];
      auto pRes =
          dconn.Query("SELECT brand_id FROM Product WHERE product_id = '" +
                      productId.GetString() + "'");
      auto brandId = FlatVector::GetData<int>(pRes->Fetch()->data[0])[0];

      b2t2Appender.BeginRow();
      b2t2Appender.Append(personId);
      b2t2Appender.Append(brandId);
      b2t2Appender.EndRow();
    }

    b2t2Appender.Flush();
    resChunk = res->Fetch();
  }

  // Create table for storing aggregated results
  dconn.Query(
      "CREATE TEMP TABLE TASK_NEW_B2_TEMPTABLE ("
      "person_id INT, "
      "brand_id INT, "
      "cnt INT)");

  dconn.Query(
      "INSERT INTO TASK_NEW_B2_TEMPTABLE "
      "SELECT person_id, brand_id, COUNT(*) "
      "FROM TASK_NEW_B2_TEMPTABLE_2 "
      "GROUP BY person_id, brand_id");

  // C: Find favorite brand per customer
  dconn.Query(
      "CREATE TEMPORARY TABLE TASK_NEW_C_TEMPTABLE ("
      "person_id INT, "
      "brand_id INT)");

  // Note that MIN() is used for tie-breaking
  dconn.Query(
      "INSERT INTO TASK_NEW_C_TEMPTABLE "
      "SELECT t1.person_id, MIN(t1.brand_id) "
      "FROM TASK_NEW_B2_TEMPTABLE AS t1, "
      "(SELECT person_id, MAX(cnt) AS max_cnt "
      " FROM TASK_NEW_B2_TEMPTABLE "
      " GROUP BY person_id) AS t2 "
      "WHERE t1.person_id = t2.person_id "
      "AND t1.cnt = t2.max_cnt "
      "GROUP BY t1.person_id");

  // D
  const int givenBrandId = 50;

  // No SF!! the original M2Bench cuts the data to 9949 and 300
  const int personSize = 9949;
  const int tagSize = 300;
  const int bransSize = 64;

  const int numIter = 1;
  const double alpha = 0.0001;

  // // logistic regression of F
  t0ConstructX(dconn, personSize, tagSize);
  t0ConstructY(dconn, personSize, givenBrandId);

  // logistic regression
  auto X = prevision::OpenArray("__X");  // 12
  auto y = prevision::OpenArray("__y");  // 13
  auto w = prevision::Full<double>({(uint32_t)tagSize, 1},
                                   {(uint32_t)tagSize, 1}, 1.0);
  for (int iter = 0; iter < numIter; iter++) {
    auto Xw = prevision::Matmul(X, w);  // 15
    auto sigmoidRes = prevision::Map(Xw, t0_sigmoid, {TILESTORE_FLOAT64},
                                     TILESTORE_DENSE);  // 16
    auto yDiff = prevision::Sub(sigmoidRes, y);         // 17
    auto Xt = prevision::Transpose(X, {1, 0});          // 18
    auto xtDiff = prevision::Matmul(Xt, yDiff);         // 19
    auto rhs = prevision::Mul(xtDiff, alpha);
    w = prevision::Sub(w, rhs);
  }

  auto pvEngine = conn.GetPrevisionEngine();
  pvEngine->Execute(*w);

  cout << "[TASK 0]: DONE" << endl;
  return;
}

void t2ConstructX(duckdb::Connection &dconn, int customerSize,
                  int productSize) {
  /* construct X */
  const char *arrname = "__X";
  int domain[] = {0, customerSize - 1, 0, productSize - 1};
  int tilesize[] = {customerSize, productSize};
  tilestore_datatype_t fm[] = {TILESTORE_FLOAT64};
  storage_util_delete_array(arrname);
  storage_util_create_array(arrname, TILESTORE_DENSE, domain, tilesize, 2, 1,
                            fm, TILESTORE_NOT_NULLABLE);
  // assume that there is only one tile
  uint64_t dcoords[] = {0, 0};

  PFpage *page;
  array_key key;
  key.arrayname = new char[4];
  memcpy(key.arrayname, arrname, 4);
  key.dcoords = dcoords;
  key.dim_len = 2;
  key.emptytile_template = BF_EMPTYTILE_DENSE;

  BF_GetBuf(key, &page);
  double *xBuf = (double *)bf_util_get_pagebuf(page);

  // copy data
  auto aRes = dconn.Query(
      "SELECT customer_id_d as person, product_id_d as product, "
      "rating From Rcustomer, Rproduct, Rating_history Where "
      "Rating_history.customer_id = Rcustomer.customer_id and "
      "Rating_history.product_id = Rproduct.product_id");
  auto aChunk = aRes->Fetch();
  while (aChunk) {
    auto customerIdVec = FlatVector::GetData<int>(aChunk->data[0]);
    auto productIdVec = FlatVector::GetData<int>(aChunk->data[1]);
    auto ratingVec = FlatVector::GetData<int>(aChunk->data[2]);
    for (int i = 0; i < aChunk->size(); ++i) {
      uint64_t coord = customerIdVec[i] * productSize + productIdVec[i];
      xBuf[coord] = (double)ratingVec[i];
    }

    aChunk = aRes->Fetch();
  }

  BF_TouchBuf(key);
  BF_UnpinBuf(key);

  delete key.arrayname;
}

/**
 *  [Task2] Product Recommendation ([D, A]=>A).
 *  Perform product recommendation using "Factorization" based on the past
 * customer ratings.
 *
 *      A: SELECT Order.cid AS cid, Review.pid AS pid, Review.rating AS rating
 *          FROM Review, Order // Document
 *          WHERE Review.oid=Order.oid // Relational
 *
 *      B, C: A.toArray(<val(=A.rating)>[cid,pid]).Factorization // 2 Arrays
 *
 *      D: B(<val: latent_factor>[cid,k]) * C(<val: latent_factor>[k,pid]) //
 * Array
 *      E: SELECT pid FROM D WHERE cid=‘x’ AND val > 4 // Relational
 *
 */
void T2() {
  PolyglotConnection conn(true, "ecommerce", true);
  auto &dconn = conn.GetDuckdbConnection();

  const int SF = 1;
  const int customerSize = 9946 * SF;  // only use customers in A
  const int productSize = 1254 * SF;   // only use products in A
  const int rank = 50;
  const int numIter = 1;

  dconn.Query(
      "CREATE TEMPORARY TABLE Rating_history("
      "customer_id varchar(20),"
      "product_id CHAR(10),"
      "rating int)");
  auto res = dconn.Query(
      "SELECT doc_get_string('customer_id', order_.data), "
      "doc_get_string('product_id', review.data), "
      "AVG(doc_get_int32('rating', review.data))::INTEGER "
      "FROM review, order_ "
      "WHERE doc_get_string('order_id', review.data) = "
      "doc_get_string('order_id', "
      "order_.data) "
      "GROUP BY doc_get_string('customer_id', order_.data), "
      "doc_get_string('product_id', review.data)");
  dconn.Query(
      "CREATE TEMPORARY TABLE Rating_history ("
      "customer_id VARCHAR(20),"
      "product_id CHAR(10),"
      "rating INT)");
  Appender rhAppender(dconn, "Rating_history");
  auto resChunk = res->Fetch();
  while (resChunk) {
    auto customerIdVec = FlatVector::GetData<string_t>(resChunk->data[0]);
    auto productIdVec = FlatVector::GetData<string_t>(resChunk->data[1]);
    auto valVec = FlatVector::GetData<int>(resChunk->data[2]);

    for (int i = 0; i < resChunk->size(); ++i) {
      rhAppender.BeginRow();
      rhAppender.Append(customerIdVec[i]);
      rhAppender.Append(productIdVec[i]);
      rhAppender.Append(valVec[i]);
      rhAppender.EndRow();
    }

    rhAppender.Flush();
    resChunk = res->Fetch();
  }

  // dconn.Query(
  //     "CREATE INDEX Rating_history_idx1 on "
  //     "Rating_history(customer_id)");
  // dconn.Query(
  //     "CREATE INDEX Rating_history_idx2 on "
  //     "Rating_history(product_id)");

  dconn.Query(
      "CREATE TEMPORARY TABLE Rcustomer as "
      "(SELECT t.customer_id, (ROW_NUMBER() OVER () - 1)::INTEGER as "
      "customer_id_d from "
      "(Select distinct(customer_id) as customer_id "
      "from Rating_history order by customer_id) as t )");

  dconn.Query(
      "CREATE TEMPORARY TABLE Rproduct as "
      "(SELECT t.product_id, (ROW_NUMBER() OVER () - 1)::INTEGER as "
      "product_id_d "
      "from (Select distinct(product_id) as product_id "
      "from Rating_history) as t )");

  // dconn.Query(
  //     "CREATE INDEX Rcustomer_idx on "
  //     "Rcustomer(customer_id)");
  // dconn.Query("CREATE INDEX Rproduct_idx on Rproduct(product_id)");

  /* Non-negative matrix factorization */
  t2ConstructX(dconn, customerSize, productSize);

  auto X = prevision::OpenArray("__X");
  auto W = prevision::Full<double>({(uint32_t)customerSize, rank},
                                   {(uint32_t)customerSize, rank}, 1.0);
  auto H = prevision::Full<double>({(uint32_t)rank, productSize},
                                   {(uint32_t)rank, productSize}, 1.0);

  std::vector<uint32_t> tDimOrder = {1, 0};
  for (int iter = 0; iter < numIter; iter++) {
    H = prevision::Mul(
        H,
        prevision::Div(
            prevision::Matmul(prevision::Transpose(W, tDimOrder), X),
            prevision::Matmul(
                prevision::Matmul(prevision::Transpose(W, tDimOrder), W), H)));
    W = prevision::Mul(
        W,
        prevision::Div(prevision::Matmul(X, prevision::Transpose(H, tDimOrder)),
                       prevision::Matmul(prevision::Matmul(W, H),
                                         prevision::Transpose(H, tDimOrder))));
  }

  auto pvEngine = conn.GetPrevisionEngine();
  pvEngine->Execute(*W);

  cout << "[TASK2] DONE" << endl;
}
