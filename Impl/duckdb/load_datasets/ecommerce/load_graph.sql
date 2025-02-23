USE ecommerce;

COPY person FROM '/tmp/m2bench/ecommerce/property_graph/person_node.csv' DELIMITER '|' CSV HEADER; 
COPY follows FROM '/tmp/m2bench/ecommerce/property_graph/person_follows_person.csv' DELIMITER '|' CSV HEADER;
COPY hashtag FROM '/tmp/m2bench/ecommerce/property_graph/hashtag_node.csv' DELIMITER ',' CSV HEADER;
COPY interested_in FROM '/tmp/m2bench/ecommerce/property_graph/person_interestedIn_tag.csv' DELIMITER ',' CSV HEADER;
