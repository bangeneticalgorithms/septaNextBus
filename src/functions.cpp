#include "functions.h"
#include "objects.h"
#include "weather.h"
#include "mysql_db.h"
#include "google_directions.h"
#include "performance_recorder.h"
#include "trainer_octave.h"
#include "trainer_linear.h"
#include "trainer_fann.h"

#include <string>
#include <vector>
#include <string.h>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <time.h>
#include <sys/timeb.h>
#include <map>
#include <math.h>
#include <sys/stat.h>
#include <algorithm>

using namespace std;

size_t make_curl_callback(char* buf, size_t size, size_t nmemb, string* curl_data)
{
	for (int c = 0; c<size*nmemb; c++)
		curl_data->push_back(buf[c]);
	
	//tell curl how many bytes we handled
	return size*nmemb; 
}

string &make_curl_call(string url)
{
	static string curl_data;
	curl_data.clear();

	CURL* curl;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if(curl)
	{
		//struct curl_slist *chunk = NULL;

		//pretend to be chrome
		//chunk = curl_slist_append(chunk, "Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
		//chunk = curl_slist_append(chunk, "Accept-Language:en-US,en;q=0.8");
		//chunk = curl_slist_append(chunk, "Cache-Control:max-age=0");
		//chunk = curl_slist_append(chunk, "Connection:keep-alive");
		//chunk = curl_slist_append(chunk, "User-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/35.0.1916.114 Safari/537.36");
		
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &make_curl_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_data);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 45);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 45);
		//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		//curl_slist_free_all(chunk);
	}
	
	curl_global_cleanup();

	return curl_data;
}

vector<BusObject> parse_bus_data(string &data, vector<string> *route_list)
{
	vector<BusObject> ret_vec;
	Json::Value root;
	Json::Reader reader;
	int timestamp = time(0);
	int weather_timestamp = Weather::getInstance()->last_timestamp;
	
	bool ret = reader.parse( data, root );
	
	if(!ret) return ret_vec;
	
	//root should be an object
	if(!root.isObject()) return ret_vec;
	
	//should have a member
	vector<string> members = root.getMemberNames();
	if(!members.size()) return ret_vec;
	
	//real_root should be an array of objects
	Json::Value real_root = root[members[0]];
	if(!real_root.isArray()) return ret_vec;

	//go through the routes
	for(unsigned int i=0;i<real_root.size();i++)
	{
		//should be an object
		Json::Value &route_obj = real_root[i];
		if(!route_obj.isObject()) continue;
		
		//should have a member
		members = route_obj.getMemberNames();
		if(!members.size()) continue;
		
		string route_id = members[0];
		
		if(route_list) route_list->push_back(route_id);
		
		//that member should be an array
		Json::Value &route_array = route_obj[route_id];
		if(!route_array.isArray()) continue;
		
		//find bus count for this route_id
		int bus_count = 0;
		for(unsigned int j=0;j<route_array.size();j++)
			if(route_array[j].isObject())
				bus_count++;
			
		for(unsigned int j=0;j<route_array.size();j++)
		{
			//busses should be objects
			Json::Value &bus_obj = route_array[j];
			if(!bus_obj.isObject()) continue;
			
			int googdir_timestamp = GoogDir::getInstance()->getRouteLastTimestamp(route_id);
			
			ret_vec.push_back(BusObject(bus_obj, route_id, timestamp, weather_timestamp, googdir_timestamp, bus_count));
		}
	}
	
	return ret_vec;
}

vector<StopObject> parse_stop_data(string &data, string route_id)
{
	vector<StopObject> ret_vec;
	Json::Value root;
	Json::Reader reader;
	
	bool ret = reader.parse( data, root );
	
	if(!ret) return ret_vec;
	
	//printf("parse_stop_data: '%s'\n", root.toStyledString().c_str());
	
	if(!root.isArray()) return ret_vec;
	
	for(unsigned int i=0;i<root.size();i++)
	{
		//should be an object
		Json::Value &stop_obj = root[i];
		if(!stop_obj.isObject()) continue;
		
		ret_vec.push_back(StopObject(stop_obj, route_id));
	}
	
	return ret_vec;
}

void get_and_store_stop_data(string route_id)
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	string call_data = make_curl_call(string("http://www3.septa.org/hackathon/Stops/") + route_id);
	vector<StopObject> stop_list = parse_stop_data(call_data, route_id);
	
	printf("route_list:'%s' stops:%lu\n", route_id.c_str(), stop_list.size());
	
	//kill stops already of this route
	myobj->deleteStopRoute(route_id);
	
	//insert new stops
	for(int i=0;i<stop_list.size();i++)
		myobj->insertStopData(stop_list[i]);
}

double current_time()
{
	static int first_sec = 0;
	static int first_msec = 0;
	struct timeb new_time;
	
	//set current time
	ftime(&new_time);

	//set if not set
	if(!first_sec)
	{
		first_sec = new_time.time;
		first_msec = new_time.millitm;
	}
	
	return (new_time.time - first_sec) + ((new_time.millitm - first_msec) * 0.001);
}

double stop_watch(string idt)
{
	static bool started = false;
	static double start_time = 0;
	double ret_time = 0;
	
	if(!started)
		start_time = current_time();
	else
	{
		ret_time = current_time() - start_time;
		printf("stop_watch:stopped for '%s' - %lf\n", idt.c_str(), ret_time);
	}
	
	started = !started;
	
	return ret_time;
}

void cleanup_bus_data(vector<BusObject> &bus_list)
{
	MysqlDB *myobj = MysqlDB::getInstance();
	map<int,BusObject> bus_map;
	
	for(auto i=bus_list.begin();i!=bus_list.end();)
	{
		BusObject &bus = *i;
		bool delete_it = false;
		
		if(bus.offset) delete_it = true;
		
		//last_bus
		if(bus_map.find(bus.label) != bus_map.end())
		{
			BusObject &last_bus = bus_map[bus.label];
			
			if(last_bus.lat == bus.lat && last_bus.lng == bus.lng) delete_it = true;
		}
		
		if(delete_it)
		{
			myobj->deleteBusID(bus.ID);
			i = bus_list.erase(i);
		}
		else
		{
			bus_map[bus.label] = bus;
			++i;
		}
	}
}

bool double_compare(double a, double b)
{
    return fabs(a - b) < 0.00001;
}

double distance(BusObject &bus, StopObject &stop)
{
	return distance(bus.lat, bus.lng, stop.lat, stop.lng);
}

double deg2rad(double deg) 
{
	return (deg * PI / 180);
}

double rad2deg(double rad) 
{
	return (rad * 180 / PI);
}

