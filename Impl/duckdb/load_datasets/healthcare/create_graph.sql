USE healthcare;

DROP TABLE IF EXISTS Disease;
DROP TABLE IF EXISTS Is_a;

CREATE TABLE IF NOT EXISTS Disease (
    disease_id BIGINT,
    term varchar(10000)
);

CREATE TABLE IF NOT EXISTS Is_a (
    source_id BIGINT,
    label varchar(10),
    destination_id BIGINT
);

