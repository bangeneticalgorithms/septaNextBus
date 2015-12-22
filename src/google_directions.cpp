#include "google_directions.h"
#include "functions.h"
#include "objects.h"
#include "mysql_db.h"
#include "settings.h"
#include <stdio.h>
#include <math.h>

#include <jsoncpp/json/json.h>

GoogDir* GoogDir::instance = NULL;

GoogDir::GoogDir()
{
    to_front_end = false;
    restrict_route_all = true;
}

GoogDir::~GoogDir()
{

}

GoogDir* GoogDir::getInstance()
{
    if(!instance) instance = new GoogDir();

    return instance;
}

void GoogDir::doProcess()
{
    if(!route_list.size())
    {
        printf("GoogDir::doProcess: !route_list.size()\n");
        return;
    }

    MysqlDB *myobj = MysqlDB::getInstance();

    //try all routes if they aren't recent until one fails
    for(set<string>::iterator it=route_list.begin(); it!=route_list.end(); ++it)
    { 
        const string &r = *it;

        //were we told to only do one route?
        if(!restrict_route_all && r != restrict_route_id) continue;

        //does this route need updated? (15 minutes)
        if(!shouldGetFrontEndData(r) && time(0) - getRouteLastTimestamp(r) < GOOGDIR_TIME_REGRAB + (to_front_end ? 0 : 30)) continue;

        printf("GoogDir::doProcess: r:'%s' ... %lu\n", r.c_str(), route_list.size());

        vector<GoogDirObject> node_times;

        //assume failure means we reached our 2 calls per second limit
        bool limit_reached = false;
        if(!getTrafficDataForRoute(r, node_times, limit_reached)) 
        {
            if(limit_reached)
            {
                string old_key = Settings::getGoogDirKey();

                Settings::cycleGoogDirKeys();

                printf("GoogDir::doProcess: limit reached for '%s'! now using '%s'\n", old_key.c_str(), Settings::getGoogDirKey().c_str());

                return;
            }
            else
            {
                printf("GoogDir::doProcess: !getTrafficDataForRoute but !limit_reached\n");
                //continue;
                return;
            }
        }

        if(!node_times.size()) continue;

        myobj->insertGoogDirData(r, node_times, to_front_end);

        last_route_node_times[r] = node_times;
        last_route_timestamp[r] = node_times[0].timestamp;
    }
}

void GoogDir::setRouteID(string route_id, bool route_all)
{
    restrict_route_all = route_all;
    restrict_route_id = route_id;
}

int GoogDir::getRouteLastTimestamp(const string &route_id)
{
    //is the last time currently stored?
    if(last_route_timestamp.find(route_id) == last_route_timestamp.end())
        last_route_timestamp[route_id] = 0;

    return last_route_timestamp[route_id];
}

void GoogDir::makeRouteList(vector<BusObject> &bus_list)
{
    route_list.clear();

    for(int i=0;i<bus_list.size();i++)
    {
        BusObject &b = bus_list[i];

        if(b.bus_count < 1) continue;

        route_list.insert(b.route_id);
    }

    //printf("GoogDir::makeRouteList: route_list[%lu]: ", route_list.size());
    //for(set<string>::iterator it=route_list.begin(); it!=route_list.end(); ++it)
    //	printf("%s, ", it->c_str());
    //printf("\n");
}

bool GoogDir::shouldGetFrontEndData(string route_id)
{
    static map < string, int > last_check_time;

    //don't check if we put into the front end
    if(to_front_end) return false;

    int the_time = time(0);

    //if it's been checked before and less than 15 seconds ago, skip
    if(last_check_time.find(route_id) != last_check_time.end() && abs(the_time - last_check_time[route_id]) < 15)
        return false;

    last_check_time[route_id] = the_time;

    //check if recent exists
    {
        MysqlDB *myobj = MysqlDB::getInstance();

        vector<GoogDirObject> node_times = myobj->getGoogDirDataFrontEnd(route_id);

        if(node_times.size() <= 2) return false;
        if(abs(the_time - node_times[0].timestamp) > 60) return false;
        if(node_times[0].timestamp <= getRouteLastTimestamp(route_id)) return false;

        return true;
    }

    return false;
}

bool GoogDir::getTrafficDataForRoute(string route_id, vector<GoogDirObject> &node_times, bool &limit_reached)
{
    node_times.clear();
    limit_reached = false;

    //check mysql front end?
    if(!to_front_end)
    {
        MysqlDB *myobj = MysqlDB::getInstance();

        node_times = myobj->getGoogDirDataFrontEnd(route_id);

        if(node_times.size() > 2 && abs(time(0) - node_times[0].timestamp) < 120)
        {
            printf("GoogDir::getTrafficDataForRoute: using front_end data: %ld < %d ... node_times.size():%lu\n", abs(time(0) - node_times[0].timestamp), 120, node_times.size());

            return true;
        }
    }

    vector<StopObject> &stops = gtfs_ordered_stop_ids_for_route_id(route_id, 4);

    string call_data;

    if(!makeGoogleCall(stops, call_data)) return false;

    return processCallData(call_data, node_times, limit_reached);
}

