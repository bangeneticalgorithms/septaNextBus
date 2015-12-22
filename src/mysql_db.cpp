#include "mysql_db.h"
#include "objects.h"
#include "functions.h"
#include "settings.h"

#include <string.h>

MysqlDB* MysqlDB::instance = NULL;

MysqlDB::MysqlDB()
{
    c_server = Settings::mysql_host;
    c_user = Settings::mysql_user;
    c_pass = Settings::mysql_pass;
    c_db = Settings::mysql_db;
    c_obj = NULL;

    doConnect();
    createStopTable();
    createBusTable();
    createBusTable(true);
    createCoeffTable();
    createWeatherTable();
    createWeatherTable(true);
    createGoogDirTable();
    createGoogDirTable(true);
    createPredictionTable();
    createRoutePerformanceTable();
    createRouteApplyModelTable();
    createRouteCostGraphTable();
    createRouteBlockIdTripsTable();
}

MysqlDB::~MysqlDB()
{
    doDisconnect();
}

MysqlDB* MysqlDB::getInstance()
{
    if(!instance) instance = new MysqlDB();

    return instance;
}

void MysqlDB::doConnect()
{
    if(c_obj) doDisconnect();

    c_obj = mysql_init(NULL);

    if(mysql_real_connect(c_obj, c_server.c_str(), c_user.c_str(), c_pass.c_str(), c_db.c_str(), 0, NULL, CLIENT_FOUND_ROWS))
        printf("MysqlDB::connected\n");
    else
    {
        printf("MysqlDB::could not connect: '%s'\n", mysql_error(c_obj));
        doDisconnect();
    }
}

void MysqlDB::createStopTable()
{
    makeDBCall("CREATE TABLE stop_data (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        lat double,\
            lng double,\
            stop_id int,\
            stopname varchar(128),\
            route_id varchar(128));");
}

void MysqlDB::createBusTable(bool is_front_end)
{
    string table_name;

    if(is_front_end)
        table_name = "bus_data_front_end";
    else
        table_name = "bus_data";

    makeDBCall("CREATE TABLE " + table_name + " (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        lat double,\
            lng double,\
            label int,\
            vehicle_id int,\
            block_id int,\
            direction varchar(128),\
            destination varchar(128),\
            offset int,\
            offset_sec int,\
            weather_timestamp int,\
            googdir_timestamp int,\
            route_id varchar(128),\
            timestamp int,\
            bus_count int);");

    //when grabbing bus data it is always linked with the corresponding weather entry
    if(!is_front_end)
    {
        makeDBCall("CREATE INDEX bus_data_wti USING BTREE ON bus_data (weather_timestamp);");
        makeDBCall("CREATE INDEX bus_data_ri USING BTREE ON bus_data (route_id);");
    }
}

void MysqlDB::createCoeffTable()
{
    makeDBCall("CREATE TABLE coeff_data (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        route_id varchar(128),\
            stop_id int,\
            theta double,\
            mu double,\
            sigma double,\
            co_num int);");

    //when everything is trained making these calls are slow for the prepredictor
    makeDBCall("CREATE INDEX coeff_data_rsi USING BTREE ON coeff_data (route_id, stop_id);");
}

void MysqlDB::createWeatherTable(bool is_front_end)
{
    string table_name;

    if(is_front_end)
        table_name = "weather_data_front_end";
    else
        table_name = "weather_data";

    makeDBCall("CREATE TABLE " + table_name + " (\
        timestamp int PRIMARY KEY,\
            cloudcover int,\
            humidity int,\
            precipMM double,\
            pressure int,\
            temp_C int,\
            temp_F int,\
            visibility int,\
            weatherCode int,\
            weatherDesc varchar(128),\
            winddir16Point varchar(128),\
            winddirDegree int,\
            windspeedKmph int,\
            windspeedMiles int,\
            w_precipMM double,\
            w_tempMaxC int,\
            w_tempMaxF int,\
            w_tempMinC int,\
            w_tempMinF int,\
            w_weatherCode int,\
            w_weatherDesc varchar(128),\
            w_winddir16Point varchar(128),\
            w_winddirDegree int,\
            w_winddirection varchar(128),\
            w_windspeedKmph int,\
            w_windspeedMiles int);");
}

void MysqlDB::createGoogDirTable(bool is_front_end)
{
    string table_name;

    if(is_front_end)
        table_name = "googdir_data_front_end";
    else
        table_name = "googdir_data";

    makeDBCall("CREATE TABLE " + table_name + " (PRIMARY KEY(route_id, gd_num, timestamp),\
        route_id varchar(128),\
            gd_num int,\
            duration int,\
            distance int,\
            steps int,\
            timestamp int);");

    if(!is_front_end)
        makeDBCall("CREATE INDEX googdir_data_rti USING BTREE ON googdir_data (route_id, timestamp);");
}

void MysqlDB::createPredictionTable()
{
    makeDBCall("CREATE TABLE prediction_data (PRIMARY KEY(bus_id, stop_id),\
        bus_id int,\
            stop_id int,\
            prediction double,\
            timestamp int);");
}

void MysqlDB::createRoutePerformanceTable()
{
    makeDBCall("CREATE TABLE route_perf_data (PRIMARY KEY(route_id, stop_id),\
        route_id varchar(128),\
            stop_id int,\
            data_points int,\
            data_trips int,\
            used_features int,\
            total_features int,\
            data_points_discarded int,\
            data_trips_discarded int,\
            discarded_bad_block_id int,\
            discarded_bad_dest int,\
            discarded_bad_dir int,\
            discarded_bad_dir_id int,\
            discarded_bad_stops_away int,\
            discarded_bad_node_times int,\
            discarded_bad_block_id_in int,\
            discarded_bad_weather_googdir int,\
            iterations int,\
            train_time double,\
            mean_squared_error double,\
            day_count_0 int,\
            day_count_1 int,\
            day_count_2 int,\
            day_count_3 int,\
            day_count_4 int,\
            day_count_5 int,\
            day_count_6 int,\
            hour_count_0 int,\
            hour_count_1 int,\
            hour_count_2 int,\
            hour_count_3 int,\
            hour_count_4 int,\
            hour_count_5 int,\
            hour_count_6 int,\
            hour_count_7 int,\
            hour_count_8 int,\
            hour_count_9 int,\
            hour_count_10 int,\
            hour_count_11 int,\
            hour_count_12 int,\
            hour_count_13 int,\
            hour_count_14 int,\
            hour_count_15 int,\
            hour_count_16 int,\
            hour_count_17 int,\
            hour_count_18 int,\
            hour_count_19 int,\
            hour_count_20 int,\
            hour_count_21 int,\
            hour_count_22 int,\
            hour_count_23 int,\
            timestamp int);");
}

void MysqlDB::createRouteApplyModelTable()
{
    makeDBCall("CREATE TABLE route_apply_model_data (PRIMARY KEY(route_id, stop_id, ram_num),\
        route_id varchar(128),\
            stop_id int,\
            ram_num int,\
            actual double,\
            predicted double,\
            timestamp int);");
}

void MysqlDB::createRouteCostGraphTable()
{
    makeDBCall("CREATE TABLE route_cost_graph_data (PRIMARY KEY(route_id, stop_id, rcg_num),\
        route_id varchar(128),\
            stop_id int,\
            rcg_num int,\
            cost double,\
            timestamp int);");
}

