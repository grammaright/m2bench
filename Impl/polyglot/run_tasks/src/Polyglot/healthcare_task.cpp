#include <chrono>
#include <iomanip>

#include "Connection/Connection.h"
#include "Polyglot/func.h"

// prevision
#include "interface/functions.h"
#include "type/type.h"

using namespace std;
using namespace duckdb;
using namespace std::chrono;
using namespace std::chrono::_V2;

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

void T9(int SF, bool isValidation) {
  uint64_t totalTime = 0, tblTime = 0, docTime = 0, arrTime = 0;
  system_clock::time_point tblStart, docStart, arrStart;
  auto totalStart = system_clock::now();

  const int patientId = 9 * SF;
  const int adverseEffectSize = 82853;
  const int drugSize = 14759;

  PolyglotConnection conn(true, "healthcare", true);
  auto &dconn = conn.GetDuckdbConnection();

  tblStart = system_clock::now();
  dconn.Query(
      "create temporary table D2A ( "
      " drug int, "
      " adverse_effect varchar(100) )");
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  docStart = system_clock::now();
  dconn.Query(R"(
    CREATE TEMP TABLE DOC_INTERM AS
    SELECT doc_make('{"adverse_effect": ' || doc_get('adverse_effect_list.adverse_effect_name', data) || ',"drug_id": ' || doc_get('drug_id', data) || '}')::VPACK AS data 
    FROM (
      SELECT doc_insert(data, unnest(doc_get_list('adverse_effect_list', data, 1)::VPack[])::VPack, 'adverse_effect_list')::VPack AS data FROM Drug
    ) AS unnamed_2 
    GROUP BY doc_get('adverse_effect_list.adverse_effect_name', data), doc_get('drug_id', data)
    )");
  docTime += duration_cast<nanoseconds>(system_clock::now() - docStart).count();

  auto res = dconn.Query(R"(
    SELECT doc_get_int32('drug_id', data) AS drug_id, doc_get_string('adverse_effect', data) AS adverse_effect_name 
    FROM DOC_INTERM
  )");

  // Conversion cost
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

  tblStart = system_clock::now();
  if (isValidation) {
    dconn.Query(
        "CREATE TEMPORARY TABLE Rdrug as "
        "(SELECT t.drug, (ROW_NUMBER() OVER () -1)::INTEGER as drug_d from "
        "(Select distinct(drug) as drug "
        "from D2A order by drug) as t )");
    dconn.Query(
        "CREATE TEMPORARY TABLE Radverse_effect as "
        "(SELECT t.adverse_effect, (ROW_NUMBER() OVER () - 1)::INTEGER as "
        "adverse_effect_d from "
        "(Select distinct(adverse_effect) as adverse_effect "
        "from D2A order by adverse_effect) as t )");
  } else {
    dconn.Query(
        "CREATE TEMPORARY TABLE Rdrug as "
        "(SELECT t.drug, (ROW_NUMBER() OVER () -1)::INTEGER as drug_d from "
        "(Select distinct(drug) as drug "
        "from D2A) as t )");
    dconn.Query(
        "CREATE TEMPORARY TABLE Radverse_effect as "
        "(SELECT t.adverse_effect, (ROW_NUMBER() OVER () - 1)::INTEGER as "
        "adverse_effect_d from "
        "(Select distinct(adverse_effect) as adverse_effect "
        "from D2A) as t )");
  }
  tblTime += duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

  // dconn.Query("CREATE INDEX Rdrug on Rdrug(drug)");
  // dconn.Query(
  //     "CREATE INDEX Radverse_effect on Radverse_effect(adverse_effect)");

  /* Cosine Similarity */
  t9ConstructD(dconn, drugSize, adverseEffectSize, tblTime, arrTime);

  arrStart = system_clock::now();
  auto D = prevision::OpenArray("__D");

  std::vector<uint32_t> tDimOrder = {1, 0};
  auto E1 = prevision::Matmul(D, prevision::Transpose(D, tDimOrder));
  auto E2 =
      prevision::Map(E1, t9_invnorm, {TILESTORE_FLOAT64}, TILESTORE_SPARSE_CSR);
  auto E = prevision::Matmul(
      prevision::Transpose(prevision::Matmul(E1, E2), tDimOrder), E2);

  auto pvEngine = conn.GetPrevisionEngine();
  pvEngine->Execute(*E);

  arrTime += duration_cast<nanoseconds>(system_clock::now() - arrStart).count();

  if (isValidation) {
    tblStart = system_clock::now();
    auto fRes = dconn.Query(
        "SELECT DISTINCT Rdrug.drug_d, Rdrug.drug "
        "FROM Prescription, Rdrug "
        "WHERE Rdrug.drug = Prescription.drug_id AND "
        "Prescription.patient_id "
        "= " +
        to_string(patientId) + " ORDER BY Rdrug.drug");
    tblTime +=
        duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

    auto fResChunk = fRes->Fetch();
    size_t resCnt = 0;
    while (fResChunk) {
      auto drugVec = FlatVector::GetData<int>(fResChunk->data[0]);
      auto originalDrugIdVec = FlatVector::GetData<int>(fResChunk->data[1]);
      for (int i = 0; i < fResChunk->size(); ++i) {
        auto id = drugVec[i];
        auto oid = originalDrugIdVec[i];

        uint64_t colTileSize = drugSize;
        uint64_t colNumTiles = (drugSize + drugSize - 1) / colTileSize;
        for (uint64_t colTileIdx = 0; colTileIdx < colNumTiles; colTileIdx++) {
          std::vector<uint64_t> dcoords = {0, colTileIdx};
          auto page =
              pvGetBuffer(E->getArrayName(), dcoords, BF_EMPTYTILE_NONE);

          auto val = t9GetValues(dconn, page, id);
          resCnt += val.size();  // not to be eliminated
          for (auto &item : val) {
            auto a = dconn.Query("SELECT drug FROM Rdrug WHERE drug_d = " +
                                 to_string(item.first));

            auto ar = a->Fetch();
            auto av = FlatVector::GetData<int>(ar->data[0]);
            cout << oid << "," << item.first << "," << av[0] << ","
                 << item.second << endl;
          }

          pvUnpinBuffer(E->getArrayName(), dcoords);
        }
      }

      fResChunk = fRes->Fetch();
    }
    cout << "resCnt: " << resCnt << endl;

  } else {
    tblStart = system_clock::now();
    auto fRes = dconn.Query(
        "SELECT DISTINCT Rdrug.drug_d, Rdrug.drug "
        "FROM Prescription, Rdrug "
        "WHERE Rdrug.drug = Prescription.drug_id AND "
        "Prescription.patient_id "
        "= " +
        to_string(patientId));
    tblTime +=
        duration_cast<nanoseconds>(system_clock::now() - tblStart).count();

    auto fResChunk = fRes->Fetch();
    size_t resCnt = 0;
    while (fResChunk) {
      auto drugVec = FlatVector::GetData<int>(fResChunk->data[0]);
      auto originalDrugIdVec = FlatVector::GetData<int>(fResChunk->data[1]);
      for (int i = 0; i < fResChunk->size(); ++i) {
        auto id = drugVec[i];
        auto oid = originalDrugIdVec[i];

        uint64_t colTileSize = drugSize;
        uint64_t colNumTiles = (drugSize + drugSize - 1) / colTileSize;
        for (uint64_t colTileIdx = 0; colTileIdx < colNumTiles; colTileIdx++) {
          std::vector<uint64_t> dcoords = {0, colTileIdx};
          auto page =
              pvGetBuffer(E->getArrayName(), dcoords, BF_EMPTYTILE_NONE);
          if (page == NULL) {
            pvUnpinBuffer(E->getArrayName(), dcoords);
            continue;
          }

          auto val = t9GetValues(dconn, page, id);
          resCnt += val.size();  // not to be eliminated

          pvUnpinBuffer(E->getArrayName(), dcoords);
        }
      }

      fResChunk = fRes->Fetch();
    }
    cout << "resCnt: " << resCnt << endl;
  }

  totalTime =
      duration_cast<nanoseconds>(system_clock::now() - totalStart).count();

  cout << "[TASK9] DONE" << endl;
  cout << "totalTime =" << setw(12) << totalTime << " ns" << endl;
  cout << "tblTime   =" << setw(12) << tblTime << " ns" << endl;
  cout << "docTime   =" << setw(12) << docTime << " ns" << endl;
  cout << "arrTime   =" << setw(12) << arrTime << " ns" << endl;
}