//lat long distance
double distance(double lat1, double lon1, double lat2, double lon2, char unit)
{
	double theta, dist;
	theta = lon1 - lon2;
	dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
	dist = acos(dist);
	dist = rad2deg(dist);
	dist = dist * 60 * 1.1515;
	switch(unit) {
		case 'M': //is statute miles (default)
			break;
		case 'K': //is kilometers
			dist = dist * 1.609344;
			break;
		case 'N': //is nautical miles
			dist = dist * 0.8684;
			break;
	}
	return (dist);
}

double reg_distance(double x1, double y1, double x2, double y2)
{
	double dx = x1-x2;
	double dy = y1-y2;
	
	return sqrt((dx*dx)+(dy*dy));
}


void debug_bus_trips(vector< vector<BusObject> > &bus_trips)
{
	printf("debug_bus_trips:trips found %lu\n", bus_trips.size());
	
	int total = 0;
	
	for(int i=0;i<bus_trips.size();i++)
	{
		vector<BusObject> &trip = bus_trips[i];
		
		printf("trip %lu ---------------------------\n", trip.size());
		for(int j=0;j<trip.size();j++)
		{
			BusObject &bus = trip[j];
			
			printf("trip %06d: %06d %lf %lf %d\n", i, bus.label, bus.lat, bus.lng, bus.timestamp);
			total++;
		}
	}
	
	printf("total %d\n", total);
}

bool route_stop_training_is_old(string &route_id, int stop_id, int retrain_time)
{
	if(retrain_time <= 0) return true;
	
	MysqlDB *myobj = MysqlDB::getInstance();
	RoutePerfObject perf_obj = myobj->getRoutePerformance(route_id, stop_id);
	
	if(!perf_obj.timestamp) return true;
	
	if(abs(time(0) - perf_obj.timestamp) > retrain_time) return true;
	
	return false;
}

//this function takes raw gps data with timestamps and turns it into data
//a machine learning algorithm can use
//first it cleans the data, removes duplicates etc
//then it groups it into trips
//a trip is when a bus begins the route to when the bus leaves the route
void create_coefficient_data(string route_id, bool stop_all, int stop_id, int iterations, int retrain_time)
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	vector<BusObject> bus_list = myobj->getBusData(route_id);
	vector<StopObject> stop_list = myobj->getRouteGTFSStops(route_id);
	//vector<StopObject> stop_list = myobj->getStopData(route_id);
	
	//get rid of offsets of non zero
	//and duplicates etc
	cleanup_bus_data(bus_list);
	
	printf("bus_list.size():%lu\n", bus_list.size());
	printf("stop_list.size():%lu\n", stop_list.size());
	
	//create bus ride segments
	map<int, vector<BusObject> > last_bus_trip;
	vector< vector<BusObject> > bus_trips;
	
	//bus_list is sorted by timestamp
	//build upon last_bus_trip the bus trip
	//for a certain bus.label
	//make a new trip and store the previous
	//in bus_trips if any bus.label's timestamps
	//excede 10 hours
	for(int i=0;i<bus_list.size();i++)
	{
		BusObject &bus = bus_list[i];
		
		auto iter = last_bus_trip.find(bus.label);
		
		//break off current list?
		if(iter != last_bus_trip.end())
		{
			BusObject &last_bus = iter->second.back();
			
			//time difference of next bus vs last bus
			//is this over 10 hours? if so make new trip
			int time_diff = abs((bus.timestamp-bus.offset_sec)-(last_bus.timestamp-last_bus.offset_sec));
			if(time_diff > 10 * 60 * 60)
			{
				bus_trips.push_back(iter->second);
				last_bus_trip.erase(iter);
			}
			else if(time_diff > 0)
			{
				//calculate velocity etc
				bus.dist_since_prev = distance(bus.lat, bus.lng, last_bus.lat, last_bus.lng);
				bus.time_since_prev = time_diff;
				bus.velocity = bus.dist_since_prev / bus.time_since_prev;
				
				if(bus.lat > last_bus.lat) bus.lat_gt_prev = true;
				if(bus.lat < last_bus.lat) bus.lat_lt_prev = true;
				if(bus.lng > last_bus.lng) bus.lng_gt_prev = true;
				if(bus.lng < last_bus.lng) bus.lng_lt_prev = true;
			}
		}
		
		last_bus_trip[bus.label].push_back(bus);
	}
	
	//grab out remaining trips
	for(auto iter = last_bus_trip.begin();iter != last_bus_trip.end(); ++iter)
		bus_trips.push_back(iter->second);
	
	//build coefficient data against all stops
	for(int i=0;i<stop_list.size();i++)
	{
		//TrainerOctave trainer;
		TrainerLinear trainer;
		//TrainerFANN trainer;
		StopObject &s = stop_list[i];
		
		trainer.route_id = route_id;
		trainer.stop_id = s.stop_id;
		trainer.iterations = iterations;
		
		if((stop_all || s.stop_id == stop_id) && route_stop_training_is_old(route_id, s.stop_id, retrain_time))
			create_coefficient_data(bus_trips, s, &trainer);
	}
}

bool bus_at_stop(BusObject &bus, StopObject &stop)
{
	return (distance(bus, stop) < 0.05);
}

void add_dimensions(vector<double> &train_entry, double value, int dim_num)
{
	for(int i=0;i<dim_num;i++) 
		train_entry.push_back(pow(value, i+1));
}

vector<StopObject> &ordered_stop_ids_for_block_id(int block_id, int service_id)
{
	static map< int, map< int, vector<StopObject> > > cache;
	
	//in the cache?
	map< int, map< int, vector<StopObject> > >::iterator it = cache.find(block_id);
	if(it != cache.end())
	{
		map< int, vector<StopObject> >::iterator it2 = it->second.find(service_id);
		
		if(it2 != it->second.end())
			return it2->second;
	}
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[block_id][service_id] = myobj->getBlockIDOrderedGTFSStops(block_id, service_id);
	
	//for(int i=0;i<cache[block_id][service_id].size();i++)
	//	cache[block_id][service_id][i].debug();
	
	printf("ordered_stop_ids_for_block_id: [%d][%d]:%lu\n", block_id, service_id, cache[block_id][service_id].size());
	
	return cache[block_id][service_id];
}

