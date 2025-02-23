#DUCKDB_BIN=/media/nvme2/Workspace/mxm/tileduck/duckdb-prevision/build/release/duckdb
DUCKDB_BIN=/media/nvme2/Workspace/mxm/tileduck/duckdb-pure/build/release/duckdb

$DUCKDB_BIN disaster < ./create_table.sql
$DUCKDB_BIN disaster < ./load_table.sql

$DUCKDB_BIN disaster < ./create_json.sql
$DUCKDB_BIN disaster < ./load_json.sql

$DUCKDB_BIN disaster < ./create_array.sql
$DUCKDB_BIN disaster < ./load_array.sql

# TileDuck does not support graph so graph data will be loaded as table
$DUCKDB_BIN disaster < ./create_graph.sql
$DUCKDB_BIN disaster < ./load_graph.sql
