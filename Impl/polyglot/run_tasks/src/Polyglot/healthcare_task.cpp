#include "Connection/Connection.h"
#include "Polyglot/func.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace std;
using namespace duckdb;

/**

 Patient
+---------------+---------+------+-----+---------+-------+
| Field         | Type    | Null | Key | Default | Extra |
+---------------+---------+------+-----+---------+-------+
| patient_id    | int     | NO   | PRI | NULL    |       |
| gender        | char(1) | YES  |     | NULL    |       |
| date_of_birth | date    | YES  |     | NULL    |       |
| date_of_death | date    | YES  |     | NULL    |       |
+---------------+---------+------+-----+---------+-------+

 Prescription
+------------+-------------+------+-----+---------+-------+
| Field      | Type        | Null | Key | Default | Extra |
+------------+-------------+------+-----+---------+-------+
| patient_id | int         | YES  | MUL | NULL    |       |
| drug_name  | varchar(20) | YES  |     | NULL    |       |
| startdate  | date        | YES  |     | NULL    |       |
| enddate    | date        | YES  |     | NULL    |       |
| drug_id    | int         | YES  |     | NULL    |       |
+------------+-------------+------+-----+---------+-------+

 Diagnosis
+-----------------------+------+------+-----+---------+-------+
| Field                 | Type | Null | Key | Default | Extra |
+-----------------------+------+------+-----+---------+-------+
| patient_id            | int  | YES  | MUL | NULL    |       |
| snomed_id             | int  | YES  |     | NULL    |       |
| diagnoses_description | text | YES  |     | NULL    |       |
+-----------------------+------+------+-----+---------+-------+
**/

/**
 *  [Task9] Drug similarity (R,D=>A)
 *  Find similar drugs for a given patient X's prescribed drug
 *
 *  A: SELECT drug_id as drug, adverse_effect_list.adverse_effect_name as
 * adverse_effect, 1 as is_adverse_effect FROM Drug UNNEST adverse_effect_list
 * //table
 *
 *  B: A.toArray  -> (<is_adverse_effect>[drug, adverse_effect]) //array
 *  C: Cosine_similarity(B) -> (<similarity coefficient>[drug1, drug2]) //array
 *  D: SELECT * FROM C WHERE drug1 in (Select drug_id from Prescription where
 * patient_id = X)  //array
 *
 */

