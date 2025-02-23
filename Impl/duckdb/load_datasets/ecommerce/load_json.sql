USE ecommerce;
LOAD json;
  
INSERT INTO "order"
SELECT * FROM read_json_objects('/tmp/m2bench/ecommerce/json/order.json', format='auto');

-- COPY "order" (data) FROM '/tmp/m2bench/ecommerce/json/order.json' (AUTO_DETECT true);
-- CREATE INDEX order_customer_id_idx ON "order"((data->>'customer_id'));
-- CREATE INDEX order_order_id_idx ON "order"((data->>'order_id'));
-- CREATE INDEX order_orderline_product_id_idx ON "order"((data->'order_line'->>'product_id'));
-- CREATE INDEX order_orderline_product_id_idx2 ON "order"((data->'order_line'));

-- COPY review (data) FROM '/tmp/m2bench/ecommerce/json/review.json' (AUTO_DETECT true);
INSERT INTO "review"
SELECT * FROM read_json_objects('/tmp/m2bench/ecommerce/json/review.json', format='auto');
-- CREATE INDEX review_order_id_idx ON review((data->>'order_id'));
-- CREATE INDEX review_product_id_idx ON review((data->>'product_id'));
-- CREATE INDEX ON review(CAST(data->>'rating' AS INT));
-- CREATE INDEX ON review(CAST(data->>'rating' AS INT), (data->>'order_id'));
