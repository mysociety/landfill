--
-- schema.sql:
-- GiveItAway schema
--
-- Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
-- Email: chris@mysociety.org; WWW: http://www.mysociety.org/
--
-- $Id: schema.sql,v 1.2 2005-10-14 18:06:29 chris Exp $
--

create table secret (
    id integer primary key default(0) check (id = 0),
    secret text not null
);

create table category (
    id serial not null primary key,
    parent_category_id integer references category(id,
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
    -- UK only for the moment
    easting double precision not null,
    northing double precision not null
);

create table item (
    id serial not null primary key,
    -- identity of donor
    email text not null,
    name text not null,
    -- about the item
    category_id integer references category(id),
    description text not null,
    location_id integer not null references location(id),
    -- state
    confirmed boolean default (f),
    available boolean default (t)
);

create index item_donor_email_idx on item(donor_email);

create table acceptor (
    id serial not null primary key,
    email text not null,
    name text not null,
    organisation text not null,
    location_id integer not null references location(id),
    bouncing boolean default (f)
);

create table acceptor_category_interest (
    acceptor_id integer not null references acceptor(id),
    category_id integer not null references category(id)
);

create table acceptor_item_interest (
    acceptor_id integer not null,
    item_id integer not null,
    whensent timestamp not null default current_timestamp,
    whenaccepted timestamp
);