void T9(int patientId) {
  // const int SF = 1;
  // const int X = 9 * SF;
  const int adverseEffectSize = 82853;
  const int drugSize = 14759;

  PolyglotConnection conn(true, "healthcare", true);
  auto &dconn = conn.GetDuckdbConnection();

  // it gives polyglot advantage
  auto res = dconn.Query(
      "SELECT doc_get_int32('drug_id', data), "
      "doc_get_string('adverse_effect_list.adverse_effect_name', data) "
      "FROM ( "
      "SELECT doc_insert(data, unnest(doc_get_list('adverse_effect_list', "
      "data, 1)::VPack[])::VPack, 'adverse_effect_list')::VPack AS data FROM "
      "Drug) "
      "GROUP BY doc_get_int32('drug_id', data), "
      "doc_get_string('adverse_effect_list.adverse_effect_name', data)");
  dconn.Query(
      "create temporary table D2A ( "
      " drug int, "
      " adverse_effect varchar(100) )");

  Appender d2aAppender(dconn, "D2A");
  auto resChunk = res->Fetch();
  while (resChunk) {
    auto drugIdVector = FlatVector::GetData<int>(resChunk->data[0]);
    auto nameVector = FlatVector::GetData<string_t>(resChunk->data[1]);
    for (int i = 0; i < resChunk->size(); ++i) {
      d2aAppender.BeginRow();

      d2aAppender.Append(drugIdVector[i]);
      d2aAppender.Append(nameVector[i]);

      d2aAppender.EndRow();
    }

    d2aAppender.Flush();
    resChunk = res->Fetch();
  }

  // dconn.Query(
  //     "CREATE TEMPORARY TABLE Rdrug as "
  //     "(SELECT t.drug, (ROW_NUMBER() OVER () -1)::INTEGER as drug_d from "
  //     "(Select distinct(drug) as drug "
  //     "from D2A order by drug) as t )");
  dconn.Query(
      "CREATE TEMPORARY TABLE Rdrug as "
      "(SELECT t.drug, (ROW_NUMBER() OVER () -1)::INTEGER as drug_d from "
      "(Select distinct(drug) as drug "
      "from D2A) as t )");

  // dconn.Query(
  //     "CREATE TEMPORARY TABLE Radverse_effect as "
  //     "(SELECT t.adverse_effect, (ROW_NUMBER() OVER () - 1)::INTEGER as "
  //     "adverse_effect_d from "
  //     "(Select distinct(adverse_effect) as adverse_effect "
  //     "from D2A order by adverse_effect) as t )");
  dconn.Query(
      "CREATE TEMPORARY TABLE Radverse_effect as "
      "(SELECT t.adverse_effect, (ROW_NUMBER() OVER () - 1)::INTEGER as "
      "adverse_effect_d from "
      "(Select distinct(adverse_effect) as adverse_effect "
      "from D2A) as t )");

  // dconn.Query("CREATE INDEX Rdrug on Rdrug(drug)");
  // dconn.Query(
  //     "CREATE INDEX Radverse_effect on Radverse_effect(adverse_effect)");

  t9ConstructD(dconn, drugSize, adverseEffectSize);
  auto D = prevision::OpenArray("__D");

  /* Cosine Similarity */
  std::vector<uint32_t> tDimOrder = {1, 0};
  auto E1 = prevision::Matmul(D, prevision::Transpose(D, tDimOrder));
  auto E2 =
      prevision::Map(E1, t9_invnorm, {TILESTORE_FLOAT64}, TILESTORE_SPARSE_CSR);
  auto E = prevision::Matmul(
      prevision::Transpose(prevision::Matmul(E1, E2), tDimOrder), E2);

  auto pvEngine = conn.GetPrevisionEngine();
  pvEngine->Execute(*E);

  auto fRes = dconn.Query(
      "SELECT DISTINCT Rdrug.drug_d, Rdrug.drug "
      "FROM Prescription, Rdrug "
      "WHERE Rdrug.drug = Prescription.drug_id AND Prescription.patient_id = " +
      to_string(patientId));
  // auto fRes = dconn.Query(
  //     "SELECT DISTINCT Rdrug.drug_d, Rdrug.drug "
  //     "FROM Prescription, Rdrug "
  //     "WHERE Rdrug.drug = Prescription.drug_id AND Prescription.patient_id =
  //     " + to_string(patientId) + " ORDER BY Rdrug.drug");
  auto page = t9GetBuffer(E->getArrayName());
  auto fResChunk = fRes->Fetch();
  size_t resCnt = 0;
  while (fResChunk) {
    auto drugVec = FlatVector::GetData<int>(fResChunk->data[0]);
    auto originalDrugIdVec = FlatVector::GetData<int>(fResChunk->data[1]);
    for (int i = 0; i < fResChunk->size(); ++i) {
      auto id = drugVec[i];
      auto oid = originalDrugIdVec[i];
      auto val = t9GetValues(dconn, page, id);
      resCnt += val.size();  // not to be eliminated

      // For validation
      // for (auto &item : val) {
      //   auto a = dconn.Query("SELECT drug FROM Rdrug WHERE drug_d = " +
      //                        to_string(item.first));
      //   auto ar = a->Fetch();
      //   auto av = FlatVector::GetData<int>(ar->data[0]);
      //   cout << oid << "," << item.first << "," << av[0] << "," <<
      //   item.second
      //        << endl;
      // }
    }

    fResChunk = fRes->Fetch();
  }
  cout << "resCnt: " << resCnt << endl;

  t9UnpinBuffer(E->getArrayName());

  cout << "[TASK9] DONE" << endl;
}
