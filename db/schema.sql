-- schema.sql:
-- Schema for Guardian Ideas thing
--
-- Copyright (c) 2008 UK Citizens Online Democracy. All rights reserved.
-- Email: matthew@mysociety.org; WWW: http://www.mysociety.org/
--
-- $Id: schema.sql,v 1.1 2008-12-12 14:56:57 matthew Exp $
--

-- secret
-- A random secret.
create table secret (
    secret text not null
);

-- If a row is present, that is date which is "today".  Used for debugging
-- to advance time without having to wait.
create table debugdate (
    override_today date
);

-- Returns the date of "today", which can be overriden for testing.
create function ms_current_date()
    returns date as '
    declare
        today date;
    begin
        today = (select override_today from debugdate);
        if today is not null then
           return today;
        else
           return current_date;
        end if;

    end;
' language 'plpgsql' stable;

-- Returns the timestamp of current time, but with possibly overriden "today".
create function ms_current_timestamp()
    returns timestamp as '
    declare
        today date;
    begin
        today = (select override_today from debugdate);
        if today is not null then
           return today + current_time;
        else
           return current_timestamp;
        end if;
    end;
' language 'plpgsql';

create table idea (
    id serial not null primary key,
    created timestamp not null,
    modified timestamp not null,
    saved_at text not null,

    idea_type integer not null check (idea_type > 0 and idea_type < 6),

    name text not null,
    email text not null,
    department text not null,
    
    title text not null,
    description text not null,

    benefits text,

    change_section text not null,

    evidence text,
    research text,

    data_already boolean,
    data_source text,

    cost_implications text,
    cost_future text,

    timing_constraint boolean,
    timing_why integer not null,
    timing_live text not null,
    date_fixed integer not null,
    timing_comments text not null,

    stats text,
    webstats_text text,

    benefits_tick text,
    benefits_text text,
    benefits_research text,
    benefits_research_text text,

    sponsorship text
);
create index idea_saved_at_idx on idea(saved_at);

-- Stores randomly generated tokens and serialised hash arrays associated
-- with them.
create table token (
    scope text not null,        -- what bit of code is using this token
    token text not null,
    data bytea not null,
    created timestamp not null,
    primary key (scope, token)
);