vector<StopObject> &gtfs_ordered_stop_ids_for_route_id(string route_id, int service_id)
{
	static map< string, int > cache;
	
	//in the cache?
	map< string, int >::iterator it = cache.find(route_id);
	if(it != cache.end())
		return ordered_stop_ids_for_block_id(it->second, service_id);
	
	//find the block id with the most stops
	int block_id=0;
	int block_id_stops=0;
	
	vector<int> &block_ids = gtfs_route_block_ids(route_id);
	
	for(int i=0;i<block_ids.size();i++)
	{
		int cur_bid = block_ids[i];
		vector<StopObject> &stops = ordered_stop_ids_for_block_id(cur_bid, service_id);
		
		if(stops.size() < block_id_stops) continue;
		
		block_id = cur_bid;
		block_id_stops = stops.size();
	}
	
	if(!block_id) printf("gtfs_ordered_stop_ids_for_route_id: no block ids found for '%s'!!\n", route_id.c_str());
	
	cache[route_id] = block_id;
	
	printf("gtfs_ordered_stop_ids_for_route_id: [%s]:%d\n", route_id.c_str(), cache[route_id]);
	
	return ordered_stop_ids_for_block_id(cache[route_id], service_id);
}

int nearest_stop_id_with_direction(BusObject &bus, vector<StopObject> &stops)
{
	//choose a known off number
	if(!stops.size()) return -1;
	
	int ret_val = -1;
	double ret_dist = 10000;
	
	//if another stop is closer choose that
	for(int i=1;i<stops.size();i++)
	{
		if(bus.direction_id != stops[i].direction_id) continue;
		
		double new_dist = distance(bus, stops[i]);
		
		if(new_dist >= ret_dist) continue;
		
		ret_val = stops[i].stop_id;
		ret_dist = new_dist;
	}
	
	return ret_val;
}

int stops_away_from(int stop_id, int dest_stop_id, vector<StopObject> &stop_list)
{
	int stop_i = -1;
	int dest_stop_i = -1;
	
	int list_size = stop_list.size();
	for(int i=0;i<list_size;i++)
	{
		if(stop_list[i].stop_id == stop_id) stop_i = i;
		if(stop_list[i].stop_id == dest_stop_id) dest_stop_i = i;
	}
	
	if(stop_i == -1) return -1;
	if(dest_stop_i == -1) return -1;
	if(stop_i <= dest_stop_i) return dest_stop_i - stop_i;
	if(dest_stop_i < stop_i) return (list_size - stop_i) + dest_stop_i;
	
	//shouldn't be able to get here
	return -1;
}

int gtfs_dir_dest_num(string trip_headsign, int block_id)
{
	static map< string, map< int, int> > cache;
	
	//in the cache?
	map< string, map< int, int> >::iterator it = cache.find(trip_headsign);
	if(it != cache.end())
	{
		map< int, int>::iterator it2 = it->second.find(block_id);
		
		if(it2 != it->second.end())
			return it2->second;
	}
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	int ret_val = myobj->getDirID(trip_headsign, block_id);
	
	cache[trip_headsign][block_id] = ret_val;
	
	//printf("gtfs_dir_dest_num:value[%s][%d]:%d\n", trip_headsign.c_str(), block_id, ret_val);
	
	return ret_val;
}

void fix_dest_str(string &the_str)
{
	//some chars that are bugged
	replace( the_str.begin(), the_str.end(), '`', '\'');
}

int gtfs_dest_num(string bus_val, vector<string> &route_vals)
{
	//no value given
	if(!bus_val.length()) return 0;
	
	//some chars are bugged
	fix_dest_str(bus_val);
	
	//just find a match
	for(int i=0;i<route_vals.size();i++)
		if(!strcmp(bus_val.c_str(), route_vals[i].c_str()))
			return i+1;
	
	//shouldn't be here
	printf("gtfs_dest_num: couldn't find '%s' in - ", bus_val.c_str());
	for(int i=0;i<route_vals.size();i++)
		printf(" '%s'", route_vals[i].c_str());
	printf("\n");
	
	return 0;
}

int gtfs_block_id_num(int bus_val, vector<int> &route_vals)
{
	//just find a match
	for(int i=0;i<route_vals.size();i++)
		if(bus_val == route_vals[i])
			return i+1;
	
	//shouldn't be here
	printf("gtfs_block_id_num: couldn't find '%d' in - ", bus_val);
	for(int i=0;i<route_vals.size();i++)
		printf(" %d,", route_vals[i]);
	printf("\n");
	
	return 0;
}

//needed to update a test database
void set_bus_counts()
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	vector<BusObject> bus_list = myobj->getBusData("21");
	
	for(int i=0;i<bus_list.size();i++)
		myobj->updateBusValue(bus_list[i].ID, "bus_count", to_string(myobj->busCountAtTimestamp("21", bus_list[i].timestamp)));
}

int nearest_stop(BusObject &bus, vector<StopObject> &stops)
{
	//choose a known off number
	if(!stops.size()) return -1;
	
	//start with 0 as initially choosen
	int ret_val = 0;
	double ret_dist = distance(bus, stops[0]);
	
	//if another stop is closer choose that
	for(int i=1;i<stops.size();i++)
	{
		double new_dist = distance(bus, stops[i]);
		
		if(new_dist >= ret_dist) continue;
		
		ret_val = i;
		ret_dist = new_dist;
	}
	
	return ret_val;
}

int time_till_next_stop(int cur_time, vector<int> &stop_times, int &stop_time_num)
{
	if(!stop_times.size())
	{
		printf("time_till_next_stop:: stop_times.size() is zero! \n");
		stop_time_num = -1;
		return 0;
	}
	
	for(int i=stop_times.size()-1;i>=0;i--)
		if(cur_time < stop_times[i])
		{
			stop_time_num = i;
			return stop_times[stop_time_num] - cur_time;
		}
		
	//cur_time greater than all stop times!
	//printf("time_till_next_stop:: cur_time greater than all stop times! \n");
	stop_time_num = stop_times.size()-1;
	return stop_times[stop_time_num] - cur_time;
}

vector<int> standard_dim_set(int feature_count, int dim_multi)
{
	vector<int> ret_vec;
	
	ret_vec.assign(feature_count, dim_multi);
	
	return ret_vec;
}

