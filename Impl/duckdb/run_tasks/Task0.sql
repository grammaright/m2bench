LOAD json;

CREATE TEMPORARY TABLE TNEW_D (person_id INT, tag_id INT, val DOUBLE PRECISION);
CREATE INDEX TNEW_D_0 ON TNEW_D(person_id, tag_id);
CREATE INDEX TNEW_D_1 ON TNEW_D(tag_id);
CREATE INDEX TNEW_D_2 ON TNEW_D(person_id);

CREATE TEMPORARY TABLE TNEW_E (person_id INT, val DOUBLE PRECISION);
CREATE INDEX TNEW_E_0 ON TNEW_E(person_id);

DROP TABLE TEMP_w;
CREATE TABLE TEMP_w (i INT, val DOUBLE PRECISION);
CREATE INDEX TNEW_w_0 ON TEMP_w(i);

CREATE TEMPORARY TABLE TEMP_w_new (i INT, val DOUBLE PRECISION);

-- Fill temp_w first
INSERT INTO TEMP_w
SELECT SEQ.i AS i, 1 AS val FROM (SELECT generate_series AS i FROM generate_series(0, 300 - 1)) AS SEQ;
-- SELECT SEQ.i AS i, random() AS val FROM (SELECT generate_series(0, 300 - 1) AS i) AS SEQ;


-- Create A
CREATE TEMPORARY TABLE TNEW_A AS (
    SELECT CAST(_from AS INTEGER) AS person_id, CAST(_to AS INTEGER) AS tag_id
    FROM interested_in
	GROUP BY person_id, tag_id
);


-- Create C
CREATE TEMPORARY TABLE TNEW_C AS (
        WITH B AS (
                SELECT customer.person_id, product.brand_id, COUNT(*) as cnt
                FROM product, review, "order", customer
                WHERE customer.customer_id = CAST("order".data->>'customer_id' AS VARCHAR) AND
                CAST("order".data->>'order_id' AS VARCHAR) = CAST(review.data->>'order_id' AS VARCHAR) AND
                CAST(review.data->>'product_id' AS VARCHAR) = product.product_id AND
                CAST(review.data->>'rating' AS INT) = 5
                GROUP BY customer.person_id, product.brand_id)

        SELECT B.person_id, MIN(B.brand_id) AS brand_id
        FROM B, (SELECT person_id, MAX(cnt) as max_cnt FROM B GROUP BY person_id) AS T1
        WHERE B.person_id = T1.person_id AND
        B.cnt = T1.max_cnt
        GROUP BY B.person_id
);


-- Create D
INSERT INTO TNEW_D
SELECT 
	DENSE.person_id AS person_id,
	DENSE.tag_id AS tag_id,
	CASE
		WHEN TNEW_A.person_id IS NULL THEN 0
		ELSE 1
	END AS val
FROM (
    SELECT FLOOR(SEQ.i / 300)::INTEGER AS person_id, FLOOR(SEQ.i % 300)::INTEGER AS tag_id 
    FROM (
        SELECT generate_series AS i FROM generate_series(0, 2984700 - 1)
    ) AS SEQ
) AS DENSE 
LEFT JOIN 
	TNEW_A
	ON TNEW_A.person_id = DENSE.person_id AND TNEW_A.tag_id = DENSE.tag_id;


-- Create E
INSERT INTO TNEW_E 
SELECT 
	DENSE.person_id AS person_id,
	CASE
		WHEN TNEW_C.person_id IS NULL THEN 0
		WHEN TNEW_C.brand_id = 50 THEN 1
		ELSE 0
	END AS val
FROM (
    SELECT SEQ.i AS person_id 
	FROM (
        SELECT generate_series AS i FROM generate_series(0, 9949 - 1)) AS SEQ) AS DENSE 
LEFT JOIN 
	TNEW_C
	ON TNEW_C.person_id = DENSE.person_id;


-- iteration
CREATE TEMPORARY TABLE Xw AS (
	SELECT TNEW_D.person_id AS person_id, SUM(TNEW_D.val * TEMP_w.val) AS val
	FROM TNEW_D, TEMP_w
	WHERE TNEW_D.tag_id = TEMP_w.i
	GROUP BY TNEW_D.person_id
);


INSERT INTO TEMP_w_new
SELECT TEMP_w.i AS i, (TEMP_w.val - OLD.val) AS val
FROM (SELECT TNEW_D.tag_id AS i, 0.0001 * SUM(DIFF.val * TNEW_D.val) AS val
	FROM (SELECT Xw.person_id AS person_id, (1 / (1 + EXP(-1 * Xw.val))) - TNEW_E.val AS val 
		FROM Xw, TNEW_E
		WHERE Xw.person_id = TNEW_E.person_id) AS DIFF, TNEW_D
	WHERE TNEW_D.person_id = DIFF.person_id
	GROUP BY TNEW_D.tag_id) AS OLD, TEMP_w
WHERE OLD.i = TEMP_w.i;

DELETE FROM TEMP_w;

INSERT INTO TEMP_w
SELECT i, val FROM TEMP_w_new;

DELETE FROM TEMP_w_new;

SELECT COUNT(*) FROM TEMP_w;
