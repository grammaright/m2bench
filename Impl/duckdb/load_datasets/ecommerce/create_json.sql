USE ecommerce;
LOAD json;
  
DROP TABLE IF EXISTS "order";
DROP TABLE IF EXISTS review;

CREATE TABLE IF NOT EXISTS "order" (
        data JSON
        );

CREATE TABLE IF NOT EXISTS review (
        data JSON
        );
