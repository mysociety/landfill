--
-- schema.sql:
-- GiveItAway schema
--
-- Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
-- Email: chris@mysociety.org; WWW: http://www.mysociety.org/
--
-- $Id: schema.sql,v 1.9 2005-10-21 17:00:12 chris Exp $
--

-- convenience functions
create function epoch(timestamp) returns double precision as
    'select case when $1 is not null then extract(epoch from $1) else null end'
    language 'sql';
    
create function epoch(timestamptz) returns double precision as
    'select case when $1 is not null then extract(epoch from $1) else null end'
    language 'sql';


create table secret (
    id integer primary key default(0) check (id = 0),
    secret text not null
);

create table category (
    id serial not null primary key,
    parent_category_id integer references category(id),
    name text not null
);

insert into category (name) values ('clothes');
insert into category (name) values ('books');
insert into category (name) values ('furniture');
insert into category (name) values ('electronics');
insert into category (name) values ('computers');
insert into category (name) values ('kitchen equipment');
insert into category (name) values ('other');

create table location (
    id serial not null primary key,
    postcode text not null,
    lat double precision not null,
    lon double precision not null
);

create index location_lat_idx on location(lat);
create index location_lon_idx on location(lon);

-- XXX add near query facility from PB

create table item (
    id serial not null primary key,
    -- identity of donor
    email text not null,
    name text not null,
    -- about the item
    category_id integer references category(id),
    description text not null,
    location_id integer not null references location(id),
    -- about the donor
--    ipaddr text not null,
--    referrer text,
    -- state
    whenadded timestamp not null default (current_timestamp),
    whenconfirmed timestamp
);

create index item_email_idx on item(email);

create table acceptor (
    id serial not null primary key,
    email text not null,
    name text not null,
    organisation text not null,
    charity_number text,
    location_id integer not null references location(id),
    bouncing boolean default (false),
    whenadded timestamp not null default (current_timestamp)
);

create table acceptor_category_interest (
    acceptor_id integer not null references acceptor(id),
    category_id integer not null references category(id),
    primary key(acceptor_id, category_id)
);

create index acceptor_category_interest_acceptor_id_idx
    on acceptor_category_interest(acceptor_id);
    
create index acceptor_category_interest_category_id_idx
    on acceptor_category_interest(category_id);

create table acceptor_item_interest (
    acceptor_id integer not null,
    item_id integer not null references item(id),
    whensent timestamp not null default current_timestamp,
    whenaccepted timestamp,
    whendeclined timestamp,
    primary key(acceptor_id, item_id)
);

create index acceptor_item_interest_acceptor_id_idx
    on acceptor_item_interest(acceptor_id);
    
create index acceptor_item_interest_item_id_idx
    on acceptor_item_interest(item_id);

-- item_current_acceptor ITEM
-- Return the ID of the current acceptor for ITEM, if any.
create function item_current_acceptor(integer) returns integer as '
    select acceptor_id
    from acceptor_item_interest
    where item_id = $1 and whendeclined is null
        and whensent > current_timestamp - ''3 days''::interval
    order by whensent desc limit 1
' language sql;

-- R_e
-- Radius of the earth, in km. This is something like 6372.8 km:
--  http://en.wikipedia.org/wiki/Earth_radius
create function R_e()
    returns double precision as 'select 6372.8::double precision;' language sql;

-- great_circle_distance LAT1 LON1 LAT2 LON2
-- Return the great circle distance between points (LAT1, LON1) and (LAT2,
-- LON2) in kilometers.
create function great_circle_distance(double precision, double precision, double precision, double precision) returns double precision as '
    select R_e() * acos(
        sin(radians($1)) * sin(radians($3))
        + cos(radians($1)) * cos(radians($3))
            * cos(radians($2 - $4))
        )
' language sql;

