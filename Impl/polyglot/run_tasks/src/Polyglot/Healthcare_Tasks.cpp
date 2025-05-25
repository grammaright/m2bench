#include <time.h>

#include <chrono>

using namespace std;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

#define BUFFER 1000

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

void T9(int patient_id) {
  //   auto mysql = mysql_connector();

  //   auto mongodb = mongodb_connector("Healthcare");
  //   auto db = mongodb.db;
  //   auto drug = db["drug"];

  //   mongocxx::pipeline p{};

  //   p.unwind("$adverse_effect_list");
  //   p.project(make_document(
  //       kvp("drug", "$drug_id"),
  //       kvp("adverse_effect", "$adverse_effect_list.adverse_effect_name"),
  //       kvp("is_adverse_effect", make_document(kvp("$literal", 1)))));

  //   mysql.mysess->sql("use Healthcare").execute();
  //   mysql.mysess
  //       ->sql(
  //           "create temporary table D2A ( "
  //           " drug int, "
  //           " adverse_effect varchar(100) )")
  //       .execute();

  //   auto insert2D2A = mysql.mysess->getSchema("Healthcare")
  //                         .getTable("D2A")
  //                         .insert("drug", "adverse_effect");
  //   auto cursor = drug.aggregate(p, mongocxx::options::aggregate{});
  //   int buffer = 0;
  //   for (auto row : cursor) {
  //     int drug_id = row["drug"].get_int32();
  //     string adverse_effect_id =
  //     string(row["adverse_effect"].get_utf8().value); int is_adverse_effect =
  //     row["is_adverse_effect"].get_int32();

  //     insert2D2A.values(drug_id, adverse_effect_id);
  //     buffer++;
  //     if (buffer >= BUFFER) {
  //       insert2D2A.execute();
  //       buffer = 0;
  //       insert2D2A = mysql.mysess->getSchema("Healthcare")
  //                        .getTable("D2A")
  //                        .insert("drug", "adverse_effect");
  //     }
  //   }
  //   if (buffer != 0) insert2D2A.execute();
  //   auto D2A = mysql.mysess->getSchema("Healthcare").getTable("D2A");

  //   mysql.mysess
  //       ->sql(
  //           "CREATE TEMPORARY TABLE Rdrug as "
  //           "(SELECT t.drug, ROW_NUMBER() OVER () -1 as drug_d from "
  //           "(Select distinct(drug) as drug "
  //           "from D2A) as t )")
  //       .execute();

  //   mysql.mysess
  //       ->sql(
  //           "CREATE TEMPORARY TABLE Radverse_effect as "
  //           "(SELECT t.adverse_effect, ROW_NUMBER() OVER () -1 as "
  //           "adverse_effect_d from "
  //           "(Select distinct(adverse_effect) as adverse_effect "
  //           "from D2A) as t )")
  //       .execute();

  //   mysql.mysess->sql("CREATE INDEX Rdrug on Rdrug(drug)").execute();
  //   mysql.mysess
  //       ->sql("CREATE INDEX Radverse_effect on
  //       Radverse_effect(adverse_effect)") .execute();

  //   auto rows =
  //       mysql.mysess
  //           ->sql(
  //               "SELECT drug_d, adverse_effect_d "
  //               " From Rdrug, Radverse_effect, D2A "
  //               " Where D2A.drug = Rdrug.drug "
  //               " and D2A.adverse_effect = Radverse_effect.adverse_effect ")
  //           .execute();

  //   int dim1 =
  //   mysql.mysess->getSchema("Healthcare").getTable("Rdrug").count(); int dim2
  //   =
  //       mysql.mysess->getSchema("Healthcare").getTable("Radverse_effect").count();
  //   unique_ptr<ScidbConnection> conn(
  //       new ScidbConnection(SCIDB_HOST_HEALTHCARE + string(":8080")));

  //   conn->exec("remove(temp)");
  //   conn->exec("remove(drug_matrix)");
  //   conn->exec("remove(similarity1)");
  //   conn->exec("remove(similarity2)");
  //   conn->exec("remove(inv_norm)");
  //   conn->exec("remove(drug_similarity)");
  //   conn->exec(
  //       "create array temp<drug:int64 NOT NULL, adverse_effect:int64 NOT
  //       NULL, " "is_adverse_effect:double NOT NULL> [i=0:" + to_string(dim1 *
  //       dim2 - 1) + ":0:1000000]");
  //   int ncell = 0;

  //   ScidbSchema sschema;
  //   sschema.attrs.push_back(ScidbAttr("drug", INT64));
  //   sschema.attrs.push_back(ScidbAttr("adverse_effect", INT64));
  //   sschema.attrs.push_back(ScidbAttr("is_adverse_effect", DOUBLE));

  //   shared_ptr<ScidbArrFile> coo(new ScidbArrFile(sschema));

  //   for (auto row : rows) {
  //     long long dim_drug = row[0].get<long>();
  //     long long dim_adverse_effect = row[1].get<long>();
  //     double is_adverse_effect = 1.0;
  //     /***
  //      *
  //      * Pass to SCIDB
  //      *
  //      */
  //     ScidbLineType line;
  //     line.push_back(dim_drug);
  //     line.push_back(dim_adverse_effect);
  //     line.push_back(is_adverse_effect);
  //     coo->add(line);
  //     ncell++;
  //   }

  //   conn->upload("temp",
  //                coo);  // upload data to "testarray" array with coo format

  //   conn->exec(
  //       "store(redimension(temp, <is_adverse_effect:double NOT NULL>[drug=0:"
  //       + to_string(dim1 - 1) + ":0:1000; adverse_effect=0:" + to_string(dim2
  //       - 1) +
  //       ":0:1000], false),  drug_matrix)");
  //   conn->exec("store(spgemm(drug_matrix,transpose(drug_matrix)),similarity1)");
  //   conn->exec(
  //       "store(project(apply(filter(similarity1,drug=drug2),result,1/"
  //       "sqrt(multiply)),result),inv_norm)");
  //   conn->exec("store(spgemm(similarity1,inv_norm),similarity2)");
  //   conn->exec("store(spgemm(transpose(similarity2),inv_norm),drug_similarity)");

  //   mysql.mysess->sql("use Healthcare").execute();

  //   std::string query =
  //       "select distinct Rdrug.drug_d "
  //       "from Prescription, Rdrug "
  //       "where Rdrug.drug = Prescription.drug_id and Prescription.patient_id
  //       = " + to_string(patient_id);

  //   auto prescribed_drugs = mysql.mysess->sql(query).execute();

  //   ScidbSchema schema;
  //   schema.dims.push_back(ScidbDim("drug22", 0, INT32_MAX, 0, 1000000));
  //   schema.attrs.push_back(ScidbAttr("multiply", DOUBLE));

  //   // cout << "drug1\tdrug2\tsimilarity" << endl;
  //   int nrow = 0;
  //   for (auto row : prescribed_drugs) {
  //     // cout << "---------drug_idx: " << row[0].get<int>() << "----------"
  //     <<
  //     // endl;
  //     auto download = conn->download(
  //         "slice(drug_similarity,drug2," + to_string(row[0].get<int>()) +
  //         ")", schema);
  //     auto line = download->readcell();
  //     while (line.size() != 0) {
  //       // cout << row[0].get<int>() << "\t";
  //       // cout << get<int>(line.at(0)) << "\t";
  //       // cout << get<double>(line.at(1)) << "\t";
  //       // cout << endl;
  //       line = download->readcell();
  //       nrow++;
  //     }
  //   }

  //   cout << "TASK9: TOTAL " << nrow++ << " ROWS ARE REPORTED" << endl;
}
