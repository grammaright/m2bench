USE DB ecommerce;

REMOVE TABLE IF EXISTS order;
REMOVE TABLE IF EXISTS review;

-- order table
DEFINE TABLE order SCHEMALESS;

-- review table
DEFINE TABLE review SCHEMALESS;
