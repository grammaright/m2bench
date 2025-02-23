USE ecommerce;

DROP TABLE IF EXISTS person;
DROP TABLE IF EXISTS follows;
DROP TABLE IF EXISTS hashtag;
DROP TABLE IF EXISTS interested_in;

CREATE TABLE IF NOT EXISTS person(
    person_id int,
    gender char(1),
    date_of_brith date,
    firstname varchar(20),
    lastname varchar(20),
    nationality varchar(20),
    email varchar(50)
);

CREATE TABLE IF NOT EXISTS follows(
    _from int,
    _to int,
    created_time timestamp
);

CREATE TABLE IF NOT EXISTS hashtag(
    tag_id int,
    content varchar(30)
);

CREATE TABLE IF NOT EXISTS interested_in(
    _from int,
    _to int,
    created_time char(20)
);
