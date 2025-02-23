#DUCKDB_BIN=/media/nvme2/Workspace/mxm/tileduck/duckdb-prevision/build/release/duckdb
DUCKDB_BIN=/media/nvme2/Workspace/mxm/tileduck/duckdb-pure/build/release/duckdb

#$DUCKDB_BIN ecommerce -c "CREATE EXTENSION IF NOT EXISTS postgis;"

$DUCKDB_BIN ecommerce < ./create_table.sql
$DUCKDB_BIN ecommerce < ./load_table.sql

$DUCKDB_BIN ecommerce < ./create_json.sql
$DUCKDB_BIN ecommerce < ./load_json.sql

# TileDuck does not support graph so graph data will be loaded as table
$DUCKDB_BIN ecommerce < ./create_graph.sql
$DUCKDB_BIN ecommerce < ./load_graph.sql