//this function isn't supported atm
vector<int> best_dim_set(vector< vector< double > > &train_data, int feature_count, vector< vector<BusObject> > &bus_trips, StopObject &stop,
							  vector<string> &route_dirs, vector<string> &route_dests, vector<int> &route_block_ids, 
							  vector<StopObject> &route_stops, set<string> &stop_dests, map<int,int> &cal_map, map<int,map<int,vector<int>>> &stop_times, TrainerBase *trainer)
{
	vector<int> ret_vec;
	vector< vector<double> > results;
	vector<int> count_choices;
	
	// the dimension amounts we will try for each feature
	//count_choices.push_back(0);
	count_choices.push_back(1);
	count_choices.push_back(2);
	count_choices.push_back(3);
	count_choices.push_back(4);
	count_choices.push_back(5);
	count_choices.push_back(6);
	count_choices.push_back(7);
	count_choices.push_back(8);
	count_choices.push_back(9);
	count_choices.push_back(10);
	//count_choices.push_back(15);
	//count_choices.push_back(20);
	//count_choices.push_back(25);
	//count_choices.push_back(30);
	//count_choices.push_back(35);
	//count_choices.push_back(40);
	
	//get the results
	for(int i=0;i<feature_count;i++)
	{
		vector<int> dim_set;
		
		dim_set = standard_dim_set(feature_count, 1);
		results.push_back(vector<double>());
		
		for(int j=0; j<count_choices.size(); j++)
		{
			double result;
			RoutePerfObject perf_obj;
			map<int, int> perf_block_id_trips;
			map<int, int> perf_block_id_discarded_trips;
			
			dim_set[i] = count_choices[j];
			
			create_train_data(train_data, bus_trips, stop, stop_dests, route_block_ids, perf_obj, perf_block_id_trips, perf_block_id_discarded_trips);
			
			vector<double> theta_results;
			vector<double> mu_results;
			vector<double> sigma_results;
			result = trainer->trainData(train_data, theta_results, mu_results, sigma_results);
			
			results[i].push_back(result);
		}
	}
	
	for(int i=0;i<results.size();i++)
	{
		int choice = count_choices[0];
		double choice_value = results[i][0];
		
		printf("best_dim_set: feature %d ----- \n", i);
		printf("best_dim_set: [%02d] %lf \n", 0, results[i][0]);
		
		for(int j=1;j<results[i].size() && j<count_choices.size();j++)
		{
			printf("best_dim_set: [%02d] %lf \n", j, results[i][j]);
			
			if(results[i][j] < choice_value)
			{
				choice = count_choices[j];
				choice_value = results[i][j];
			}
		}
		
		ret_vec.push_back(choice);
	}
	
	for(int i=0;i<ret_vec.size();i++)
		printf("best_dim_set: ret_vec [%02d] %d \n", i, ret_vec[i]);
	
	return ret_vec;
}

//this function isn't supported atm
vector<int> best_dim_set_basic(vector< vector< double > > &train_data, int feature_count, vector< vector<BusObject> > &bus_trips, StopObject &stop,
							  vector<string> &route_dirs, vector<string> &route_dests, vector<int> &route_block_ids, 
							  vector<StopObject> &route_stops, set<string> &stop_dests, map<int,int> &cal_map, map<int,map<int,vector<int>>> &stop_times, TrainerBase *trainer)
{
	vector<int> ret_vec;
	vector<double> results;
	vector<int> count_choices;
	
	// the dimension amounts we will try for each feature
	//count_choices.push_back(0);
	count_choices.push_back(1);
	count_choices.push_back(2);
	count_choices.push_back(3);
	count_choices.push_back(4);
	count_choices.push_back(5);
	count_choices.push_back(6);
	count_choices.push_back(7);
	count_choices.push_back(8);
	count_choices.push_back(9);
	count_choices.push_back(10);
	//count_choices.push_back(15);
	//count_choices.push_back(20);
	//count_choices.push_back(25);
	//count_choices.push_back(30);
	//count_choices.push_back(35);
	//count_choices.push_back(40);
	
	//get the results
	for(int i=0;i<count_choices.size();i++)
	{
		vector<int> dim_set;
		double result;
		RoutePerfObject perf_obj;
		map<int, int> perf_block_id_trips;
		map<int, int> perf_block_id_discarded_trips;
		
		dim_set = standard_dim_set(feature_count, count_choices[i]);
		
		//create_train_data(train_data, dim_set, bus_trips, stop, route_dirs, route_dests, route_block_ids, route_stops, stop_dests, cal_map, stop_times);
		create_train_data(train_data, bus_trips, stop, stop_dests, route_block_ids, perf_obj, perf_block_id_trips, perf_block_id_discarded_trips);
		
		vector<double> theta_results;
		vector<double> mu_results;
		vector<double> sigma_results;
		result = trainer->trainData(train_data, theta_results, mu_results, sigma_results);
		
		results.push_back(result);
	}
	
	int choice = count_choices[0];
	double choice_value = results[0];
	
	for(int i=0;i<results.size();i++)
	{
		if(results[i] < choice_value)
		{
			choice = count_choices[i];
			choice_value = results[i];
		}
	}
	
	ret_vec = standard_dim_set(feature_count, choice);
	
	for(int i=0;i<ret_vec.size();i++)
		printf("best_dim_set: ret_vec [%02d] %d \n", i, ret_vec[i]);
	
	return ret_vec;
}

int used_features(vector<double> &mu_results, vector<double> &sigma_results)
{
	int ret_val = 0;
	
	if(mu_results.size() != sigma_results.size())
		return 0;
	
	for(int i=0;i<mu_results.size();i++)
		if(!(double_compare(mu_results[i], 0) && double_compare(sigma_results[i], 1)))
			ret_val++;
		
	return ret_val;
}

//for each trip, searching in reverse, find the first entry that is "at the stop"
//create data from all the following entries until you cross the stop again
void create_coefficient_data(vector< vector<BusObject> > &bus_trips, StopObject &stop, TrainerBase *trainer)
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	if(!trainer) return;
	
	RoutePerfObject perf_obj;
	map<int, int> perf_block_id_trips;
	map<int, int> perf_block_id_discarded_trips;
	vector< vector<double> > train_data;
	
	vector<int> route_block_ids = myobj->getRouteGTFSBlockIDs(trainer->route_id);
	set<string> stop_dests = myobj->getStopDestinations(trainer->stop_id);

	vector<int> dim_set;
	
	dim_set = standard_dim_set(20);
	//dim_set = best_dim_set_basic(train_data, 19, bus_trips, stop, route_dirs, route_dests, route_block_ids, route_stops, stop_dests, cal_map, stop_times, trainer);
	
	//create_train_data(train_data, dim_set, bus_trips, stop, route_dirs, route_dests, route_block_ids, route_stops, stop_dests, cal_map, stop_times);
	create_train_data(train_data, bus_trips, stop, stop_dests, route_block_ids, perf_obj, perf_block_id_trips, perf_block_id_discarded_trips);
	
	stop_watch();
	vector<double> theta_results;
	vector<double> mu_results;
	vector<double> sigma_results;
	double result = trainer->trainData(train_data, theta_results, mu_results, sigma_results);
	double train_time = stop_watch("trainer->trainData");
	
	printf("create_coefficient_data:trainData result:%lf \n", result);
	
	//write out results for graphing
	//trainer->writeResultData(train_data, theta_results, mu_results, sigma_results);
	
	//put them into mysql
	myobj->insertCoeff(trainer->route_id, trainer->stop_id, theta_results, mu_results, sigma_results);
	
	//store performance data
	perf_obj.iterations = trainer->iterations;
	perf_obj.data_points = train_data.size();
	perf_obj.used_features = used_features(mu_results, sigma_results);
	perf_obj.total_features = mu_results.size();
	perf_obj.train_time = train_time;
	perf_obj.mean_squared_error = result;
	perf_obj.timestamp = time(0);
	PerfRec::getInstance()->storeRoutePerformance(trainer->route_id, trainer->stop_id, perf_obj);
	PerfRec::getInstance()->storeRouteModelApply(trainer->route_id, trainer->stop_id, train_data, theta_results, mu_results, sigma_results, perf_obj.timestamp);
	PerfRec::getInstance()->storeRouteCostGraph(trainer->route_id, trainer->stop_id, trainer->cost_graph, perf_obj.timestamp);
	PerfRec::getInstance()->storeRouteBlockIdTrips(trainer->route_id, trainer->stop_id, perf_block_id_trips, perf_block_id_discarded_trips, perf_obj.timestamp);
}

