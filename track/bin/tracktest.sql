-- SQL query fragments to find out tracking things

-- Show for anyone with a subscribed=1 entry, how many times each front page style was viewed:
select count(*), extradata from event where tracking_id in (select tracking_id from event as e1 where extradata like '%subscribed=1%') and extradata like '%frontpagestyle%' group by extradata order by extradata;

-- For all events after a certain date, show events for all people who have manged to subscribe,
-- but never went to the front page. (Basically shows people with cookies turned off, or not
-- set to allow 3rd party cookies)
select * from event where tracking_id in (select tracking_id from event as e1 where extradata like '%subscribed=1%' and whenlogged >= '2006-03-27 16:59:26.587772' and (select count(*) from event where tracking_id = e1.tracking_id and extradata like '%frontpagestyle%') = 0) order by tracking_id, whenlogged;