void MysqlDB::createRouteBlockIdTripsTable()
{
    makeDBCall("CREATE TABLE route_block_id_trips_data (PRIMARY KEY(route_id, stop_id, block_id),\
        route_id varchar(128),\
            stop_id int,\
            block_id int,\
            trips int,\
            trips_discarded int,\
            block_id_in bool,\
            timestamp int);");
}

void MysqlDB::createGTFSTripsTable()
{
    //route_id,service_id,trip_id,trip_headsign,block_id,direction_id,shape_id
    //12966,1,4165438,"204 Eagleview",8605,0,162669
    makeDBCall("CREATE TABLE gtfs_trips_data (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        route_id int,\
            service_id int,\
            trip_id int,\
            trip_headsign varchar(128),\
            block_id int,\
            direction_id int,\
            shape_id int);");
}

void MysqlDB::createGTFSStopTimesTable()
{
    //trip_id,arrival_time,departure_time,stop_id,stop_sequence
    //4120486,13:35:00,13:35:00,1874,1
    makeDBCall("CREATE TABLE gtfs_stop_times_data (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        trip_id int,\
            arrival_time time,\
            departure_time time,\
            stop_id int,\
            stop_sequence int);");
}

void MysqlDB::createGTFSStopsTable()
{
    //stop_id,stop_name,stop_lat,stop_lon,location_type,parent_station,zone_id,wheelchair_boarding
    //2,Ridge Av & Lincoln Dr  - FS,40.014986,-75.206826,,31032,1,0
    makeDBCall("CREATE TABLE gtfs_stops_data (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        stop_id int,\
            stop_name varchar(128),\
            stop_lat double,\
            stop_lon double,\
            location_type int,\
            parent_station int,\
            zone_id int,\
            wheelchair_boarding int);");
}

void MysqlDB::createGTFSRoutesTable()
{
    //route_id,route_short_name,route_long_name,route_type,route_color,route_text_color,route_url
    //13028,1,Parx Casino to 54th-City,3,,,
    makeDBCall("CREATE TABLE gtfs_routes_data (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        route_id int,\
            route_short_name varchar(128),\
            route_long_name varchar(128),\
            route_type int,\
            route_color varchar(32),\
            route_text_color varchar(32),\
            route_url varchar(512));");
}

void MysqlDB::createGTFSCalendarTable()
{
    //service_id,monday,tuesday,wednesday,thursday,friday,saturday,sunday,start_date,end_date
    //1,1,1,1,1,1,0,0,20140831,20150207
    makeDBCall("CREATE TABLE gtfs_calendar_data (ID int NOT NULL AUTO_INCREMENT, PRIMARY KEY(ID),\
        service_id int,\
            monday int,\
            tuesday int,\
            wednesday int,\
            thursday int,\
            friday int,\
            saturday int,\
            sunday int,\
            start_date date,\
            end_date date);");
}

