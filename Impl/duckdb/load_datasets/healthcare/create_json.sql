USE healthcare;
LOAD json;
  
DROP TABLE IF EXISTS Drug;

CREATE TABLE IF NOT EXISTS Drug (
    data JSON
);
