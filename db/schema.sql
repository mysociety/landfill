--
-- schema.sql:
-- Schema for tracking bug thing.
--
-- Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
-- Email: chris@mysociety.org; WWW: http://www.mysociety.org/
--
-- $Id: schema.sql,v 1.1 2005-12-02 18:03:30 chris Exp $
--

create table secret (
    id integer primary key default(0) check (id = 0),
    secret text not null
);

create table useragent (
    id serial not null primary key,
    useragent text not null
);

create index useragent_useragent_idx on useragent(useragent);

create sequence tracking_id_seq;

create table event (
    id serial not null primary key,
    tracking_id integer not null,
    when timestamp not null default current_timestamp,
    ipaddr varchar(17) not null,
    -- XXX some other information inferrable from 
    useragent_id integer references useragent(id),
    url text not null,      -- XXX this is going to cost quite a bit of space;
                            -- possibly compress it by having a urlprefix_id and
                            -- a urlsuffix?
    extradata bytea         -- anything else the generating page wants to record
);

create index event_tracking_id_idx on event(tracking_id);
create index event_when_idx on event(when);
create index event_ipaddr_idx on event(ipaddr);

