#ifndef OBJECTS_H
#define OBJECTS_H

#include <string>
#include <jsoncpp/json/json.h>

using namespace std;

class WeatherObject
{
    public:
        WeatherObject() { clear(); }

        WeatherObject &operator=(const WeatherObject &b);

        void clear();
        void debug();

        int cloudcover;
        int humidity;
        double precipMM;
        int pressure;
        int temp_C;
        int temp_F;
        int visibility;
        int weatherCode;
        string weatherDesc;
        string winddir16Point;
        int winddirDegree;
        int windspeedKmph;
        int windspeedMiles;

        double w_precipMM;
        int w_tempMaxC;
        int w_tempMaxF;
        int w_tempMinC;
        int w_tempMinF;
        int w_weatherCode;
        string w_weatherDesc;
        string w_winddir16Point;
        int w_winddirDegree;
        string w_winddirection;
        int w_windspeedKmph;
        int w_windspeedMiles;

        int timestamp;
};

class GoogDirObject
{
    public:
        GoogDirObject() { clear(); }

        void clear();
        void debug();

        GoogDirObject &operator=(const GoogDirObject &o);

        int distance;
        int duration;
        int steps;
        int timestamp;
};

class BusObject
{
    public:
        BusObject() { clear(); }
        BusObject(Json::Value &json_obj, string _route_id, int _timestamp, int _weather_timestamp, int _googdir_timestamp, int _bus_count);

        void clear();
        void debug();

        BusObject &operator=(const BusObject &b);

        int ID;
        int block_id;
        int offset;
        int offset_sec;
        int vehicle_id;
        int label;
        int weather_timestamp;
        int googdir_timestamp;
        int timestamp;
        int bus_count;
        double lat;
        double lng;
        string route_id;
        string direction;
        string destination;
        int direction_id;
        int nearest_stop_id;
        int stops_away;
        double velocity;
        int time_since_prev;
        double dist_since_prev;
        bool lat_gt_prev;
        bool lat_lt_prev;
        bool lng_gt_prev;
        bool lng_lt_prev;

        WeatherObject w_obj;
        vector<GoogDirObject> node_times;
};

class StopObject
{
    public:
        StopObject() { clear(); }
        StopObject(Json::Value &json_obj, string _route_id = "");

        void clear();
        void debug();

        StopObject &operator=(const StopObject &s);

        int ID;
        int stop_id;
        double lat;
        double lng;
        string stopname;
        string route_id;
        int direction_id;
};

class CoeffObject
{
    public:
        CoeffObject() { clear(); }

        void clear();
        void debug();

        CoeffObject &operator=(const CoeffObject &s);

        double theta;
        double mu;
        double sigma;
};

class RoutePerfObject
{
    public:
        RoutePerfObject() { clear(); }

        void clear();
        void debug();

        RoutePerfObject &operator=(const RoutePerfObject &o);

        int data_points;
        int data_trips;
        int used_features;
        int total_features;
        int data_points_discarded;
        int data_trips_discarded;
        int discarded_bad_block_id;
        int discarded_bad_dest;
        int discarded_bad_dir;
        int discarded_bad_dir_id;
        int discarded_bad_stops_away;
        int discarded_bad_node_times;
        int discarded_bad_block_id_in;
        int discarded_bad_weather_googdir;
        int discarded_bad_stop_times;
        int iterations;
        double train_time;
        double mean_squared_error;
        int timestamp;

        int day_count[7];
        int hour_count[24];
};

#endif
