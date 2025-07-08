#include <unistd.h>

#include <chrono>
#include <iomanip>
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

/**
 *  [Task 0] Building a Logistic Model ([R, D, G, A] => A).
 *  Build a logistic regression model to predict if a user prefers the given
 * brand.
 */
void T0(int SF, bool isValidation) {
  uint64_t totalTime = 0, tblTime = 0, docTime = 0, arrTime = 0;
  system_clock::time_point tblStart, docStart, arrStart;
  auto totalStart = system_clock::now();

  const int givenBrandId = 50;

  // No SF!! the original M2Bench cuts the data to 9949 and 300
  const int personSize = 9949;
  const int tagSize = 300;
  const int bransSize = 64;

  const int numIter = 1;
  const double alpha = 0.0001;

  PolyglotConnection conn(true, "ecommerce", true);
  auto &dconn = conn.GetDuckdbConnection();

  // A
  tblStart = system_clock::now();
  dconn.Query(
      "CREATE TEMPORARY TABLE TASK_NEW_A_TEMPTABLE AS "
      "SELECT p.person_id, h.tag_id "
      "FROM Person p "
      "JOIN Interested_in i ON p.person_id = i._from "
      "JOIN Hashtag h ON i._to = h.tag_id");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  // B
  // Get pairs of customer_id and product_id that the customer gives the
  // highest rating score.
  tblStart = system_clock::now();
  dconn.Query(
      "CREATE TEMPORARY TABLE TASK_NEW_B2_TEMPTABLE_2 ("
      "person_id INT, "
      "brand_id INT)");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();
  Appender b2t2Appender(dconn, "TASK_NEW_B2_TEMPTABLE_2");

  docStart = system_clock::now();
  dconn.Query(R"(
    CREATE TEMP TABLE DOC_INTERM AS
    SELECT doc_make('{"total_price": ' || doc_get('total_price', order_.data) || ',"order_line": ' || doc_get('order_line', order_.data) || ',"order_.order_id": ' || doc_get('order_.order_id', order_.data) || ',"B1.order_id": ' || doc_get('B1.order_id', B1.data) || ',"customer_id": ' ||     doc_get('customer_id', order_.data) || ',"product_id": ' || doc_get('product_id', B1.data) || ',"rating": ' || doc_get('rating', B1.data) || ',"order_date": ' || doc_get('order_date', order_.data) || ',"feedback": ' || doc_get('feedback', B1.data) || '}')::VPack AS data FROM (SELECT * FROM review WHERE doc_get_int32('rating', review.data) = 5) AS B1 JOIN order_ ON doc_get_string('order_id', B1.data) = doc_get_string('order_id', order_.data)
  )");
  docTime += duration_cast<nanoseconds>(system_clock::now() - docStart).count();

  auto res = dconn.Query(R"(
    SELECT doc_get_string('customer_id', data) AS customer_id, doc_get_string('product_id', data) AS product_id 
    FROM DOC_INTERM
  )");

  // communication cost here
  auto resChunk = res->Fetch();
  while (resChunk) {
    auto customerIdVec = FlatVector::GetData<string_t>(resChunk->data[0]);
    auto productIdVec = FlatVector::GetData<string_t>(resChunk->data[1]);
    for (int i = 0; i < resChunk->size(); i++) {
      auto customerId = customerIdVec[i];
      auto productId = productIdVec[i];
      tblStart = system_clock::now();
      auto cRes =
          dconn.Query("SELECT person_id FROM Customer WHERE customer_id = '" +
                      customerId.GetString() + "'");
      tblTime +=
          duration_cast<nanoseconds>(system_clock::now() - tblStart).count();
      auto personId = FlatVector::GetData<int>(cRes->Fetch()->data[0])[0];

      tblStart = system_clock::now();
      auto pRes =
          dconn.Query("SELECT brand_id FROM Product WHERE product_id = '" +
                      productId.GetString() + "'");
      tblTime +=
          duration_cast<nanoseconds>(system_clock::now() - tblStart).count();
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
  tblStart = system_clock::now();
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
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  // logistic regression of F
  t0ConstructX(dconn, personSize, tagSize, tblTime, arrTime);
  t0ConstructY(dconn, personSize, givenBrandId, tblTime, arrTime);

  // logistic regression
  arrStart = system_clock::now();
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
  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();
  totalTime =
      duration_cast<nanoseconds>(system_clock::now() - totalStart).count();

  if (isValidation) {
    DoValidation([&]() {
      if (SF != 1) {
        cout << "Auto validation is only for SF=1" << endl;

        auto res = ReadCell(w->getArrayName(), {0, 0});
        cout << "{0, 0}=" << res.valDouble << endl;

        res = ReadCell(w->getArrayName(), {1, 0});
        cout << "{1, 0}=" << res.valDouble << endl;

        res = ReadCell(w->getArrayName(), {tagSize - 2, 0});
        cout << "{" << tagSize - 2 << ", 0}=" << res.valDouble << endl;

        res = ReadCell(w->getArrayName(), {tagSize - 1, 0});
        cout << "{" << tagSize - 1 << ", 0}=" << res.valDouble << endl;
      } else {
        auto res = ReadCell(w->getArrayName(), {0, 0});
        cout << "{0, 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 0.8 < 0.001);

        res = ReadCell(w->getArrayName(), {1, 0});
        cout << "{1, 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 0.9861 < 0.001);

        res = ReadCell(w->getArrayName(), {tagSize - 2, 0});
        cout << "{" << tagSize - 2 << ", 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 0.9808 < 0.001);

        res = ReadCell(w->getArrayName(), {tagSize - 1, 0});
        cout << "{" << tagSize - 1 << ", 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 0.9684 < 0.001);
      }
    });
  }

  cout << "[TASK 0]: DONE" << endl;
  cout << "totalTime =" << setw(12) << totalTime << " ns" << endl;
  cout << "tblTime   =" << setw(12) << tblTime << " ns" << endl;
  cout << "docTime   =" << setw(12) << docTime << " ns" << endl;
  cout << "arrTime   =" << setw(12) << arrTime << " ns" << endl;
  return;
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
void T2(int SF, bool isValidation) {
  uint64_t totalTime = 0, tblTime = 0, docTime = 0, arrTime = 0;
  system_clock::time_point tblStart, docStart, arrStart;
  auto totalStart = system_clock::now();

  PolyglotConnection conn(true, "ecommerce", true);
  auto &dconn = conn.GetDuckdbConnection();

  const int customerSize = 9946 * SF;  // only use customers in A
  const int productSize = 1254 * SF;   // only use products in A
  const int rank = 50;
  const int numIter = 1;

  tblStart = system_clock::now();
  dconn.Query(
      "CREATE TEMPORARY TABLE Rating_history("
      "customer_id varchar(20),"
      "product_id CHAR(10),"
      "rating double)");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  docStart = system_clock::now();
  dconn.Query(R"(
    CREATE TEMP TABLE DOC_INTERM AS
    SELECT doc_make('{"customer_id": ' || doc_get('customer_id', data) || ',"product_id": ' || doc_get('product_id', data) || ',"rating": ' || doc_get('rating', data)::DOUBLE || '}')::VPACK AS data FROM (SELECT doc_make('{"total_price": ' || doc_get('total_price', order_.data) || ',"order_line": ' || doc_get('order_line', order_.data) || ',"order_.order_id": ' || doc_get('order_.order_id', order_.data) || ',"customer_id": ' || doc_get('customer_id', order_.data) || ',"order_date": ' || doc_get('order_date', order_.data) || ',"feedback": ' || doc_get('feedback', review.data) || ',"review.order_id": ' || doc_get('review.order_id', review.data) || ',"rating": ' || doc_get('rating', review.data) || ',"product_id": ' || doc_get('product_id', review.data) || '}')::VPack AS data FROM review JOIN order_ ON doc_get_string('order_id', review.data) = doc_get_string('order_id', order_.data)) AS A1
  )");
  docTime += duration_cast<nanoseconds>(system_clock::now() - docStart).count();

  auto res = dconn.Query(R"(
    SELECT doc_get_string('customer_id', data) AS customer_id, doc_get_string('product_id', data) AS product_id, doc_get_double('rating', data) AS rating 
    FROM DOC_INTERM;
  )");

  // Conversion cost
  Appender rhAppender(dconn, "Rating_history");
  auto resChunk = res->Fetch();
  while (resChunk) {
    auto customerIdVec = FlatVector::GetData<string_t>(resChunk->data[0]);
    auto productIdVec = FlatVector::GetData<string_t>(resChunk->data[1]);
    auto valVec = FlatVector::GetData<double>(resChunk->data[2]);

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

  tblStart = system_clock::now();
  if (isValidation) {
    dconn.Query(
        "CREATE TEMPORARY TABLE Rcustomer as "
        "(SELECT t.customer_id, (ROW_NUMBER() OVER () - 1)::INTEGER as "
        "customer_id_d from "
        "(Select distinct(customer_id) as customer_id "
        "from Rating_history order by customer_id) as t )");
  } else {
    dconn.Query(
        "CREATE TEMPORARY TABLE Rcustomer as "
        "(SELECT t.customer_id, (ROW_NUMBER() OVER () - 1)::INTEGER as "
        "customer_id_d from "
        "(Select distinct(customer_id) as customer_id "
        "from Rating_history) as t )");
  }
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  tblStart = system_clock::now();
  dconn.Query(
      "CREATE TEMPORARY TABLE Rproduct as "
      "(SELECT t.product_id, (ROW_NUMBER() OVER () - 1)::INTEGER as "
      "product_id_d "
      "from (Select distinct(product_id) as product_id "
      "from Rating_history) as t )");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  // dconn.Query(
  //     "CREATE INDEX Rcustomer_idx on "
  //     "Rcustomer(customer_id)");
  // dconn.Query("CREATE INDEX Rproduct_idx on Rproduct(product_id)");

  /* Non-negative matrix factorization */
  t2ConstructX(dconn, customerSize, productSize, SF, tblTime, arrTime);

  arrStart = system_clock::now();

  auto X = prevision::OpenArray("__X");
  auto W = prevision::Full<double>({(uint32_t)customerSize, rank},
                                   {(uint32_t)2000, rank}, 1.0);
  auto H = prevision::Full<double>({(uint32_t)rank, (uint32_t)productSize},
                                   {(uint32_t)rank, (uint32_t)300}, 1.0);

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

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();
  totalTime =
      duration_cast<nanoseconds>(system_clock::now() - totalStart).count();

  if (isValidation) {
    DoValidation([&]() {
      if (SF != 1) {
        cout << "Auto validation is only for SF=1" << endl;

        auto res = ReadCell(W->getArrayName(), {0, 0});
        cout << "{0, 0}=" << res.valDouble << endl;

        res = ReadCell(W->getArrayName(), {1, 0});
        cout << "{1, 0}=" << res.valDouble << endl;

        res = ReadCell(W->getArrayName(), {(uint32_t)customerSize - 2, 0});
        cout << "{" << customerSize - 2 << ", 0}=" << res.valDouble << endl;

        res = ReadCell(W->getArrayName(), {(uint32_t)customerSize - 1, 0});
        cout << "{" << customerSize - 1 << ", 0}=" << res.valDouble << endl;
      } else {
        auto res = ReadCell(W->getArrayName(), {0, 0});
        cout << "{0, 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 0.9158290030386842 < 0.001);

        res = ReadCell(W->getArrayName(), {1, 0});
        cout << "{1, 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 1.8480049618942054 < 0.001);

        res = ReadCell(W->getArrayName(), {(uint32_t)customerSize - 2, 0});
        cout << "{" << customerSize - 2 << ", 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 1.0484382216217532 < 0.001);

        res = ReadCell(W->getArrayName(), {(uint32_t)customerSize - 1, 0});
        cout << "{" << customerSize - 1 << ", 0}=" << res.valDouble << endl;
        DoTest(res.valDouble - 1.1463302766557029 < 0.001);
      }
    });
  }

  cout << "[TASK2] DONE" << endl;
  cout << "totalTime =" << setw(12) << totalTime << " ns" << endl;
  cout << "tblTime   =" << setw(12) << tblTime << " ns" << endl;
  cout << "docTime   =" << setw(12) << docTime << " ns" << endl;
  cout << "arrTime   =" << setw(12) << arrTime << " ns" << endl;
}