bool GoogDir::makeGoogleCall(vector<StopObject> &stops, string &call_data)
{
    if(stops.size() < 2) return false;

    vector<StopObject> use_stops;

    //can use up to 8 waypoints plus the origin/destination
    const int max_wp = 8;

    if(stops.size() <= max_wp+1)
        use_stops = stops;
    else
    {
        //origin stop is the first stop
        use_stops.push_back(stops[0]);

        //add in the waypoints
        for(int i=1;i<=max_wp;i++)
        {
            //printf("GoogDir::makeGoogleCall: adding wp %d \n", (int)round(stops.size() * i / (max_wp+1)));
            use_stops.push_back(stops[(int)round(stops.size() * i / (max_wp+1))]);
        }
    }

    //https://maps.googleapis.com/maps/api/directions/json?origin=39.946959,-75.144466&destination=39.959725,-75.247282&waypoints=39.949303,-75.163657|39.951,-75.177172&sensor=false&key=AIzaSyCCvcJeiCWaLRfJyI2mnZIvy2bbQagRREs
    string url = "https://maps.googleapis.com/maps/api/directions/json";

    //origin and destination
    StopObject &s0 = use_stops[0];
    url += "?origin=" + to_string(s0.lat) + "," + to_string(s0.lng);
    url += "&destination=" + to_string(s0.lat) + "," + to_string(s0.lng);

    //waypoints
    url += "&waypoints=";
    for(int i=1;i<use_stops.size();i++)
    {
        StopObject &s = use_stops[i];

        if(i!=1) url += "|";

        url += to_string(s.lat) + "," + to_string(s.lng);
    }

    //key etc
    url += "&sensor=false&key=" + Settings::getGoogDirKey();

    printf("GoogDir::makeGoogleCall: url:'%s' \n", url.c_str());

    call_data = make_curl_call(url);

    //printf("GoogDir::makeGoogleCall: call_data:'%s' \n", call_data.c_str());

    return call_data.size();
}

bool GoogDir::processCallData(string &call_data, vector<GoogDirObject> &node_times, bool &limit_reached)
{
    Json::Value root;
    Json::Reader reader;

    bool ret = reader.parse( call_data, root );

    if(!ret) return false;

    //root should be an object
    if(!root.isObject()) return false;

    //check if status value is "OK"
    Json::Value &status = root["status"];
    if(status.isNull()) return false;
    if(!status.isString()) return false;
    if(status.asString() != "OK") 
    {
        printf("GoogDir::processCallData: status is not 'OK':'%s'\n", status.asString().c_str());

        if(status.asString() == "OVER_QUERY_LIMIT") limit_reached = true;
        if(status.asString() == "REQUEST_DENIED") limit_reached = true;

        return false;
    }

    //get route object
    Json::Value &routes_array = root["routes"];
    if(routes_array.isNull()) return false;
    if(!routes_array.isArray()) return false;
    if(!routes_array.size()) return false;
    Json::Value &route = routes_array[(unsigned int)0];
    if(route.isNull()) return false;
    if(!route.isObject()) return false;

    //get "legs"
    Json::Value &legs = route["legs"];
    if(legs.isNull()) return false;
    if(!legs.isArray()) return false;

    //collect times etc
    node_times.clear();

    //get timestamp ready
    int timestamp = time(0);

    for(unsigned int i=0;i<legs.size();i++)
    {
        Json::Value &leg_obj = legs[i];

        int duration = 0;
        int steps = 0;
        int distance = 0;

        //check if is an object
        if(leg_obj.isNull()) return false;
        if(!leg_obj.isObject()) return false;

        //get duration value
        Json::Value &duration_obj = leg_obj["duration"];
        if(duration_obj.isNull()) return false;
        if(!duration_obj.isObject()) return false;
        Json::Value &duration_val = duration_obj["value"];
        if(duration_val.isNull()) return false;
        if(!duration_val.isInt()) return false;

        duration = duration_val.asInt();

        //get steps value
        Json::Value &steps_obj = leg_obj["steps"];
        if(steps_obj.isNull()) return false;
        if(!steps_obj.isArray()) return false;

        steps = steps_obj.size();

        //get distance value
        Json::Value &distance_obj = leg_obj["distance"];
        if(distance_obj.isNull()) return false;
        if(!distance_obj.isObject()) return false;
        Json::Value &distance_val = distance_obj["value"];
        if(distance_val.isNull()) return false;
        if(!distance_val.isInt()) return false;

        distance = distance_val.asInt();

        //printf("GoogDir::processCallData: distance:%d duration:%d steps:%d\n", distance, duration, steps);

        GoogDirObject new_node_time;

        new_node_time.distance = distance;
        new_node_time.duration = duration;
        new_node_time.steps = steps;
        new_node_time.timestamp = timestamp;

        node_times.push_back(new_node_time);
    }

    return true;
}

void GoogDir::testRoute(string route_id)
{
    vector<GoogDirObject> node_times;

    bool limit_reached = false;
    bool passed = getTrafficDataForRoute(route_id, node_times, limit_reached);

    printf("GoogDir::testRoute: passed:%d limit_reached:%d \n", passed, limit_reached);

    for(int i=0;i<node_times.size();i++)
        node_times[i].debug();
}

