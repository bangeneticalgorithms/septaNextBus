#ifndef MYSQL_DB_H
#define MYSQL_DB_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

class BusObject;
class StopObject;
class CoeffObject;
class WeatherObject;
class GoogDirObject;
class RoutePerfObject;

class MysqlDB
{
    public:
        MysqlDB();
        ~MysqlDB();

        void doConnect();
        void doDisconnect();
        bool makeDBCall(string query);
        void createStopTable();
        void createBusTable(bool is_front_end = false);
        void createCoeffTable();
        void createWeatherTable(bool is_front_end = false);
        void createGoogDirTable(bool is_front_end = false);
        void createPredictionTable();
        void createRoutePerformanceTable();
        void createRouteApplyModelTable();
        void createRouteCostGraphTable();
        void createRouteBlockIdTripsTable();
        void createGTFSTripsTable();
        void createGTFSStopTimesTable();
        void createGTFSStopsTable();
        void createGTFSRoutesTable();
        void createGTFSCalendarTable();
        void insertCoeff(string route_id, int stop_id, vector<double> theta_results, vector<double> mu_results, vector<double> sigma_results);
        void insertCoeff(string &route_id, int stop_id, double theta, double mu, double sigma, int co_num);
        void insertPrediction(int bus_id, int stop_id, double prediction);
        void insertRoutePerformance(string route_id, int stop_id, RoutePerfObject &perf_obj);
        void insertRouteApplyModel(string route_id, int stop_id, int ram_num, double actual, double predicted, int timestamp);
        void insertRouteCostGraph(string route_id, int stop_id, vector<double> &cost_graph, int timestamp);
        void insertRouteBlockIdTrips(string route_id, int stop_id, map<int, int> &perf_block_id_trips, map<int, int> &perf_block_id_discarded_trips, map<int, bool> &perf_block_id_in, int timestamp);
        bool insertWeather(WeatherObject &obj, bool to_front_end = false);
        bool insertWeatherFrontEnd(WeatherObject &obj);
        void deleteGoogDirFrontEnd(string route_id);
        void deleteCoeff(string route_id, int stop_id);
        void deleteRoutePerformance(string route_id, int stop_id);
        void deleteRouteApplyModel(string route_id, int stop_id);
        void deleteRouteCostGraph(string route_id, int stop_id);
        void deleteRouteBlockIdTrips(string route_id, int stop_id);
        void deletePrediction(int bus_id, int stop_id);
        bool insertBusData(BusObject &obj, bool to_front_end = false);
        bool insertGoogDirData(string route_id, vector<GoogDirObject> &node_times, bool to_front_end = false);
        bool insertGoogDirData(string route_id, int gd_num, GoogDirObject &obj, bool to_front_end = false);
        void insertStopData(StopObject &obj);
        void insertGTFSTrip(vector<string> &strings);
        void insertGTFSStopTime(vector<string> &strings);
        void insertGTFSStop(vector<string> &strings);
        void insertGTFSRoute(vector<string> &strings);
        void insertGTFSCalendar(vector<string> &strings);
        void createGTFSStopTimesIndexes();
        void createGTFSTripsIndexes();
        void createGTFSStopsIndexes();
        void createGTFSRoutesIndexes();
        string escapeStr(string &str);
        void deleteStopRoute(string route_id);
        bool clearWeatherFrontEnd();
        bool clearBusFrontEnd();
        bool clearPredictions();
        vector<BusObject> getBusData(string route_id);
        vector<StopObject> getStopData(string route_id);
        vector<CoeffObject> getCoeffData(string route_id, int stop_id);
        vector<GoogDirObject> getGoogDirData(string route_id, int timestamp, bool use_cache = true);
        vector<GoogDirObject> getGoogDirDataFrontEnd(string route_id);
        WeatherObject getWeatherFrontEnd();
        RoutePerfObject getRoutePerformance(string route_id, int stop_id);
        int getDirID(string trip_headsign, int block_id);
        vector<string> getRouteDirDests(string route_id, bool is_directions);
        vector<string> getBusDataRoutes();
        vector<string> getRouteGTFSDests(string route_id);
        vector<int> getRouteGTFSBlockIDs(string route_id);
        vector<StopObject> getRouteGTFSStops(string route_id);
        vector<StopObject> getBlockIDOrderedGTFSStops(int block_id, int service_id);
        set<int> getStopBlockIDs(int stop_id);
        set<int> getRouteStopBlockIDs(string route_id, int stop_id);
        set<string> getStopDestinations(int stop_id);
        map<int,int> getGTFSCalendarMap();
        map<int,map<int,vector<int>>> getGTFSStopTimes(string route_id, int stop_id);
        int busCountAtTimestamp(string route_id, int timestamp);
        void deleteBusID(int ID);
        void updateBusValue(int ID, string value_name, string value);
        bool tableExists(string table_name);
        void dropTable(string table_name);

        static MysqlDB* getInstance();
        static MysqlDB* instance;

        string c_server;
        string c_user;
        string c_pass;
        string c_db;
        MYSQL *c_obj;
};

#endif
