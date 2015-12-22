#ifndef GTFS_H
#define GTFS_H

enum GTFS_DB_TYPE
{
    GTFS_TRIPS_DB,
    GTFS_STOP_TIMES_DB,
    GTFS_STOPS_DB,
    GTFS_ROUTES_DB,
    GTFS_CALENDAR_DB,
    GTFS_DB_TYPE_MAX
};

void load_gtfs(bool force_reload = false);
void load_gtfs_file(int db_type, bool force_reload = false);

#endif
