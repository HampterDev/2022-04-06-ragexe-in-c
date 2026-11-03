SET NAMES latin1;
SET SESSION max_recursive_iterations = 70000;
DROP TABLE IF EXISTS cp;
CREATE TABLE cp (cp INT PRIMARY KEY, b VARBINARY(4) NOT NULL, s VARCHAR(4) CHARACTER SET latin1 COLLATE latin1_swedish_ci NOT NULL, KEY (s)) ENGINE=MyISAM;
INSERT INTO cp (cp, b, s)
WITH RECURSIVE n(cp) AS (SELECT 128 UNION ALL SELECT cp + 1 FROM n WHERE cp < 65535)
SELECT cp, CAST(CONVERT(CHAR(cp USING utf32) USING utf8mb4) AS BINARY), CAST(CONVERT(CHAR(cp USING utf32) USING utf8mb4) AS BINARY)
FROM n WHERE cp NOT BETWEEN 55296 AND 57343;
SELECT COUNT(*) AS bmp_chars FROM cp;
-- characters whose UTF-8 bytes compare equal to another character's bytes
SELECT COUNT(*) AS chars_in_collision_groups FROM cp a WHERE EXISTS (SELECT 1 FROM cp b WHERE b.s = a.s AND b.b <> a.b);
SELECT CASE WHEN a.cp BETWEEN 1024 AND 1279 THEN 'Cyrillic' WHEN a.cp BETWEEN 44032 AND 55203 THEN 'Hangul syllables' WHEN a.cp BETWEEN 19968 AND 40959 THEN 'CJK unified' WHEN a.cp BETWEEN 12352 AND 12543 THEN 'Kana' WHEN a.cp < 2048 THEN 'other 2-byte' ELSE 'other 3-byte' END AS block, COUNT(*) AS colliding
FROM cp a WHERE EXISTS (SELECT 1 FROM cp b WHERE b.s = a.s AND b.b <> a.b) GROUP BY block;
-- examples
SELECT HEX(a.b) AS a_utf8, a.cp AS a_cp, HEX(b.b) AS b_utf8, b.cp AS b_cp FROM cp a JOIN cp b ON b.s = a.s AND b.cp > a.cp WHERE a.cp BETWEEN 1024 AND 1279 OR a.cp BETWEEN 44032 AND 55203 LIMIT 6;
