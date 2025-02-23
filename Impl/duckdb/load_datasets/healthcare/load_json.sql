USE healthcare;
LOAD json;
  
-- TODO: It is necessary to check if omitting the `sed` affects the result
-- COPY Drug (data) FROM program 'sed -e ''s/\\/\\\\/g'' /tmp/m2bench/healthcare/json/drug.json';

INSERT INTO "Drug"
SELECT * FROM read_json_objects('/tmp/m2bench/healthcare/json/drug.json', format='auto');