bool block_id_in(int block_id, vector<int> &route_block_ids)
{
	for(int i=0;i<route_block_ids.size();i++)
		if(block_id == route_block_ids[i])
			return true;
		
	return false;
}

bool bus_weather_and_googdir_timestamp_good(BusObject &bus)
{
	if(abs(bus.timestamp - bus.weather_timestamp) > WEATHER_TIME_GOOD) return false;
	if(abs(bus.timestamp - bus.googdir_timestamp) > GOOGDIR_TIME_GOOD) return false;
	
	return true;
}

bool bus_is_good(BusObject &bus, StopObject &stop, vector<int> &route_block_ids, RoutePerfObject &perf_obj)
{
	if(!bus.block_id) { perf_obj.discarded_bad_block_id++; return false; }
	if(!bus.destination.size()) { perf_obj.discarded_bad_dest++; return false; }
	if(bus.destination.size()==1) { perf_obj.discarded_bad_dest++; return false; }
	if(!bus.direction.size()) { perf_obj.discarded_bad_dir++; return false; }
	if(bus.direction.size()==1) { perf_obj.discarded_bad_dir++; return false; }
	if(bus.direction_id == -1) { perf_obj.discarded_bad_dir_id++; return false; }
	if(bus.stops_away == -1) { perf_obj.discarded_bad_stops_away++; return false; }
	if(bus.node_times.size() < 9) { perf_obj.discarded_bad_node_times++; return false; }
	//if(!bus.time_since_prev) { return false; }
	//if(bus.time_since_prev > 10 * 60) { return false; }
	
	if(!block_id_in(bus.block_id, route_block_ids)) { perf_obj.discarded_bad_block_id_in++; return false; }
	
	if(!bus_weather_and_googdir_timestamp_good(bus)) { perf_obj.discarded_bad_weather_googdir++; return false; }
	
	//going to have an eventual problem with the stop times?
	//int day, hour, min;
	//set_day_hour_min_from_timestamp(bus.timestamp, day, hour, min);
	//map<int,int> &cal_map = gtfs_calendar_map();
	//map<int,map<int,vector<int>>> &stop_times = gtfs_stop_times(bus.route_id, stop.stop_id);
	//if(!stop_times[bus.block_id][cal_map[day]].size()) { perf_obj.discarded_bad_stop_times++; return false; }
	
	return true;
}

void printout_debug_array(string array_name, vector<double> &array)
{
	for(int i=0;i<array.size();i++)
		printf("%s[%d]:%E\n", array_name.c_str(), i, array[i]);
}

void printout_debug_array(string array_name, vector<CoeffObject> &array)
{
	for(int i=0;i<array.size();i++)
		printf("%s[%d]:%E\n", array_name.c_str(), i, array[i].theta);
}

bool get_prediction_check(vector<double> &train_entry, vector<CoeffObject> &coeff_list)
{
	return (train_entry.size() + 1 == coeff_list.size());
}

double get_prediction(BusObject &bus, vector<double> &train_entry, vector<CoeffObject> &coeff_list)
{
	if(!get_prediction_check(train_entry, coeff_list))
	{
		printf("get_prediction error: [%lu vs %lu] \n", train_entry.size(), coeff_list.size());
		return 0;
	}

	double total = coeff_list[0].theta;
	for (int i=0;i<train_entry.size();i++)
	{
		double value = train_entry[i];
		CoeffObject &coefficient = coeff_list[i+1];
		
		value -= coefficient.mu;
		value /= coefficient.sigma;
		total += value * coefficient.theta;
	}
	
	//adjust for difference in time since septa got the data
	total -= bus.offset_sec;

	//adjust for difference in time since we got the data
	int timestamp_diff = time(0) - bus.timestamp;

	if(timestamp_diff > 0)
		total -= timestamp_diff;
	
	return total;
}

bool bus_is_new(BusObject &bus, map<int, BusObject> &old_buses_cache)
{
	//this vehicle_id stored?
	map<int, BusObject>::iterator it = old_buses_cache.find(bus.vehicle_id);
	if(it != old_buses_cache.end())
	{
		//does the stored one have the same lat/lng and a more recent offset?
		if(it->second.lat == bus.lat 
			&& it->second.lng == bus.lng 
			&& it->second.offset_sec < bus.offset_sec)
		{
			/*
			printf("bus_is_new: false [%lf vs %lf] [%lf vs %lf] [%d vs %d] \n", 
				it->second.lat, bus.lat,
				it->second.lng, bus.lng,
				it->second.offset_sec, bus.offset_sec);
			*/
			
			return false;
		}
	}
	
	old_buses_cache[bus.vehicle_id] = bus;
	
	//printf("bus_is_new: true\n");
	
	return true;
}

vector<StopObject> &gtfs_route_stops(string route_id)
{
	static map< string, vector<StopObject> > cache;
	
	//in the cache?
	map< string, vector<StopObject> >::iterator it = cache.find(route_id);
	if(it != cache.end())
		return it->second;
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[route_id] = myobj->getRouteGTFSStops(route_id);
	
	printf("gtfs_route_stops:cache[%s].size():%lu\n", route_id.c_str(), cache[route_id].size());
	
	return cache[route_id];
}

set<int> &gtfs_route_stop_block_ids(string route_id, int stop_id)
{
	static map< string, map< int, set<int> > > cache;
	
	//in the cache?
	auto it = cache.find(route_id);
	if(it != cache.end())
	{
		auto it2 = it->second.find(stop_id);
		
		if(it2 != it->second.end())
			return it2->second;
	}
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[route_id][stop_id] = myobj->getRouteStopBlockIDs(route_id, stop_id);
	
	printf("gtfs_route_stop_block_ids:cache[%s][%d].size():%lu\n", route_id.c_str(), stop_id, cache[route_id][stop_id].size());
	
	return cache[route_id][stop_id];
}

