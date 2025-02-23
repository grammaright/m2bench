LOAD json;
LOAD spatial;

CREATE TEMP TABLE T15A (longitude int, latitude int, pm10_sum double precision, pm10_count int);
CREATE TEMP TABLE T15B (coordinates geometry, pm10_avg double precision);

INSERT INTO T15A
SELECT longitude, latitude, sum(pm10) AS pm10_sum, count(pm10) AS pm10_count
FROM Finedust_idx
WHERE (5 <= timestamp) AND (timestamp <= 10)
GROUP BY longitude, latitude;

CREATE INDEX t15a_latlon ON T15A (latitude, longitude);

INSERT INTO T15B
SELECT ST_Point(-118.34501002237936 + (t1.longitude * 0.000216636), 34.011898718557454 + (t1.latitude * 0.000172998)) AS coordinates, SUM(t2.pm10_sum) / SUM(t2.pm10_count) AS pm10_avg
FROM T15A as t1, T15A as t2
WHERE ((t1.latitude - 2) <= t2.latitude) AND (t2.latitude <= (t1.latitude + 2))
AND ((t1.longitude - 2) <= t2.longitude) AND (t2.longitude <= (t1.longitude + 2))
GROUP BY coordinates;

SELECT CAST(Site_centroid.data->>'site_id' AS INT)
FROM Site_centroid
WHERE Site_centroid.data->'properties'->>'type' = 'roadnode'
ORDER BY ST_Distance_Sphere(ST_GeomFromGeoJSON(Site_centroid.data->>'centroid'), ST_Point(-118.0614431, 34.068509)::geometry) ASC
LIMIT 1;

SELECT CAST(Site_centroid.data->>'site_id' AS INT)
FROM Site_centroid
WHERE Site_centroid.data->'properties'->>'type' = 'roadnode'
ORDER BY ST_Distance_Sphere(ST_GeomFromGeoJSON(Site_centroid.data->>'centroid'), ((
    SELECT T15B.coordinates FROM T15B, (SELECT MAX(T15B.pm10_avg) as max_avg FROM T15B) as tc1 WHERE T15B.pm10_avg = tc1.max_avg LIMIT 1
)::geometry)) ASC
LIMIT 1;

DROP TABLE T15A;
DROP TABLE T15B;
