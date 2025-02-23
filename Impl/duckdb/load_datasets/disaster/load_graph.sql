USE disaster;

COPY Roadnodes FROM '/tmp/m2bench/disaster/property_graph/Roadnode.csv' DELIMITER ',' CSV HEADER; 
COPY Roads FROM '/tmp/m2bench/disaster/property_graph/Road.csv' DELIMITER ',' CSV HEADER;