vector<int> &gtfs_route_block_ids(string route_id)
{
	static map< string, vector<int> > cache;
	
	//in the cache?
	map< string, vector<int> >::iterator it = cache.find(route_id);
	if(it != cache.end())
		return it->second;
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[route_id] = myobj->getRouteGTFSBlockIDs(route_id);
	
	printf("gtfs_route_block_ids:cache[%s].size():%lu\n", route_id.c_str(), cache[route_id].size());
	
	return cache[route_id];
}

vector<string> &gtfs_route_dests(string route_id)
{
	static map< string, vector<string> > cache;
	
	//in the cache?
	map< string, vector<string> >::iterator it = cache.find(route_id);
	if(it != cache.end())
		return it->second;
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[route_id] = myobj->getRouteGTFSDests(route_id);
	
	printf("gtfs_route_dests:cache[%s].size():%lu\n", route_id.c_str(), cache[route_id].size());
	
	return cache[route_id];
}

vector<string> &gtfs_route_dirs(string route_id)
{
	static map< string, vector<string> > cache;
	
	//in the cache?
	map< string, vector<string> >::iterator it = cache.find(route_id);
	if(it != cache.end())
		return it->second;
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[route_id] = myobj->getRouteDirDests(route_id, true);
	
	printf("gtfs_route_dirs:cache[%s].size():%lu\n", route_id.c_str(), cache[route_id].size());
	
	return cache[route_id];
}

set<string> &gtfs_stop_dests(int stop_id)
{
	static map< int, set<string> > cache;
	
	//in the cache?
	map< int, set<string> >::iterator it = cache.find(stop_id);
	if(it != cache.end())
		return it->second;
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[stop_id] = myobj->getStopDestinations(stop_id);
	
	printf("gtfs_stop_dests:cache[%d].size():%lu\n", stop_id, cache[stop_id].size());
	
	return cache[stop_id];
}

map<int,int> &gtfs_calendar_map()
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	static map<int,int> cache = myobj->getGTFSCalendarMap();

	return cache;
}

map< int, map< int, vector< int > > > &gtfs_stop_times(string route_id, int stop_id)
{
	static map< string, map< int, map< int, map< int, vector< int > > > > > cache;
	
	//in the cache?
	map< string, map< int, map< int, map< int, vector< int > > > > >::iterator it = cache.find(route_id);
	if(it != cache.end())
	{
		map< int, map< int, map< int, vector< int > > > >::iterator it2 = it->second.find(stop_id);
		
		if(it2 != it->second.end())
			return it2->second;
	}
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[route_id][stop_id] = myobj->getGTFSStopTimes(route_id, stop_id);
	
	printf("gtfs_stop_times: [%s][%d]\n", route_id.c_str(), stop_id);
	
	return cache[route_id][stop_id];
}

vector<CoeffObject> &get_coefficients(string route_id, int stop_id)
{
	static map< string, map< int, vector<CoeffObject> > > cache;
	
	//in the cache?
	map< string, map< int, vector<CoeffObject> > >::iterator it = cache.find(route_id);
	if(it != cache.end())
	{
		map< int, vector<CoeffObject> >::iterator it2 = it->second.find(stop_id);
		
		if(it2 != it->second.end())
			return it2->second;
	}
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	cache[route_id][stop_id] = myobj->getCoeffData(route_id, stop_id);
	
	printf("get_coefficients: [%s][%d]\n", route_id.c_str(), stop_id);
	
	return cache[route_id][stop_id];
}

void set_day_hour_min_from_timestamp(int timestamp, int &day, int &hour, int &min)
{
	time_t timestamp_t = timestamp - (4 * 60 * 60);
	
	//tm *ti = localtime(&timestamp_t);
	tm *ti = gmtime(&timestamp_t); //UTC time
	
	//is daylight savings time in effect?
	if((ti->tm_mon < 2) || (ti->tm_mon > 10) || (ti->tm_mon == 2 && ti->tm_mday < 14) || (ti->tm_mon == 10 && ti->tm_mday > 7))
	{
		//daylight savings time second sunday in march
		//standard time first sunday november
		
		timestamp_t = timestamp - (5 * 60 * 60);
		tm *ti = gmtime(&timestamp_t);
	}
	
	day = ti->tm_wday;
	hour = ti->tm_hour;
	min = ti->tm_min;
}

void set_bus_stops_away(BusObject &bus, StopObject &stop)
{
	//int day, hour, min;
	//set_day_hour_min_from_timestamp(bus.timestamp, day, hour, min);
	//map<int,int> &cal_map = gtfs_calendar_map();
	
	//set this if needed
	if(bus.direction_id = -1)
	{
		//some chars are bugged
		fix_dest_str(bus.destination);
	
		bus.direction_id = gtfs_dir_dest_num(bus.destination, bus.block_id);
	}

	//4 seems to be covered most by block_ids, using cal_map tosses out a lot of block_ids
	vector<StopObject> &ordered_stops = ordered_stop_ids_for_block_id(bus.block_id, 4);//cal_map[day]);
	bus.nearest_stop_id = nearest_stop_id_with_direction(bus, ordered_stops);
	bus.stops_away = stops_away_from(bus.nearest_stop_id, stop.stop_id, ordered_stops);
	
	if(!ordered_stops.size())
		printf("set_bus_stops_away: !ordered_stops.size() block_id:%d day:%d cal_map:%d\n", bus.block_id, 4, 0);//day, cal_map[day]);
	
	//printf("set_bus_stops_away: direction_id:%d nearest_stop_id:%d stops_away:%d \n", bus.direction_id, bus.nearest_stop_id, bus.stops_away);
}

