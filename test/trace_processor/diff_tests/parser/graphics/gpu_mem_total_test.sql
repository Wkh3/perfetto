SELECT ct.name, ct.unit, ct.description, c.ts, p.pid, cast_int!(c.value) AS value
FROM counter_track ct
LEFT JOIN process_counter_track pct USING (id)
LEFT JOIN process p USING (upid)
LEFT JOIN counter c ON c.track_id = ct.id
ORDER BY ts;
