-- Schema change to add "ccme" field to volunteer_interest
-- BenC 2006-12-11

begin transaction;
create temporary table temp_volunteer_interest (
    ticket_num integer not null references ticket(tn),
    name text not null,
    email text not null,
    whenregistered integer not null
);
insert into temp_volunteer_interest SELECT ticket_num, name, email, whenregistered FROM volunteer_interest;
drop table volunteer_interest;
create table volunteer_interest (
    ticket_num integer not null references ticket(tn),
    name text not null,
    email text not null,
    whenregistered integer not null,
    ccme boolean not null
);
insert into volunteer_interest SELECT ticket_num, name, email, whenregistered, 'FALSE' as ccme FROM temp_volunteer_interest;
drop table temp_volunteer_interest;
commit;