void build_bus_train_entry(BusObject &bus, StopObject &stop, vector<double> &train_entry, bool printf_php)
{
	train_entry.clear();
	
	//vector<string> &route_dirs = gtfs_route_dirs(bus.route_id); //myobj->getRouteDirDests(trainer->route_id, true);
	vector<string> &route_dests = gtfs_route_dests(bus.route_id); //myobj->getRouteGTFSDests(trainer->route_id);
	vector<int> &route_block_ids = gtfs_route_block_ids(bus.route_id); //myobj->getRouteGTFSBlockIDs(trainer->route_id);
	vector<StopObject> &route_stops = gtfs_route_stops(bus.route_id); //myobj->getRouteGTFSStops(trainer->route_id);
	//set<int> stop_block_ids = myobj->getStopBlockIDs(trainer->stop_id);
	//set<string> &stop_dests = gtfs_stop_dests(stop.stop_id); //myobj->getStopDestinations(trainer->stop_id);
	map<int,int> &cal_map = gtfs_calendar_map(); //myobj->getGTFSCalendarMap();
	map<int,map<int,vector<int>>> &stop_times = gtfs_stop_times(bus.route_id, stop.stop_id); //myobj->getGTFSStopTimes(trainer->route_id, trainer->stop_id);

	static vector<int> dim_set = standard_dim_set(21 + (9*3));
	
	//this does fix_dest_str and direction_id = gtfs_dir_dest_num
	set_bus_stops_away(bus, stop);
	
	double dist = distance(bus, stop);
	int day, hour, min;
	set_day_hour_min_from_timestamp(bus.timestamp, day, hour, min);
	int hour2 = hour / 12;
	int hour4 = hour / 6;
	int hour6 = hour / 4;
	int hour12 = hour / 2;
	int seconds_since_midnight = ((hour+1)*60)+min;
	int dest = gtfs_dest_num(bus.destination, route_dests);
	int block_id_num = gtfs_block_id_num(bus.block_id, route_block_ids);
	int bus_count = bus.bus_count;//myobj->busCountAtTimestamp(trainer->route_id, bus.timestamp);
	int near_stop_num = nearest_stop(bus, route_stops);
	int stop_time_num = 0;
	int stop_time_diff = time_till_next_stop(seconds_since_midnight, stop_times[bus.block_id][cal_map[day]], stop_time_num);
	
	int d = 0;
	
	//lat / lng / distance manipulations
	add_dimensions(train_entry, bus.lat, dim_set[d++]);
	add_dimensions(train_entry, bus.lng, dim_set[d++]);
	add_dimensions(train_entry, bus.lat * bus.lng, dim_set[d++]);
	add_dimensions(train_entry, bus.lat / bus.lng, dim_set[d++]);
	add_dimensions(train_entry, dist, dim_set[d++]);
	add_dimensions(train_entry, stop.lat - bus.lat, dim_set[d++]);
	add_dimensions(train_entry, stop.lng - bus.lng, dim_set[d++]);
	
	//what day of the week?
	train_entry.push_back(day==0 ? 1 : 0);
	train_entry.push_back(day==1 ? 1 : 0);
	train_entry.push_back(day==2 ? 1 : 0);
	train_entry.push_back(day==3 ? 1 : 0);
	train_entry.push_back(day==4 ? 1 : 0);
	train_entry.push_back(day==5 ? 1 : 0);
	train_entry.push_back(day==6 ? 1 : 0);
	
	//hour?
	add_dimensions(train_entry, hour, dim_set[d++]);
	for(int k=0;k<2;k++) train_entry.push_back(hour2==k ? 1 : 0);
	for(int k=0;k<4;k++) train_entry.push_back(hour4==k ? 1 : 0);
	for(int k=0;k<6;k++) train_entry.push_back(hour6==k ? 1 : 0);
	for(int k=0;k<12;k++) train_entry.push_back(hour12==k ? 1 : 0);
	
	//what direction?
	train_entry.push_back(bus.direction_id==-1 ? 1 : 0);
	train_entry.push_back(bus.direction_id==0 ? 1 : 0);
	train_entry.push_back(bus.direction_id==1 ? 1 : 0);
	train_entry.push_back(bus.direction_id>1 ? 1 : 0);
	
	//what destination?
	for(int k=0;k<route_dests.size()+1;k++)
		train_entry.push_back(dest==k ? 1 : 0);

	//what block id num?
	for(int k=0;k<route_block_ids.size()+1;k++)
		train_entry.push_back(block_id_num==k ? 1 : 0);
	
	//what nearby stop num?
	for(int k=0;k<route_stops.size()+1;k++)
		train_entry.push_back(near_stop_num==k ? 1 : 0);
	
	//stops till dest?
	add_dimensions(train_entry, bus.stops_away, dim_set[d++]);
	
	//bus count at that point?
	add_dimensions(train_entry, bus_count, dim_set[d++]);
	
	//weather?
	add_dimensions(train_entry, bus.w_obj.cloudcover, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.humidity, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.precipMM, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.temp_F, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.visibility, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.weatherCode, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.windspeedKmph, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.w_tempMinF, dim_set[d++]);
	add_dimensions(train_entry, bus.w_obj.w_tempMaxF, dim_set[d++]);
	
	//traffic data
	for(int k=0;k<9;k++)
	{
		if(k<bus.node_times.size())
		{
			GoogDirObject &n = bus.node_times[k];
			
			add_dimensions(train_entry, n.duration, dim_set[d++]);
			add_dimensions(train_entry, n.distance, dim_set[d++]);
			add_dimensions(train_entry, n.steps, dim_set[d++]);
		}
		else
		{
			add_dimensions(train_entry, 0, dim_set[d++]);
			add_dimensions(train_entry, 0, dim_set[d++]);
			add_dimensions(train_entry, 0, dim_set[d++]);
		}
	}
	
	//velocity
	//add_dimensions(train_entry, bus.velocity, dim_set[d++]);
	//add_dimensions(train_entry, bus.dist_since_prev, dim_set[d++]);
	//add_dimensions(train_entry, bus.time_since_prev, dim_set[d++]);
	
	//lat greater than / less than
	//train_entry.push_back(bus.lat_gt_prev ? 1 : 0);
	//train_entry.push_back(bus.lat_lt_prev ? 1 : 0);
	//train_entry.push_back(bus.lng_gt_prev ? 1 : 0);
	//train_entry.push_back(bus.lng_lt_prev ? 1 : 0);
	
	int traffic_offset = 0;
	if(bus.node_times.size()) traffic_offset = abs(bus.timestamp - bus.node_times[0].timestamp);
	add_dimensions(train_entry, traffic_offset, dim_set[d++]);
	
	//time till next scheduled stop from this block_id?
	add_dimensions(train_entry, stop_time_diff, dim_set[d++]);
	train_entry.push_back(stop_time_num);
	//add_dimensions(train_entry, stop_time_num);
	
	if(d!=dim_set.size()) printf("build_bus_train_entry: bad dim_set size - %lu vs d:%d \n", dim_set.size(), d);
	
	if(printf_php)
	{
		printf("\n");
		printf("$bus['lat'] = %lf;\n", bus.lat);
		printf("$bus['lng'] = %lf;\n", bus.lng);
		printf("$bus['label'] = %d;\n", bus.label);
		printf("$bus['VehicleID'] = %d;\n", bus.vehicle_id);
		printf("$bus['BlockID'] = %d;\n", bus.block_id);
		printf("$bus['Direction'] = \"%s\";\n", bus.direction.c_str());
		printf("$bus['destination'] = \"%s\";\n", bus.destination.c_str());
		printf("$bus['Offset'] = %d;\n", bus.offset);
		printf("$bus['Offset_sec'] = %d;\n", bus.offset_sec);
		printf("$bus['RouteID'] = \"%s\";\n", bus.route_id.c_str());
		printf("$bus['bus_count'] = %d;\n", bus.bus_count);
		printf("$bus['direction_id'] = %d;\n", bus.direction_id);
		printf("$bus['timestamp'] = time();//1434328403;\n");
		printf("$bus['nearest_stop'] = %d;\n", bus.nearest_stop_id);
		printf("$bus['stops_away'] = %d;\n", bus.stops_away);
		printf("\n");
		
		printf("$weather['cloudcover'] = %d;\n", bus.w_obj.cloudcover);
		printf("$weather['humidity'] = %d;\n", bus.w_obj.humidity);
		printf("$weather['precipMM'] = %lf;\n", bus.w_obj.precipMM);
		printf("$weather['temp_F'] = %d;\n", bus.w_obj.temp_F);
		printf("$weather['visibility'] = %d;\n", bus.w_obj.visibility);
		printf("$weather['weatherCode'] = %d;\n", bus.w_obj.weatherCode);
		printf("$weather['windspeedKmph'] = %d;\n", bus.w_obj.windspeedKmph);
		printf("$weather['tempMaxF'] = %d;\n", bus.w_obj.w_tempMaxF);
		printf("$weather['tempMinF'] = %d;\n", bus.w_obj.w_tempMinF);
		printf("$weather['timestamp'] = time();//1434328409;\n");
		printf("\n");
		
		printf("$day = %d;\n", day);
		printf("$hour = %d;\n", hour);
		printf("$min = %d;\n", min);
		printf("\n");
	}
}

