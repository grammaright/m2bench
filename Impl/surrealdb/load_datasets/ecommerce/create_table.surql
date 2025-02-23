USE DB ecommerce;

REMOVE TABLE IF EXISTS brand;
REMOVE TABLE IF EXISTS product;
REMOVE TABLE IF EXISTS customer;

-- brand
DEFINE TABLE brand SCHEMAFULL;

DEFINE FIELD brand_id ON brand TYPE int;
DEFINE FIELD name ON brand TYPE string;
DEFINE FIELD country ON brand TYPE string;
DEFINE FIELD industry ON brand TYPE string;

-- product
DEFINE TABLE product SCHEMAFULL;

DEFINE FIELD product_id ON product TYPE string;
DEFINE FIELD title ON product TYPE string;
DEFINE FIELD price ON product TYPE float;
DEFINE FIELD brand_id ON product TYPE int;

DEFINE INDEX product_product_id ON TABLE product COLUMNS product_id UNIQUE;

-- customer
DEFINE TABLE customer SCHEMAFULL;

DEFINE FIELD customer_id ON customer TYPE string;
DEFINE FIELD person_id ON customer TYPE int;
DEFINE FIELD gender ON customer TYPE string;
DEFINE FIELD date_of_birth ON customer TYPE string;
DEFINE FIELD zipcode ON customer TYPE string;
DEFINE FIELD city ON customer TYPE string;
DEFINE FIELD county ON customer TYPE string;
DEFINE FIELD state ON customer TYPE string;

DEFINE INDEX customer_customer_id ON TABLE customer COLUMNS customer_id UNIQUE;
