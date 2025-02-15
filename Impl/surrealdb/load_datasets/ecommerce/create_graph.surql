USE DB ecommerce;

REMOVE TABLE IF EXISTS person;
REMOVE TABLE IF EXISTS follows;
REMOVE TABLE IF EXISTS hashtag;
REMOVE TABLE IF EXISTS interested_in;

-- person
DEFINE TABLE person SCHEMAFULL;

DEFINE FIELD person_id ON person TYPE int;
DEFINE FIELD gender ON person TYPE string;
DEFINE FIELD date_of_brith ON person TYPE string;
DEFINE FIELD firstname ON person TYPE string;
DEFINE FIELD lastname ON person TYPE string;
DEFINE FIELD nationality ON person TYPE string;
DEFINE FIELD email ON person TYPE string;

-- follows
DEFINE TABLE follows SCHEMAFULL TYPE RELATION FROM person TO person;

-- DEFINE FIELD _from ON follows TYPE int;
-- DEFINE FIELD _to ON follows TYPE int;
DEFINE FIELD created_time ON follows TYPE string;

-- hashtag
DEFINE TABLE hashtag SCHEMAFULL;

DEFINE FIELD tag_id ON hashtag TYPE int;
DEFINE FIELD content ON hashtag TYPE string;

-- interested_in
DEFINE TABLE interested_in SCHEMAFULL TYPE RELATION FROM person TO hashtag;

-- DEFINE FIELD _from ON interested_in TYPE int;
-- DEFINE FIELD _to ON interested_in TYPE int;
DEFINE FIELD created_time ON interested_in TYPE string;