void create_train_data(vector< vector< double > > &train_data, vector< vector<BusObject> > &bus_trips, 
							  StopObject &stop, set<string> &stop_dests, vector<int> &route_block_ids, RoutePerfObject &perf_obj,
							  map<int, int> &perf_block_id_trips, map<int, int> &perf_block_id_discarded_trips)
{
	static bool output_first_bus = true;
	
	train_data.clear();
	
	for(int i=0;i<bus_trips.size();i++)
	{
		vector<BusObject> &trip = bus_trips[i];
		
		//check
		if(!trip.size()) continue;
		
		bool bus_found = false;
		BusObject the_stop_bus;
		//int prev_stops_away;
		
		//int z=0;
		for(int j=trip.size()-1;j>=0;j--)
		{
			BusObject &bus = trip[j];
			//z++;
			
			//will set the bus's direction_id and stops_away
			set_bus_stops_away(bus, stop);
			
			//is good bus data?
			if(!bus_is_good(bus, stop, route_block_ids, perf_obj))
			{
				perf_obj.data_points_discarded++;
				
				//also the beginning of a data trip?
				if(bus_at_stop(bus, stop))
				{
					fix_dest_str(bus.destination);
				
					if(stop_dests.find(bus.destination) != stop_dests.end())
					{
						if(perf_block_id_trips.find(bus.block_id) == perf_block_id_trips.end()) perf_block_id_trips[bus.block_id] = 0;
						if(perf_block_id_discarded_trips.find(bus.block_id) == perf_block_id_discarded_trips.end()) perf_block_id_discarded_trips[bus.block_id] = 0;
						
						perf_obj.data_trips_discarded++;
						perf_block_id_discarded_trips[bus.block_id]++;
					}
				}
				
				continue;
			}
			
			//check if bus at stop
			if(bus_at_stop(bus, stop))
			{
				//some chars are bugged
				fix_dest_str(bus.destination);
				
				//make sure this bus is going in a direction supported by this stop
				if(stop_dests.find(bus.destination) != stop_dests.end())
				{
					//prev_stops_away = bus.stops_away;
					//printf("bus_at_stop - stops_away:%d nearest_stop_id:%d \n", bus.stops_away, bus.nearest_stop_id);
					
					//printf("bus_at_stop - '%s' '%s' %lfx%lf t:%d %d ... %d:%d:%d \n", bus.direction.c_str(), bus.destination.c_str(), bus.lat, bus.lng, bus.timestamp, bus.block_id, i, j, z);
					bus_found = true;
					the_stop_bus = bus;
					//continue;
					
					//increment performance block_id trips
					if(perf_block_id_trips.find(bus.block_id) == perf_block_id_trips.end()) perf_block_id_trips[bus.block_id] = 0;
					if(perf_block_id_discarded_trips.find(bus.block_id) == perf_block_id_discarded_trips.end()) perf_block_id_discarded_trips[bus.block_id] = 0;
					
					perf_block_id_trips[bus.block_id]++;
				}
			}
			
			//don't make data with this bus if it
			//isn't following a bus found at a stop
			if(!bus_found) continue;

			//an attempt to use data up until it is at the starting stop again
			//if(bus.stops_away < prev_stops_away && prev_stops_away - bus.stops_away > 10)
			//{
			//	printf("bus loop ended - stops_away:%d<%d time:%d \n", bus.stops_away, prev_stops_away, the_stop_bus.timestamp - bus.timestamp);
			//	
			//	bus_found = false;
			//	continue;
			//}
			//prev_stops_away = bus.stops_away;
			
			//don't train against busses more than 30 minutes away
			//from the stop
			if(the_stop_bus.timestamp - bus.timestamp > 30 * 60)
			{
				//printf("bus_at_end - '%s' '%s' %lfx%lf t:%d %d ... %d:%d:%d \n", bus.direction.c_str(), bus.destination.c_str(), bus.lat, bus.lng, bus.timestamp, bus.block_id, i, j, z);
				bus_found = false;
				continue;
			}
			
			vector<double> train_entry;
			
			build_bus_train_entry(bus, stop, train_entry, output_first_bus);
			
			//printout only first bus info
			output_first_bus = false;
			
			//value to learn
			train_entry.insert(train_entry.begin(), (the_stop_bus.timestamp - the_stop_bus.offset_sec) - (bus.timestamp - bus.offset_sec));
			
			train_data.push_back(train_entry);
			
			//performance recording
			if(the_stop_bus.timestamp == bus.timestamp)
			{
				int day, hour, min;
				set_day_hour_min_from_timestamp(bus.timestamp, day, hour, min);
				
				perf_obj.data_trips++;
				if(day>=0 && day<7) perf_obj.day_count[day]++;
				else printf("create_train_data: error !(day>=0 && day<7)\n");
				if(hour>=0 && hour<24) perf_obj.hour_count[hour]++;
				else printf("create_train_data: error !(hour>=0 && hour<24)\n");
			}
		}
	}
}

string to_stringe(double val)
{
	char buff[1024];
	
	snprintf(buff, 1024, "%e", val);
	
	return buff;
}
