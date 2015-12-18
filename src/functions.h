#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>
#include <vector>
#include <set>
#include <map>

#define WEATHER_TIME_GOOD 30 * 60
#define GOOGDIR_TIME_GOOD 45 * 60

#define WEATHER_TIME_REGRAB 5 * 60
#define GOOGDIR_TIME_REGRAB 30 * 60

#define PI 3.14159265358979323846

using namespace std;

class BusObject;
class StopObject;
class CoeffObject;
class TrainerBase;
class RoutePerfObject;

string &make_curl_call(string url);
vector<BusObject> parse_bus_data(string &data, vector<string> *route_list = NULL);
vector<StopObject> parse_stop_data(string &data, string route_id = "");
void get_and_store_stop_data(string route_id);
double current_time();
double stop_watch(string idt = "");
void cleanup_bus_data(vector<BusObject> &bus_list);
void create_coefficient_data(string route_id, bool stop_all, int stop_id, int iterations, int retrain_time);
void create_coefficient_data(vector< vector<BusObject> > &bus_trips, StopObject &stop, TrainerBase *trainer);
void create_train_data(vector< vector< double > > &train_data, vector< vector<BusObject> > &bus_trips, 
							  StopObject &stop, set<string> &stop_dests, vector<int> &route_block_ids, RoutePerfObject &perf_obj,
							  map<int, int> &perf_block_id_trips, map<int, int> &perf_block_id_discarded_trips);
bool double_compare(double a, double b);
double distance(BusObject &bus, StopObject &stop);
double distance(double lat1, double lon1, double lat2, double lon2, char unit = 0);
double reg_distance(double x1, double y1, double x2, double y2);
bool bus_at_stop(BusObject &bus, StopObject &stop);
void add_dimensions(vector<double> &train_entry, double value, int dim_num = 5);
int gtfs_dest_num(string bus_val, vector<string> &route_vals);
int gtfs_block_id_num(int bus_val, vector<int> &route_vals);
int time_till_next_stop(int cur_time, vector<int> &stop_times);
int time_till_next_stop(int cur_time, vector<int> &stop_times, int &stop_time_num);
vector<int> standard_dim_set(int feature_count = 30, int dim_multi = 5);
void fix_dest_str(string &the_str);
void set_bus_counts();
string to_stringe(double val);
void build_bus_train_entry(BusObject &bus, StopObject &stop, vector<double> &train_entry, bool printf_php = false);
vector<StopObject> &gtfs_ordered_stop_ids_for_route_id(string route_id, int service_id);
vector<StopObject> &gtfs_route_stops(string route_id);
set<int> &gtfs_route_stop_block_ids(string route_id, int stop_id);
vector<int> &gtfs_route_block_ids(string route_id);
vector<string> &gtfs_route_dests(string route_id);
vector<string> &gtfs_route_dirs(string route_id);
set<string> &gtfs_stop_dests(int stop_id);
map<int,int> &gtfs_calendar_map();
map< int, map< int, vector< int > > > &gtfs_stop_times(string route_id, int stop_id);
vector<CoeffObject> &get_coefficients(string route_id, int stop_id);
void printout_debug_array(string array_name, vector<double> &array);
void printout_debug_array(string array_name, vector<CoeffObject> &array);
bool get_prediction_check(vector<double> &train_entry, vector<CoeffObject> &coeff_list);
double get_prediction(BusObject &bus, vector<double> &train_entry, vector<CoeffObject> &coeff_list);
bool bus_is_new(BusObject &bus, map<int, BusObject> &old_buses_cache);
bool bus_weather_and_googdir_timestamp_good(BusObject &bus);
void set_day_hour_min_from_timestamp(int timestamp, int &day, int &hour, int &min);


#endif
