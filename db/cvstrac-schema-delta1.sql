--
-- cvstrac-schema-changes.sql:
-- Additions to the cvstrac database for our volunteer tasks system.
--
-- Copyright (c) 2005 UK Citizens Online Democracy. All rights reserved.
-- Email: chris@mysociety.org; WWW: http://www.mysociety.org/
--
-- $Id: cvstrac-schema-delta1.sql,v 1.1 2006-12-11 15:50:21 ben Exp $
--

create table volunteer_interest (
    ticket_num integer not null references ticket(tn),
    name text not null,
    email text not null,
    whenregistered integer not null
);
