USE healthcare;

COPY Disease FROM '/tmp/m2bench/healthcare/property_graph/Disease_network_nodes.csv' DELIMITER ',' CSV HEADER; 
COPY Is_a FROM '/tmp/m2bench/healthcare/property_graph/Disease_network_edges.csv' DELIMITER ',' CSV HEADER;
