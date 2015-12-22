#include "gtfs.h"
#include "mysql_db.h"
#include "functions.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

vector<string> split(string &str, char tok)
{
    vector<string> ret_vec;
    stringstream ss(str);
    string the_str;

    while(getline( ss, the_str, tok ))
        ret_vec.push_back(the_str);

    //empty last item?
    if(str.size() && str.at(str.size()-1) == tok)
        ret_vec.push_back("");

    return ret_vec;
}

void load_gtfs_file(int db_type, bool force_reload)
{
    string filename;
    string tablename;

    switch(db_type)
    {
        case GTFS_TRIPS_DB:
            filename = "gtfs_public/trips.txt";
            tablename = "gtfs_trips_data";
            break;
        case GTFS_STOP_TIMES_DB:
            filename = "gtfs_public/stop_times.txt";
            tablename = "gtfs_stop_times_data";
            break;
        case GTFS_STOPS_DB:
            filename = "gtfs_public/stops.txt";
            tablename = "gtfs_stops_data";
            break;
        case GTFS_ROUTES_DB:
            filename = "gtfs_public/routes.txt";
            tablename = "gtfs_routes_data";
            break;
        case GTFS_CALENDAR_DB:
            filename = "gtfs_public/calendar.txt";
            tablename = "gtfs_calendar_data";
            break;
        default:
            printf("load_gtfs_file:invalid db type \n");
            return;
            break;
    }

    ifstream infile(filename);

    if(!infile.is_open())
    {
        printf("could not load '%s' \n", filename.c_str());
        printf("download gtfs from http://www3.septa.org/hackathon/ \n", filename.c_str());
        return;
    }

    MysqlDB *myobj = MysqlDB::getInstance();

    //already loaded and not forcing a reload?
    if(!force_reload && myobj->tableExists(tablename)) return;

    //drop and then recreate
    myobj->dropTable(tablename);

    switch(db_type)
    {
        case GTFS_TRIPS_DB: myobj->createGTFSTripsTable(); break;
        case GTFS_STOP_TIMES_DB: myobj->createGTFSStopTimesTable(); break;
        case GTFS_STOPS_DB: myobj->createGTFSStopsTable(); break;
        case GTFS_ROUTES_DB: myobj->createGTFSRoutesTable(); break;
        case GTFS_CALENDAR_DB: myobj->createGTFSCalendarTable(); break;
    }

    stop_watch();

    string line_str;

    //skip first line
    getline( infile, line_str );

    while(getline( infile, line_str ))
    {
        //get rid of char 13 \r if present
        line_str.erase(remove(line_str.begin(), line_str.end(), 13), line_str.end());

        vector<string> strings = split(line_str, ',');

        switch(db_type)
        {
            case GTFS_TRIPS_DB: myobj->insertGTFSTrip(strings); break;
            case GTFS_STOP_TIMES_DB: myobj->insertGTFSStopTime(strings); break;
            case GTFS_STOPS_DB: myobj->insertGTFSStop(strings); break;
            case GTFS_ROUTES_DB: myobj->insertGTFSRoute(strings); break;
            case GTFS_CALENDAR_DB: myobj->insertGTFSCalendar(strings); break;
        }

        //printf("load_gtfs_file:'%s' \n", line_str.c_str());

        //for(int i=0;i<strings.size();i++)
        //	printf("load_gtfs_file:...     '%s' \n", strings[i].c_str());
    }

    //create indexes
    switch(db_type)
    {
        case GTFS_TRIPS_DB: myobj->createGTFSTripsIndexes(); break;
        case GTFS_STOP_TIMES_DB: myobj->createGTFSStopTimesIndexes(); break;
        case GTFS_STOPS_DB: myobj->createGTFSStopsIndexes(); break;
        case GTFS_ROUTES_DB: myobj->createGTFSRoutesIndexes(); break;
        case GTFS_CALENDAR_DB: break;
    }

    stop_watch("load_gtfs_file - " + filename);
}

void load_gtfs(bool force_reload)
{
    for(int i=0;i<GTFS_DB_TYPE_MAX;i++)
        load_gtfs_file(i, force_reload);
}
