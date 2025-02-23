USE disaster;
  
DROP TABLE IF EXISTS Roadnodes;
DROP TABLE IF EXISTS Roads;

CREATE TABLE IF NOT EXISTS Roadnodes (
    roadnode_id INT,
    site_id INT
);

CREATE TABLE IF NOT EXISTS Roads (
    _from INT,
    _to INT,
    distance INT
);
