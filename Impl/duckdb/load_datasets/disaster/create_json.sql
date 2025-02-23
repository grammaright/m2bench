USE disaster;
LOAD json;
  
DROP TABLE IF EXISTS Site;
CREATE TABLE IF NOT EXISTS Site (
        data JSON
);

DROP TABLE IF EXISTS Site_centroid;
CREATE TABLE Site_centroid (
        data JSON
);
