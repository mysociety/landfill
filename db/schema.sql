--
-- schema.sql:
-- GiveItAway schema
--
-- Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
-- Email: chris@mysociety.org; WWW: http://www.mysociety.org/
--
-- $Id: schema.sql,v 1.7 2005-10-20 09:46:41 chris Exp $
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

