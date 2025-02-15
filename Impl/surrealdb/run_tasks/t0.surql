-- B
SELECT
        (
                SELECT
                        (SELECT person_id FROM customer WHERE $parent.customer_id = costumer_id LIMIT 1)[0].person_id  AS person_id
                FROM order
                WHERE order.order_id = $parent.order_id
                LIMIT 1
        )[0].person_id AS person_id,
        (SELECT brand_id FROM product WHERE $parent.product_id = product_id LIMIT 1)[0].brand_id AS brand_id,
        COUNT(review_id) AS cnt
FROM review
WHERE rating = 5
GROUP BY person_id, brand_id;