void MysqlDB::insertGTFSCalendar(vector<string> &strings)
{
    if(strings.size() != 10)
    {
        printf("MysqlDB::insertGTFSRoute: token amount not 10! \n");
        return;
    }

    string item_names;
    string item_vals;
    int i=0;

    //service_id,monday,tuesday,wednesday,thursday,friday,saturday,sunday,start_date,end_date
    //1,1,1,1,1,1,0,0,20140831,20150207

    item_names += "service_id, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "monday, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "tuesday, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "wednesday, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "thursday, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "friday, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "saturday, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "sunday, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "start_date, ";
    item_vals += "'" + strings[i++] + "', ";

    item_names += "end_date";
    item_vals += "'" + escapeStr(strings[i++]) + "'";

    makeDBCall("INSERT INTO gtfs_calendar_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::insertGTFSRoute(vector<string> &strings)
{
    if(strings.size() != 7)
    {
        printf("MysqlDB::insertGTFSRoute: token amount not 7! \n");
        return;
    }

    string item_names;
    string item_vals;

    //route_id,route_short_name,route_long_name,route_type,route_color,route_text_color,route_url
    //13028,1,Parx Casino to 54th-City,3,,,

    item_names += "route_id, ";
    item_vals += "'" + strings[0] + "', ";

    item_names += "route_short_name, ";
    item_vals += "'" + escapeStr(strings[1]) + "', ";

    item_names += "route_long_name, ";
    item_vals += "'" + escapeStr(strings[2]) + "', ";

    item_names += "route_type, ";
    item_vals += "'" + strings[3] + "', ";

    item_names += "route_color, ";
    item_vals += "'" + escapeStr(strings[4]) + "', ";

    item_names += "route_text_color, ";
    item_vals += "'" + escapeStr(strings[5]) + "', ";

    item_names += "route_url";
    item_vals += "'" + escapeStr(strings[6]) + "'";

    makeDBCall("INSERT INTO gtfs_routes_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::insertGTFSStop(vector<string> &strings)
{
    if(strings.size() != 8)
    {
        printf("MysqlDB::insertGTFSStop: token amount not 8! \n");
        return;
    }

    string item_names;
    string item_vals;

    //stop_id,stop_name,stop_lat,stop_lon,location_type,parent_station,zone_id,wheelchair_boarding
    //2,Ridge Av & Lincoln Dr  - FS,40.014986,-75.206826,,31032,1,0

    item_names += "stop_id, ";
    item_vals += "'" + strings[0] + "', ";

    item_names += "stop_name, ";
    item_vals += "'" + escapeStr(strings[1]) + "', ";

    item_names += "stop_lat, ";
    item_vals += "'" + strings[2] + "', ";

    item_names += "stop_lon, ";
    item_vals += "'" + strings[3] + "', ";

    item_names += "location_type, ";
    item_vals += "'" + strings[4] + "', ";

    item_names += "parent_station, ";
    item_vals += "'" + strings[5] + "', ";

    item_names += "zone_id, ";
    item_vals += "'" + strings[6] + "', ";

    item_names += "wheelchair_boarding";
    item_vals += "'" + strings[7] + "'";

    makeDBCall("INSERT INTO gtfs_stops_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::insertGTFSStopTime(vector<string> &strings)
{
    if(strings.size() != 5)
    {
        printf("MysqlDB::insertGTFSStopTime: token amount not 5! \n");
        return;
    }

    string item_names;
    string item_vals;

    //trip_id,arrival_time,departure_time,stop_id,stop_sequence
    //4120486,13:35:00,13:35:00,1874,1

    item_names += "trip_id, ";
    item_vals += "'" + strings[0] + "', ";

    item_names += "arrival_time, ";
    item_vals += "'" + strings[1] + "', ";

    item_names += "departure_time, ";
    item_vals += "'" + strings[2] + "', ";

    item_names += "stop_id, ";
    item_vals += "'" + strings[3] + "', ";

    item_names += "stop_sequence";
    item_vals += "'" + strings[4] + "'";

    makeDBCall("INSERT INTO gtfs_stop_times_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::insertGTFSTrip(vector<string> &strings)
{
    if(strings.size() != 7)
    {
        printf("MysqlDB::insertGTFSTrip: token amount not 7! \n");
        return;
    }

    string trip_headsign = strings[3].substr(1, strings[3].length()-2);
    string item_names;
    string item_vals;

    //route_id,service_id,trip_id,trip_headsign,block_id,direction_id,shape_id
    //12966,1,4165438,"204 Eagleview",8605,0,162669

    item_names += "route_id, ";
    item_vals += "'" + strings[0] + "', ";

    item_names += "service_id, ";
    item_vals += "'" + strings[1] + "', ";

    item_names += "trip_id, ";
    item_vals += "'" + strings[2] + "', ";

    item_names += "trip_headsign, ";
    item_vals += "'" + escapeStr(trip_headsign) + "', ";

    item_names += "block_id, ";
    item_vals += "'" + strings[4] + "', ";

    item_names += "direction_id, ";
    item_vals += "'" + strings[5] + "', ";

    item_names += "shape_id";
    item_vals += "'" + strings[6] + "'";

    makeDBCall("INSERT INTO gtfs_trips_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::insertCoeff(string route_id, int stop_id, vector<double> theta_results, vector<double> mu_results, vector<double> sigma_results)
{
    deleteCoeff(route_id, stop_id);

    if(!theta_results.size()) { printf("MysqlDB::insertCoeff: !theta_results.size()\n"); return; }
    if(mu_results.size()+1 != theta_results.size()) { printf("MysqlDB::insertCoeff: mu_results.size()+1 != theta_results.size()\n"); return; }
    if(sigma_results.size()+1 != theta_results.size()) { printf("MysqlDB::insertCoeff: sigma_results.size()+1 != theta_results.size()\n"); return; }

    insertCoeff(route_id, stop_id, theta_results[0], 0, 0, 0);

    for(int i=0;i<mu_results.size();i++)
        insertCoeff(route_id, stop_id, theta_results[i+1], mu_results[i], sigma_results[i], i+1);
}

void MysqlDB::insertCoeff(string &route_id, int stop_id, double theta, double mu, double sigma, int co_num)
{
    makeDBCall("INSERT INTO coeff_data ( route_id, stop_id, theta, mu, sigma, co_num ) VALUES ( '" + 
            escapeStr(route_id) + "', '" + 
            to_string(stop_id) + "', '" + 
            to_stringe(theta) + "', '" + 
            to_stringe(mu) + "', '" + 
            to_stringe(sigma) + "', '" + 
            to_string(co_num) + "' );");
}

void MysqlDB::deleteCoeff(string route_id, int stop_id)
{
    makeDBCall("DELETE FROM coeff_data WHERE route_id='" + escapeStr(route_id) + "' AND stop_id='" + to_string(stop_id) +  "'");
}

void MysqlDB::deleteGoogDirFrontEnd(string route_id)
{
    makeDBCall("DELETE FROM googdir_data_front_end WHERE route_id='" + escapeStr(route_id) + "'");
}

void MysqlDB::deleteStopRoute(string route_id)
{
    makeDBCall("DELETE FROM stop_data WHERE route_id='" + escapeStr(route_id) + "'");
}

void MysqlDB::insertPrediction(int bus_id, int stop_id, double prediction)
{
    makeDBCall("INSERT INTO prediction_data ( bus_id, stop_id, prediction, timestamp ) VALUES ( '" + 
            to_string(bus_id) + "', '" + 
            to_string(stop_id) + "', '" + 
            to_string(prediction) + "', '" + 
            to_string(time(0)) + "' );");
}

void MysqlDB::insertRoutePerformance(string route_id, int stop_id, RoutePerfObject &perf_obj)
{
    string item_names;
    string item_vals;

    item_names += "route_id, ";
    item_vals += "'" + escapeStr(route_id) + "', ";

    item_names += "stop_id, ";
    item_vals += "'" + to_string(stop_id) + "', ";

    item_names += "data_points, ";
    item_vals += "'" + to_string(perf_obj.data_points) + "', ";

    item_names += "data_trips, ";
    item_vals += "'" + to_string(perf_obj.data_trips) + "', ";

    item_names += "used_features, ";
    item_vals += "'" + to_string(perf_obj.used_features) + "', ";

    item_names += "total_features, ";
    item_vals += "'" + to_string(perf_obj.total_features) + "', ";

    item_names += "data_points_discarded, ";
    item_vals += "'" + to_string(perf_obj.data_points_discarded) + "', ";

    item_names += "data_trips_discarded, ";
    item_vals += "'" + to_string(perf_obj.data_trips_discarded) + "', ";

    item_names += "discarded_bad_block_id, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_block_id) + "', ";

    item_names += "discarded_bad_dest, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_dest) + "', ";

    item_names += "discarded_bad_dir, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_dir) + "', ";

    item_names += "discarded_bad_dir_id, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_dir_id) + "', ";

    item_names += "discarded_bad_stops_away, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_stops_away) + "', ";

    item_names += "discarded_bad_node_times, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_node_times) + "', ";

    item_names += "discarded_bad_block_id_in, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_block_id_in) + "', ";

    item_names += "discarded_bad_weather_googdir, ";
    item_vals += "'" + to_string(perf_obj.discarded_bad_weather_googdir) + "', ";

    item_names += "iterations, ";
    item_vals += "'" + to_string(perf_obj.iterations) + "', ";

    item_names += "train_time, ";
    item_vals += "'" + to_string(perf_obj.train_time) + "', ";

    item_names += "mean_squared_error, ";
    item_vals += "'" + to_string(perf_obj.mean_squared_error) + "', ";

    for(int i=0;i<7;i++)
    {
        item_names += "day_count_" + to_string(i) + ", ";
        item_vals += "'" + to_string(perf_obj.day_count[i]) + "', ";
    }

    for(int i=0;i<24;i++)
    {
        item_names += "hour_count_" + to_string(i) + ", ";
        item_vals += "'" + to_string(perf_obj.hour_count[i]) + "', ";
    }

    item_names += "timestamp";
    item_vals += "'" + to_string(perf_obj.timestamp) + "'";

    makeDBCall("INSERT INTO route_perf_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::insertRouteApplyModel(string route_id, int stop_id, int ram_num, double actual, double predicted, int timestamp)
{
    makeDBCall("INSERT INTO route_apply_model_data ( route_id, stop_id, ram_num, actual, predicted, timestamp ) VALUES ( '" + 
            escapeStr(route_id) + "', '" + 
            to_string(stop_id) + "', '" + 
            to_string(ram_num) + "', '" + 
            to_string(actual) + "', '" + 
            to_string(predicted) + "', '" + 
            to_string(timestamp) + "' );");
}

void MysqlDB::insertRouteCostGraph(string route_id, int stop_id, vector<double> &cost_graph, int timestamp)
{
    for(int i=0;i<cost_graph.size();i++)
    {
        makeDBCall("INSERT INTO route_cost_graph_data ( route_id, stop_id, rcg_num, cost, timestamp ) VALUES ( '" + 
                escapeStr(route_id) + "', '" + 
                to_string(stop_id) + "', '" + 
                to_string(i) + "', '" + 
                to_string(cost_graph[i]) + "', '" + 
                to_string(timestamp) + "' );");
    }
}

void MysqlDB::insertRouteBlockIdTrips(string route_id, int stop_id, map<int, int> &perf_block_id_trips, map<int, int> &perf_block_id_discarded_trips, map<int, bool> &perf_block_id_in, int timestamp)
{
    for(auto iter=perf_block_id_trips.begin();iter!=perf_block_id_trips.end();iter++)
    {
        makeDBCall("INSERT INTO route_block_id_trips_data ( route_id, stop_id, block_id, trips, trips_discarded, block_id_in, timestamp ) VALUES ( '" + 
                escapeStr(route_id) + "', '" + 
                to_string(stop_id) + "', '" + 
                to_string(iter->first) + "', '" + 
                to_string(iter->second) + "', '" + 
                to_string(perf_block_id_discarded_trips[iter->first]) + "', '" + 
                to_string(perf_block_id_in[iter->first]) + "', '" + 
                to_string(timestamp) + "' );");
    }
}

void MysqlDB::deleteRouteBlockIdTrips(string route_id, int stop_id)
{
    makeDBCall("DELETE FROM route_block_id_trips_data WHERE route_id='" + escapeStr(route_id) + "' AND stop_id='" + to_string(stop_id) +  "'");
}

void MysqlDB::deleteRouteCostGraph(string route_id, int stop_id)
{
    makeDBCall("DELETE FROM route_cost_graph_data WHERE route_id='" + escapeStr(route_id) + "' AND stop_id='" + to_string(stop_id) +  "'");
}

void MysqlDB::deleteRouteApplyModel(string route_id, int stop_id)
{
    makeDBCall("DELETE FROM route_apply_model_data WHERE route_id='" + escapeStr(route_id) + "' AND stop_id='" + to_string(stop_id) +  "'");
}

void MysqlDB::deleteRoutePerformance(string route_id, int stop_id)
{
    makeDBCall("DELETE FROM route_perf_data WHERE route_id='" + escapeStr(route_id) + "' AND stop_id='" + to_string(stop_id) +  "'");
}

void MysqlDB::deletePrediction(int bus_id, int stop_id)
{
    makeDBCall("DELETE FROM prediction_data WHERE bus_id='" + to_string(bus_id) + "' AND stop_id='" + to_string(stop_id) +  "'");
}

bool MysqlDB::insertWeather(WeatherObject &obj, bool to_front_end)
{
    string item_names;
    string item_vals;

    item_names += "cloudcover, ";
    item_vals += "'" + to_string(obj.cloudcover) + "', ";

    item_names += "humidity, ";
    item_vals += "'" + to_string(obj.humidity) + "', ";

    item_names += "precipMM, ";
    item_vals += "'" + to_stringe(obj.precipMM) + "', ";

    item_names += "pressure, ";
    item_vals += "'" + to_string(obj.pressure) + "', ";

    item_names += "temp_C, ";
    item_vals += "'" + to_string(obj.temp_C) + "', ";

    item_names += "temp_F, ";
    item_vals += "'" + to_string(obj.temp_F) + "', ";

    item_names += "visibility, ";
    item_vals += "'" + to_string(obj.visibility) + "', ";

    item_names += "weatherCode, ";
    item_vals += "'" + to_string(obj.weatherCode) + "', ";

    item_names += "weatherDesc, ";
    item_vals += "'" + escapeStr(obj.weatherDesc) + "', ";

    item_names += "winddir16Point, ";
    item_vals += "'" + escapeStr(obj.winddir16Point) + "', ";

    item_names += "winddirDegree, ";
    item_vals += "'" + to_string(obj.winddirDegree) + "', ";

    item_names += "windspeedKmph, ";
    item_vals += "'" + to_string(obj.windspeedKmph) + "', ";

    item_names += "windspeedMiles, ";
    item_vals += "'" + to_string(obj.windspeedMiles) + "', ";

    item_names += "w_precipMM, ";
    item_vals += "'" + to_stringe(obj.w_precipMM) + "', ";

    item_names += "w_tempMaxC, ";
    item_vals += "'" + to_string(obj.w_tempMaxC) + "', ";

    item_names += "w_tempMaxF, ";
    item_vals += "'" + to_string(obj.w_tempMaxF) + "', ";

    item_names += "w_tempMinC, ";
    item_vals += "'" + to_string(obj.w_tempMinC) + "', ";

    item_names += "w_tempMinF, ";
    item_vals += "'" + to_string(obj.w_tempMinF) + "', ";

    item_names += "w_weatherCode, ";
    item_vals += "'" + to_string(obj.w_weatherCode) + "', ";

    item_names += "w_weatherDesc, ";
    item_vals += "'" + escapeStr(obj.w_weatherDesc) + "', ";

    item_names += "w_winddir16Point, ";
    item_vals += "'" + escapeStr(obj.w_winddir16Point) + "', ";

    item_names += "w_winddirDegree, ";
    item_vals += "'" + to_string(obj.w_winddirDegree) + "', ";

    item_names += "w_winddirection, ";
    item_vals += "'" + escapeStr(obj.w_winddirection) + "', ";

    item_names += "w_windspeedKmph, ";
    item_vals += "'" + to_string(obj.w_windspeedKmph) + "', ";

    item_names += "w_windspeedMiles, ";
    item_vals += "'" + to_string(obj.w_windspeedMiles) + "', ";

    item_names += "timestamp";
    item_vals += "'" + to_string(obj.timestamp) + "'";

    if(to_front_end)
        return makeDBCall("INSERT INTO weather_data_front_end (" + item_names + " ) VALUES ( " + item_vals + " );");
    else
        return makeDBCall("INSERT INTO weather_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

bool MysqlDB::insertWeatherFrontEnd(WeatherObject &obj)
{
    if(!clearWeatherFrontEnd()) return false;

    if(!insertWeather(obj, true)) return false;

    return true;
}

bool MysqlDB::clearWeatherFrontEnd()
{
    return makeDBCall("TRUNCATE weather_data_front_end;");
}

bool MysqlDB::clearBusFrontEnd()
{
    return makeDBCall("TRUNCATE bus_data_front_end;");
}

bool MysqlDB::clearPredictions()
{
    return makeDBCall("TRUNCATE prediction_data;");
}

bool MysqlDB::insertBusData(BusObject &obj, bool to_front_end)
{
    string item_names;
    string item_vals;
    string table_name;

    item_names += "lat, ";
    item_vals += "'" + to_string(obj.lat) + "', ";

    item_names += "lng, ";
    item_vals += "'" + to_string(obj.lng) + "', ";

    item_names += "label, ";
    item_vals += "'" + to_string(obj.label) + "', ";

    item_names += "vehicle_id, ";
    item_vals += "'" + to_string(obj.vehicle_id) + "', ";

    item_names += "block_id, ";
    item_vals += "'" + to_string(obj.block_id) + "', ";

    item_names += "direction, ";
    item_vals += "'" + escapeStr(obj.direction) + "', ";

    item_names += "destination, ";
    item_vals += "'" + escapeStr(obj.destination) + "', ";

    item_names += "offset, ";
    item_vals += "'" + to_string(obj.offset) + "', ";

    item_names += "offset_sec, ";
    item_vals += "'" + to_string(obj.offset_sec) + "', ";

    item_names += "weather_timestamp, ";
    item_vals += "'" + to_string(obj.weather_timestamp) + "', ";

    item_names += "googdir_timestamp, ";
    item_vals += "'" + to_string(obj.googdir_timestamp) + "', ";

    item_names += "route_id, ";
    item_vals += "'" + escapeStr(obj.route_id) + "', ";

    item_names += "timestamp, ";
    item_vals += "'" + to_string(obj.timestamp) + "', ";

    item_names += "bus_count";
    item_vals += "'" + to_string(obj.bus_count) + "'";

    if(to_front_end)
        table_name = "bus_data_front_end";
    else
        table_name = "bus_data";

    return makeDBCall("INSERT INTO " + table_name + " (" + item_names + " ) VALUES ( " + item_vals + " );");
}

bool MysqlDB::insertGoogDirData(string route_id, vector<GoogDirObject> &node_times, bool to_front_end)
{
    if(to_front_end) deleteGoogDirFrontEnd(route_id);

    for(int i=0;i<node_times.size();i++)
        insertGoogDirData(route_id, i, node_times[i], to_front_end);

    return true;
}

bool MysqlDB::insertGoogDirData(string route_id, int gd_num, GoogDirObject &obj, bool to_front_end)
{
    string item_names;
    string item_vals;
    string table_name;

    item_names += "route_id, ";
    item_vals += "'" + escapeStr(route_id) + "', ";

    item_names += "gd_num, ";
    item_vals += "'" + to_string(gd_num) + "', ";

    item_names += "distance, ";
    item_vals += "'" + to_string(obj.distance) + "', ";

    item_names += "duration, ";
    item_vals += "'" + to_string(obj.duration) + "', ";

    item_names += "steps, ";
    item_vals += "'" + to_string(obj.steps) + "', ";

    item_names += "timestamp";
    item_vals += "'" + to_string(obj.timestamp) + "'";

    if(to_front_end)
        table_name = "googdir_data_front_end";
    else
        table_name = "googdir_data";

    return makeDBCall("INSERT INTO " + table_name + " (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::insertStopData(StopObject &obj)
{
    string item_names;
    string item_vals;

    item_names += "lat, ";
    item_vals += "'" + to_string(obj.lat) + "', ";

    item_names += "lng, ";
    item_vals += "'" + to_string(obj.lng) + "', ";

    item_names += "stop_id, ";
    item_vals += "'" + to_string(obj.stop_id) + "', ";

    item_names += "stopname, ";
    item_vals += "'" + escapeStr(obj.stopname) + "', ";

    item_names += "route_id";
    item_vals += "'" + escapeStr(obj.route_id) + "'";

    makeDBCall("INSERT INTO stop_data (" + item_names + " ) VALUES ( " + item_vals + " );");
}

void MysqlDB::createGTFSStopTimesIndexes()
{
    //needed to speed up getting all the stops on a route
    makeDBCall("CREATE INDEX gtfs_stop_times_data_ti USING BTREE ON gtfs_stop_times_data (trip_id);");

    //needed to speed up getting all the destinations for a stop id
    makeDBCall("CREATE INDEX gtfs_stop_times_data_si USING BTREE ON gtfs_stop_times_data (stop_id);");
}

void MysqlDB::createGTFSTripsIndexes()
{
    //needed to speed up getting all the destinations for a stop id
    makeDBCall("CREATE INDEX gtfs_trips_data_ti USING BTREE ON gtfs_trips_data (trip_id);");

    //needed to speed up getting all the routes for a stop
    makeDBCall("CREATE INDEX gtfs_trips_data_ri USING BTREE ON gtfs_routes_data (route_id);");
}

void MysqlDB::createGTFSStopsIndexes()
{
    //needed to speed up getting all the stops on a route
    makeDBCall("CREATE INDEX gtfs_stops_data_si USING BTREE ON gtfs_stops_data (stop_id);");
}

void MysqlDB::createGTFSRoutesIndexes()
{
    //needed to speed up getting all the routes for a stop
    makeDBCall("CREATE INDEX gtfs_routes_data_ri USING BTREE ON gtfs_routes_data (route_id);");
}

string MysqlDB::escapeStr(string &str)
{
    static char *buf = NULL;
    static int size = 0;

    if(!c_obj) return "";
    if(!str.size()) return "";

    if(str.size() * 2 + 1 > size)
    {
        size = str.size() * 2 + 1;

        if(buf) free(buf);
        buf = (char*)malloc(size);
    }

    mysql_real_escape_string(c_obj, buf, str.c_str(), str.size());

    return buf;
}

void MysqlDB::doDisconnect()
{
    if(c_obj) mysql_close(c_obj);
    c_obj = NULL;
}

bool MysqlDB::makeDBCall(string query)
{
    if(!c_obj) return false;

    int ret = mysql_query(c_obj, query.c_str());

    if(!ret)
    {
        //printf("MysqlDB::makeDBCall:success\n");
        return true;
    }
    else
    {
        printf("MysqlDB::makeDBCall:failed: '%s...' %d:'%s'\n", query.substr(0,20).c_str(), ret, mysql_error(c_obj));
        return false;
    }
}

vector<BusObject> MysqlDB::getBusData(string route_id)
{
    vector<BusObject> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;
    int num_fields;

    //only pull the weather variables we're interested in training against
    //makeDBCall("SELECT * FROM bus_data WHERE route_id='" + escapeStr(route_id) + "' ORDER BY timestamp ASC;");
    makeDBCall("SELECT b.*, cloudcover, humidity, precipMM, temp_F, visibility, weatherCode, windspeedKmph, w_tempMinF, w_tempMaxF \
            FROM bus_data as b, weather_data as w \
            WHERE b.weather_timestamp = w.timestamp \
            AND route_id='" + escapeStr(route_id) + "' \
            ORDER BY timestamp ASC;");

    res=mysql_store_result(c_obj);

    /*
       num_fields = mysql_num_fields(res);
       fields = mysql_fetch_fields(res);
       for(int i=0;i<num_fields;i++)
       printf("MysqlDB::getBusData - '%s' \n", fields[i].name);

       MysqlDB::getBusData - 'ID' 
       MysqlDB::getBusData - 'lat' 
       MysqlDB::getBusData - 'lng' 
       MysqlDB::getBusData - 'label' 
       MysqlDB::getBusData - 'vehicle_id' 
       MysqlDB::getBusData - 'block_id' 
       MysqlDB::getBusData - 'direction' 
       MysqlDB::getBusData - 'destination' 
       MysqlDB::getBusData - 'offset' 
       MysqlDB::getBusData - 'weather_code' 
       MysqlDB::getBusData - 'weather_desc' 
       MysqlDB::getBusData - 'route_id' 
       MysqlDB::getBusData - 'timestamp'
       */

    while ((row = mysql_fetch_row(res)))
    {
        BusObject new_obj;
        int i = 0;

        new_obj.ID = atoi(row[i++]);
        new_obj.lat = atof(row[i++]);
        new_obj.lng = atof(row[i++]);
        new_obj.label = atoi(row[i++]);
        new_obj.vehicle_id = atoi(row[i++]);
        new_obj.block_id = atoi(row[i++]);
        new_obj.direction = row[i++];
        new_obj.destination = row[i++];
        new_obj.offset = atoi(row[i++]);
        new_obj.offset_sec = atoi(row[i++]);
        new_obj.weather_timestamp = atoi(row[i++]);
        new_obj.googdir_timestamp = atoi(row[i++]);
        new_obj.route_id = row[i++];
        new_obj.timestamp = atoi(row[i++]);
        new_obj.bus_count = atoi(row[i++]);

        new_obj.w_obj.cloudcover = atoi(row[i++]);
        new_obj.w_obj.humidity = atoi(row[i++]);
        new_obj.w_obj.precipMM = atof(row[i++]);
        new_obj.w_obj.temp_F = atoi(row[i++]);
        new_obj.w_obj.visibility = atoi(row[i++]);
        new_obj.w_obj.weatherCode = atoi(row[i++]);
        new_obj.w_obj.windspeedKmph = atoi(row[i++]);
        new_obj.w_obj.w_tempMinF = atoi(row[i++]);
        new_obj.w_obj.w_tempMaxF = atoi(row[i++]);
        new_obj.w_obj.timestamp = new_obj.weather_timestamp;

        new_obj.node_times = getGoogDirData(new_obj.route_id, new_obj.googdir_timestamp);

        //new_obj.debug();

        ret_vec.push_back(new_obj);
    }

    mysql_free_result(res);

    return ret_vec;
}

vector<StopObject> MysqlDB::getStopData(string route_id)
{
    vector<StopObject> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;
    int num_fields;

    makeDBCall("SELECT * FROM stop_data WHERE route_id='" + escapeStr(route_id) + "';");

    res=mysql_store_result(c_obj);

    /*
       num_fields = mysql_num_fields(res);
       fields = mysql_fetch_fields(res);
       for(int i=0;i<num_fields;i++)
       printf("MysqlDB::getStopData - '%s' \n", fields[i].name);

       MysqlDB::getStopData - 'ID' 
       MysqlDB::getStopData - 'lat' 
       MysqlDB::getStopData - 'lng' 
       MysqlDB::getStopData - 'stop_id' 
       MysqlDB::getStopData - 'stopname' 
       MysqlDB::getStopData - 'route_id'
       */

    while ((row = mysql_fetch_row(res)))
    {
        StopObject new_obj;

        new_obj.ID = atoi(row[0]);
        new_obj.lat = atof(row[1]);
        new_obj.lng = atof(row[2]);
        new_obj.stop_id = atoi(row[3]);
        new_obj.stopname = row[4];
        new_obj.route_id = row[5];

        ret_vec.push_back(new_obj);
    }

    mysql_free_result(res);

    return ret_vec;
}

vector<CoeffObject> MysqlDB::getCoeffData(string route_id, int stop_id)
{
    vector<CoeffObject> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;
    int num_fields;

    makeDBCall("SELECT theta, mu, sigma FROM coeff_data WHERE route_id='" +
            escapeStr(route_id) + "' AND stop_id='" + to_string(stop_id) + "' ORDER BY co_num");

    res=mysql_store_result(c_obj);

    /*
       num_fields = mysql_num_fields(res);
       fields = mysql_fetch_fields(res);
       for(int i=0;i<num_fields;i++)
       printf("MysqlDB::getCoeffData - '%s' \n", fields[i].name);

       MysqlDB::getCoeffData - 'theta' 
       MysqlDB::getCoeffData - 'mu' 
       MysqlDB::getCoeffData - 'sigma' 
       */
    while ((row = mysql_fetch_row(res)))
    {
        CoeffObject new_obj;

        new_obj.theta = atof(row[0]);
        new_obj.mu = atof(row[1]);
        new_obj.sigma = atof(row[2]);

        ret_vec.push_back(new_obj);
    }

    mysql_free_result(res);

    return ret_vec;
}

vector<GoogDirObject> MysqlDB::getGoogDirData(string route_id, int timestamp, bool use_cache)
{
    static map< string, map< int, vector< GoogDirObject > > > cache;

    if(use_cache)
    {
        //in the cache?
        map< string, map< int, vector< GoogDirObject > > >::iterator it = cache.find(route_id);
        if(it != cache.end())
        {
            map< int, vector< GoogDirObject > >::iterator it2 = it->second.find(timestamp);

            if(it2 != it->second.end())
                return it2->second;
        }
    }

    //get it from mysql
    {
        vector<GoogDirObject> ret_vec;
        MYSQL_RES *res;
        MYSQL_ROW row;
        MYSQL_FIELD *fields;
        int num_fields;

        makeDBCall("SELECT duration, distance, steps, timestamp \
                FROM septa_next_bus_db.googdir_data \
                WHERE route_id='" + escapeStr(route_id) + "' \
                AND timestamp=" + to_string(timestamp) + " \
                ORDER BY gd_num;");

        res=mysql_store_result(c_obj);

        /*
           num_fields = mysql_num_fields(res);
           fields = mysql_fetch_fields(res);
           for(int i=0;i<num_fields;i++)
           printf("MysqlDB::getCoeffData - '%s' \n", fields[i].name);

           MysqlDB::getCoeffData - 'theta' 
           MysqlDB::getCoeffData - 'mu' 
           MysqlDB::getCoeffData - 'sigma' 
           */
        while ((row = mysql_fetch_row(res)))
        {
            GoogDirObject new_obj;

            new_obj.duration = atoi(row[0]);
            new_obj.distance = atoi(row[1]);
            new_obj.steps = atoi(row[2]);
            new_obj.timestamp = atoi(row[3]);

            ret_vec.push_back(new_obj);
        }

        mysql_free_result(res);

        cache[route_id][timestamp] = ret_vec;
    }

    return cache[route_id][timestamp];
}

vector<GoogDirObject> MysqlDB::getGoogDirDataFrontEnd(string route_id)
{
    vector<GoogDirObject> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT duration, distance, steps, timestamp \
            FROM googdir_data_front_end \
            WHERE route_id='" + escapeStr(route_id) + "' \
            ORDER BY timestamp DESC, gd_num;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        GoogDirObject new_obj;

        new_obj.duration = atoi(row[0]);
        new_obj.distance = atoi(row[1]);
        new_obj.steps = atoi(row[2]);
        new_obj.timestamp = atoi(row[3]);

        //not the same as the other timestamps?
        if(ret_vec.size() && ret_vec[0].timestamp != new_obj.timestamp)
        {
            printf("MysqlDB::getGoogDirDataFrontEnd: more than one timestamp found!!\n");

            mysql_free_result(res);

            return ret_vec;
        }

        ret_vec.push_back(new_obj);
    }

    mysql_free_result(res);

    return ret_vec;
}

RoutePerfObject MysqlDB::getRoutePerformance(string route_id, int stop_id)
{
    RoutePerfObject perf_obj;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT * FROM route_perf_data \
            WHERE route_id='" + escapeStr(route_id) + "' \
            AND stop_id=" + to_string(stop_id) + " \
            ORDER BY timestamp DESC \
            LIMIT 1;");

    res=mysql_store_result(c_obj);

    if (row = mysql_fetch_row(res))
    {
        int i=0;

        i++; //route_id
        i++; //stop_id
        perf_obj.data_points = atoi(row[i++]);
        perf_obj.data_trips = atoi(row[i++]);
        perf_obj.used_features = atoi(row[i++]);
        perf_obj.total_features = atoi(row[i++]);
        perf_obj.data_points_discarded = atoi(row[i++]);
        perf_obj.data_trips_discarded = atoi(row[i++]);
        perf_obj.discarded_bad_block_id = atoi(row[i++]);
        perf_obj.discarded_bad_dest = atoi(row[i++]);
        perf_obj.discarded_bad_dir = atoi(row[i++]);
        perf_obj.discarded_bad_dir_id = atoi(row[i++]);
        perf_obj.discarded_bad_stops_away = atoi(row[i++]);
        perf_obj.discarded_bad_node_times = atoi(row[i++]);
        perf_obj.discarded_bad_block_id_in = atoi(row[i++]);
        perf_obj.discarded_bad_weather_googdir = atoi(row[i++]);
        perf_obj.iterations = atoi(row[i++]);
        perf_obj.train_time = atof(row[i++]);
        perf_obj.mean_squared_error = atof(row[i++]);

        for(int j=0;j<7;j++) perf_obj.day_count[j] = atof(row[i++]);
        for(int j=0;j<24;j++) perf_obj.hour_count[j] = atof(row[i++]);

        perf_obj.timestamp = atoi(row[i++]);
    }

    mysql_free_result(res);

    return perf_obj;
}

WeatherObject MysqlDB::getWeatherFrontEnd()
{
    WeatherObject w_obj;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT * FROM weather_data_front_end \
            ORDER BY timestamp DESC \
            LIMIT 1;");

    res=mysql_store_result(c_obj);

    if (row = mysql_fetch_row(res))
    {
        int i=0;

        w_obj.timestamp = atoi(row[i++]);
        w_obj.cloudcover = atoi(row[i++]);
        w_obj.humidity = atoi(row[i++]);
        w_obj.precipMM = atof(row[i++]);
        w_obj.pressure = atoi(row[i++]);
        w_obj.temp_C = atoi(row[i++]);
        w_obj.temp_F = atoi(row[i++]);
        w_obj.visibility = atoi(row[i++]);
        w_obj.weatherCode = atoi(row[i++]);
        w_obj.weatherDesc = row[i++];
        w_obj.winddir16Point = row[i++];
        w_obj.winddirDegree = atoi(row[i++]);
        w_obj.windspeedKmph = atoi(row[i++]);
        w_obj.windspeedMiles = atoi(row[i++]);
        w_obj.w_precipMM = atof(row[i++]);
        w_obj.w_tempMaxC = atoi(row[i++]);
        w_obj.w_tempMaxF = atoi(row[i++]);
        w_obj.w_tempMinC = atoi(row[i++]);
        w_obj.w_tempMinF = atoi(row[i++]);
        w_obj.w_weatherCode = atoi(row[i++]);
        w_obj.w_weatherDesc = row[i++];
        w_obj.w_winddir16Point = row[i++];
        w_obj.w_winddirDegree = atoi(row[i++]);
        w_obj.w_winddirection = row[i++];
        w_obj.w_windspeedKmph = atoi(row[i++]);
        w_obj.w_windspeedMiles = atoi(row[i++]);
    }

    mysql_free_result(res);

    return w_obj;
}

vector<string> MysqlDB::getRouteGTFSDests(string route_id)
{
    vector<string> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT DISTINCT trip_headsign \
            FROM gtfs_routes_data AS r, gtfs_trips_data AS t\
            WHERE r.route_id = t.route_id\
            AND route_short_name = '" + escapeStr(route_id) + "'\
            ORDER BY trip_headsign ASC;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        //printf("MysqlDB::getRouteGTFSDests(%s) '%s'\n", route_id.c_str(), row[0]);

        if(strlen(row[0])) ret_vec.push_back(row[0]);
    }

    mysql_free_result(res);

    return ret_vec;
}

vector<int> MysqlDB::getRouteGTFSBlockIDs(string route_id)
{
    vector<int> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT DISTINCT block_id \
            FROM gtfs_routes_data AS r, gtfs_trips_data AS t\
            WHERE r.route_id = t.route_id\
            AND route_short_name = '" + escapeStr(route_id) + "'\
            ORDER BY block_id ASC;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        int block_id = 0;

        if(row[0]) block_id = atoi(row[0]);

        //printf("MysqlDB::getRouteGTFSDests(%s) %d \n", route_id.c_str(), block_id);

        ret_vec.push_back(block_id);
    }

    mysql_free_result(res);

    return ret_vec;
}

vector<StopObject> MysqlDB::getRouteGTFSStops(string route_id)
{
    vector<StopObject> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT s.stop_id,stop_name,stop_lat,stop_lon \
            FROM \
            (SELECT DISTINCT stop_id \
             FROM \
             (SELECT DISTINCT trip_id \
              FROM \
              (SELECT DISTINCT route_id \
               FROM gtfs_routes_data \
               WHERE route_short_name='" + escapeStr(route_id) + "') as r, gtfs_trips_data as t \
              WHERE t.route_id=r.route_id) as t, gtfs_stop_times_data as st \
             WHERE t.trip_id=st.trip_id) as eh, gtfs_stops_data as s \
            WHERE eh.stop_id=s.stop_id \
            ORDER BY stop_id ASC");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        StopObject new_obj;

        if(row[0]) new_obj.stop_id = atoi(row[0]);
        new_obj.stopname = row[1];
        if(row[2]) new_obj.lat = atof(row[2]);
        if(row[3]) new_obj.lng = atof(row[3]);

        ret_vec.push_back(new_obj);
    }

    mysql_free_result(res);

    return ret_vec;
}

vector<StopObject> MysqlDB::getBlockIDOrderedGTFSStops(int block_id, int service_id)
{
    vector<StopObject> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT t.direction_id, t.trip_headsign, s.stop_id, s.stop_lat, s.stop_lon, st.stop_sequence \
            FROM gtfs_trips_data as t, gtfs_stops_data as s, gtfs_stop_times_data as st \
            WHERE t.block_id=" + to_string(block_id) + " \
            AND t.service_id=" + to_string(service_id) + " \
            AND t.trip_id=st.trip_id \
            AND st.stop_id=s.stop_id \
            GROUP BY t.direction_id, t.service_id, st.stop_sequence \
            ORDER BY direction_id, stop_sequence;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        StopObject new_obj;
        int i=0;

        if(row[i]) new_obj.direction_id = atoi(row[i++]);
        i++;
        if(row[i]) new_obj.stop_id = atoi(row[i++]);
        if(row[i]) new_obj.lat = atof(row[i++]);
        if(row[i]) new_obj.lng = atof(row[i++]);

        ret_vec.push_back(new_obj);
    }

    mysql_free_result(res);

    return ret_vec;
}

set<int> MysqlDB::getStopBlockIDs(int stop_id)
{
    set<int> ret_set;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT DISTINCT block_id \
            FROM gtfs_stop_times_data as s, gtfs_trips_data as t \
            WHERE s.trip_id=t.trip_id \
            AND stop_id=" + to_string(stop_id) + ";");


    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        int block_id = 0;

        if(row[0]) block_id = atoi(row[0]);

        //printf("MysqlDB::getStopBlockIDs(%d) %d \n", stop_id, block_id);

        ret_set.insert(block_id);
    }

    mysql_free_result(res);

    return ret_set;
}

set<int> MysqlDB::getRouteStopBlockIDs(string route_id, int stop_id)
{
    set<int> ret_set;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT DISTINCT block_id \
            FROM septa_next_bus_db.gtfs_stop_times_data as s, septa_next_bus_db.gtfs_trips_data as t, septa_next_bus_db.gtfs_routes_data as r \
            WHERE s.trip_id=t.trip_id \
            AND r.route_id=t.route_id \
            AND stop_id=" + to_string(stop_id) + " \
            AND r.route_short_name='" + escapeStr(route_id) + "';");


    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        int block_id = 0;

        if(row[0]) block_id = atoi(row[0]);

        //printf("MysqlDB::getRouteStopBlockIDs(%s, %d) %d \n", route_id.c_str(), stop_id, block_id);

        ret_set.insert(block_id);
    }

    mysql_free_result(res);

    return ret_set;
}

set<string> MysqlDB::getStopDestinations(int stop_id)
{
    set<string> ret_set;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT DISTINCT trip_headsign \
            FROM \
            (SELECT DISTINCT trip_id \
             FROM gtfs_stop_times_data \
             WHERE stop_id=" + to_string(stop_id) + ") as s, gtfs_trips_data as t \
            WHERE s.trip_id=t.trip_id;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        //printf("MysqlDB::getStopDestinations(%d) '%s' \n", stop_id, row[0]);

        if(row[0]) ret_set.insert(row[0]);
    }

    mysql_free_result(res);

    return ret_set;
}

//this is a hack which assumes start_date and end_date can be ignored
//left = day, right = service_id
map<int,int> MysqlDB::getGTFSCalendarMap()
{
    map<int,int> ret_map;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT * FROM gtfs_calendar_data;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        int i=1;
        int service_id = atoi(row[i++]);
        int mon = atoi(row[i++]);
        int tue = atoi(row[i++]);
        int wed = atoi(row[i++]);
        int thu = atoi(row[i++]);
        int fri = atoi(row[i++]);
        int sat = atoi(row[i++]);
        int sun = atoi(row[i++]);

        if(mon) ret_map[1] = service_id;
        if(tue) ret_map[2] = service_id;
        if(wed) ret_map[3] = service_id;
        if(thu) ret_map[4] = service_id;
        if(fri) ret_map[5] = service_id;
        if(sat) ret_map[6] = service_id;
        if(sun) ret_map[0] = service_id;
    }

    //for(int i=0;i<7;i++) printf("MysqlDB::getGTFSCalendarMap ret_map[%d]=%d \n", i, ret_map[i]);

    mysql_free_result(res);

    return ret_map;
}

map<int,map<int,vector<int>>> MysqlDB::getGTFSStopTimes(string route_id, int stop_id)
{
    map<int,map<int,vector<int>>> ret_map;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT block_id, service_id, arrival_time \
            FROM gtfs_stop_times_data as st, gtfs_trips_data as t, gtfs_routes_data AS r \
            WHERE st.trip_id=t.trip_id \
            AND stop_id=" + to_string(stop_id) + "  \
            AND route_short_name = '" + escapeStr(route_id) + "' \
            ORDER BY block_id, service_id, arrival_time;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        if(!row[0]) continue;
        if(!row[1]) continue;
        if(!row[2]) continue;

        int block_id = atoi(row[0]);
        int service_id = atoi(row[1]);
        string time_str = row[2];
        int time_val;

        time_val = atoi(time_str.substr(0,2).c_str()) * 60;
        time_val += atoi(time_str.substr(3,2).c_str());

        ret_map[block_id][service_id].push_back(time_val);

        //printf("MysqlDB::getGTFSStopTimes(rt:%s, st:%d) bl:%d ser:%d '%s' %d \n", route_id.c_str(), stop_id, service_id, block_id, row[2], time_val);
    }

    return ret_map;
}

int MysqlDB::getDirID(string trip_headsign, int block_id)
{
    int ret_val = -1;
    MYSQL_RES *res;
    MYSQL_ROW row;

    makeDBCall("SELECT DISTINCT direction_id \
            FROM gtfs_trips_data as t \
            WHERE t.trip_headsign='" + escapeStr(trip_headsign) + "' \
            AND t.block_id=" + to_string(block_id) + ";");

    res=mysql_store_result(c_obj);

    if((row = mysql_fetch_row(res)) && row[0])
        ret_val = atoi(row[0]);
    else
        printf("MysqlDB::getDirID: could not find trip_headsign:'%s' for block_id:%d \n", trip_headsign.c_str(), block_id);

    return ret_val;
}

vector<string> MysqlDB::getRouteDirDests(string route_id, bool is_directions)
{
    vector<string> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;

    if(is_directions)
        makeDBCall("SELECT DISTINCT direction FROM bus_data WHERE route_id='" + escapeStr(route_id) + "';");
    else
        makeDBCall("SELECT DISTINCT destination FROM bus_data WHERE route_id='" + escapeStr(route_id) + "';");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
    {
        //printf("MysqlDB::getRouteDirDests(%s) '%s'\n", route_id.c_str(), row[0]);

        if(strlen(row[0])) ret_vec.push_back(row[0]);
    }

    if(ret_vec.size() != 2)
    {
        printf("MysqlDB::getRouteDirDests(%s): amount not 2! (%lu) ", route_id.c_str(), ret_vec.size());
        for(int i=0;i<ret_vec.size();i++)
            printf("'%s' ", ret_vec[i].c_str());
        printf("\n");
    }

    mysql_free_result(res);

    return ret_vec;
}

vector<string> MysqlDB::getBusDataRoutes()
{
    vector<string> ret_vec;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;

    makeDBCall("SELECT DISTINCT route_id FROM bus_data ORDER BY route_id;");

    res=mysql_store_result(c_obj);

    while ((row = mysql_fetch_row(res)))
        if(strlen(row[0])) ret_vec.push_back(row[0]);

    if(!ret_vec.size())
        printf("MysqlDB::getBusDataRoutes() no routes found! (bus training data empty?)\n");

    mysql_free_result(res);

    return ret_vec;
}

int MysqlDB::busCountAtTimestamp(string route_id, int timestamp)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *fields;
    int ret_val;

    makeDBCall("SELECT DISTINCT vehicle_id FROM bus_data WHERE route_id='" + escapeStr(route_id) + "' AND timestamp=" + to_string(timestamp) + ";");

    res=mysql_store_result(c_obj);

    ret_val = mysql_num_rows(res);

    mysql_free_result(res);

    return ret_val;
}

void MysqlDB::deleteBusID(int ID)
{
    printf("MysqlDB::deleteBusID(%d)\n", ID);
    makeDBCall("DELETE FROM bus_data WHERE ID='" + to_string(ID) + "'");
}

void MysqlDB::updateBusValue(int ID, string value_name, string value)
{
    makeDBCall("UPDATE bus_data SET " + value_name + "=" + value + " WHERE ID='" + to_string(ID) + "'");
}

bool MysqlDB::tableExists(string table_name)
{
    MYSQL_RES *res;
    int row_num;

    makeDBCall("SHOW TABLES LIKE '" + escapeStr(table_name) + "'");

    res=mysql_store_result(c_obj);

    row_num = mysql_num_rows(res);

    mysql_free_result(res);

    return row_num;
}

void MysqlDB::dropTable(string table_name)
{
    makeDBCall("DROP TABLE " + escapeStr(table_name) + "");
}

