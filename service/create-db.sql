pragma journal_mode = WAL;
begin transaction;
create table if not exists configfiles (name text unique, timestamp bigint);
create table if not exists urls (sourcefile integer, protocol text, domainsuffix text);
create unique index if not exists urls_index on urls (sourcefile, protocol, domainsuffix);
commit transaction;
