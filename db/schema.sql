--
-- schema.sql:
-- Debugging stuff.
--
-- Copyright (c) 2006 Chris Lightfoot. All rights reserved.
-- Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
--
-- $Id: schema.sql,v 1.1 2006-08-31 12:19:43 chris Exp $
--

create schema debug;

grant usage on schema debug to public;

-- Information we record about each client to the database. Unfortunately
-- there's no sane way to share this between the various databases, so instead
-- we put a copy in each database; the monitor process then has to connect to
-- each database to be able to read the data out.
create table debug.client (
    -- Backend PID
    backendpid integer not null primary key default pg_backend_pid()
        check (backendpid = pg_backend_pid()),

    -- Hostname of host where client is running
    host text not null,

    -- Information about the TCP connection between client and server.
    clientaddr inet,
    clientport integer
        check (clientport is null
                or (clientport >= 1 and clientport < 65536)),
                
    serveraddr inet,
    serverport integer
        check (serverport is null
                or (serverport >= 1 and serverport < 65536)),

    -- Database in use.
    dbname text not null default current_database()
        check(dbname = current_database()),
    dbuser text not null default current_user
        check (dbuser = current_user),

    -- PID and PPID of client process.
    pid integer not null check (pid > 1),
    ppid integer not null check (ppid >= 1),

    -- UNIX credentials of client.
    uid integer not null check (uid >= 0),
    gid integer not null check (gid >= 0),

    -- Arguments, environment and current directory of client (at connect).
    argv bytea,     -- would like to make this "not null", but can't as it's
                    -- not guaranteed that you can actually access argv[] from
                    -- outside main on all platforms (it's not standard,
                    -- unlike environ(3)).
    environ bytea not null,
    cwd text,       -- it is possible that a process might not be able to
                    -- determine its own cwd -- see getcwd(3).

    -- When connected.
    connected timestamp not null default current_timestamp
        check (connected = current_timestamp),

    -- Other checks.
    check ((clientaddr is null and clientport is null
            and serveraddr is null and serverport is null) or
            (clientaddr is not null and clientport is not null
            and serveraddr is not null and serverport is not null))
);

grant select, insert, delete on debug.client to public;
    -- Ideally we'd like to be able to prevent any client from deleting any
    -- other client's rows in the client table, but ignore this for the moment
    -- (probably just needs a trigger). Note also that we need the "select"
    -- privilege to do a "delete ... where ...".

create function debug.record_client_info(
                    text,           -- client hostname
                    inet, integer,  -- client host/port
                    inet, integer,  -- server host/port
                    text,           -- database name
                    text,           -- user name
                    integer,        -- client PID
                    integer,        -- client PPID
                    integer,        -- client UID
                    integer,        -- client GID
                    bytea, bytea,   -- argv and environ
                    text            -- current working directory
        ) returns boolean as '
    delete from debug.client where backendpid = pg_backend_pid();
    insert into debug.client (
            host, clientaddr, clientport, serveraddr, serverport,
            dbname, dbuser, pid, ppid, uid, gid,
            argv, environ, cwd
        ) values (
            $1, $2, $3, $4, $5,
            $6, $7, $8, $9, $10, $11,
            $12, $13, $14
        );
    select true as result;
' language sql;

grant execute on function debug.record_client_info(
                    text,           -- client hostname
                    inet, integer,  -- client host/port
                    inet, integer,  -- server host/port
                    text,           -- database name
                    text,           -- user name
                    integer,        -- client PID
                    integer,        -- client PPID
                    integer,        -- client UID
                    integer,        -- client GID
                    bytea, bytea,   -- argv and environ
                    text            -- current working directory
        ) to public;
