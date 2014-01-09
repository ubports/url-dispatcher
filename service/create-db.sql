create table if not exists configdirs (path text, timestamp datetime);
create table if not exists configfiles (directory integer, name text, timestamp datetime);
create table if not exists urls (sourcefile integer, protocol text, domainsuffix text);
