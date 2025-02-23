#DUCKDB_BIN=/media/nvme2/Workspace/mxm/tileduck/duckdb-prevision/build/release/duckdb
DUCKDB_BIN=/media/nvme2/Workspace/mxm/tileduck/duckdb-pure/build/release/duckdb

$DUCKDB_BIN healthcare < ./create_table.sql
$DUCKDB_BIN healthcare < ./load_table.sql

$DUCKDB_BIN healthcare < ./create_json.sql
$DUCKDB_BIN healthcare < ./load_json.sql

# TileDuck does not support graph so graph data will be loaded as table
$DUCKDB_BIN healthcare < ./create_graph.sql
$DUCKDB_BIN healthcare < ./load_graph.sql
