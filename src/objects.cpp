#include "objects.h"

BusObject &BusObject::operator=(const BusObject &b)
{
	if(this == &b) return *this;
	
	ID = b.ID;
	route_id = b.route_id;
	block_id = b.block_id;
	offset = b.offset;
	offset_sec = b.offset_sec;
	vehicle_id = b.vehicle_id;
	label = b.label;
	weather_timestamp = b.weather_timestamp;
	googdir_timestamp = b.googdir_timestamp;
	timestamp = b.timestamp;
	bus_count = b.bus_count;
	lat = b.lat;
	lng = b.lng;
	direction = b.direction;
	destination = b.destination;
	direction_id = b.direction_id;
	nearest_stop_id = b.nearest_stop_id;
	stops_away = b.stops_away;
	velocity = b.velocity;
	time_since_prev = b.time_since_prev;
	dist_since_prev = b.dist_since_prev;
	lat_gt_prev = b.lat_gt_prev;
	lat_lt_prev = b.lat_lt_prev;
	lng_gt_prev = b.lng_gt_prev;
	lng_lt_prev = b.lng_lt_prev;
	
	w_obj = b.w_obj;
	node_times = b.node_times;
	
	return *this;
}

void BusObject::clear()
{
	ID = 0;
	block_id = 0;
	offset = 0;
	offset_sec = 0;
	vehicle_id = 0;
	label = 0;
	weather_timestamp = 0;
	googdir_timestamp = 0;
	timestamp = 0;
	bus_count = 0;
	lat = 0;
	lng = 0;
	route_id = "";
	direction = "";
	destination = "";
	direction_id = -1;
	nearest_stop_id = -1;
	stops_away = -1;
	velocity = 0;
	time_since_prev = 0;
	dist_since_prev = 0;
	lat_gt_prev = false;
	lat_lt_prev = false;
	lng_gt_prev = false;
	lng_lt_prev = false;
	
	w_obj.clear();
	node_times.clear();
}

BusObject::BusObject(Json::Value &json_obj, string _route_id, int _timestamp, int _weather_timestamp, int _googdir_timestamp, int _bus_count)
{
	Json::Value member;
	
	//set to defaults
	clear();
	
	if(!json_obj.isObject()) return;
	
	route_id = _route_id;
	timestamp = _timestamp;
	bus_count = _bus_count;
	weather_timestamp = _weather_timestamp;
	googdir_timestamp = _googdir_timestamp;
	
	member = json_obj["BlockID"];
	if(!member.isNull() && member.isInt()) block_id = member.asInt();
	
	member = json_obj["Offset"];
	if(!member.isNull() && member.isInt()) offset = member.asInt();
	
	member = json_obj["Offset_sec"];
	if(!member.isNull() && member.isInt()) offset_sec = member.asInt();
	
	member = json_obj["VehicleID"];
	if(!member.isNull() && member.isInt()) vehicle_id = member.asInt();
	
	member = json_obj["label"];
	if(!member.isNull() && member.isInt()) label = member.asInt();
	
	member = json_obj["lat"];
	if(!member.isNull() && member.isDouble()) lat = member.asDouble();
	
	member = json_obj["lng"];
	if(!member.isNull() && member.isDouble()) lng = member.asDouble();
	
	member = json_obj["Direction"];
	if(!member.isNull() && member.isString()) direction = member.asString();
	
	member = json_obj["destination"];
	if(!member.isNull() && member.isString()) destination = member.asString();
}

void BusObject::debug()
{
	printf("BusObject::debug: %p \n", this);
	printf("BusObject::debug: route_id:'%s' \n", route_id.c_str());
	printf("BusObject::debug: block_id:%d \n", block_id);
	printf("BusObject::debug: vehicle_id:%d \n", vehicle_id);
	printf("BusObject::debug: offset:%d \n", offset);
	printf("BusObject::debug: offset_sec:%d \n", offset_sec);
	printf("BusObject::debug: label:%d \n", label);
	printf("BusObject::debug: weather_timestamp:%d \n", weather_timestamp);
	printf("BusObject::debug: googdir_timestamp:%d \n", googdir_timestamp);
	printf("BusObject::debug: timestamp:%d \n", timestamp);
	printf("BusObject::debug: bus_count:%d \n", bus_count);
	printf("BusObject::debug: lat:%lf \n", lat);
	printf("BusObject::debug: lng:%lf \n", lng);
	printf("BusObject::debug: direction:'%s' \n", direction.c_str());
	printf("BusObject::debug: destination:'%s' \n", destination.c_str());
	printf("BusObject::debug: direction_id:%d \n", direction_id);
	printf("BusObject::debug: nearest_stop_id:%d \n", nearest_stop_id);
	printf("BusObject::debug: stops_away:%d \n", stops_away);
	printf("BusObject::debug: velocity:%lf \n", velocity);
	printf("BusObject::debug: time_since_prev:%d \n", time_since_prev);
	printf("BusObject::debug: dist_since_prev:%lf \n", dist_since_prev);
	printf("BusObject::debug: lat_gt_prev:%d \n", lat_gt_prev);
	printf("BusObject::debug: lat_lt_prev:%d \n", lat_lt_prev);
	printf("BusObject::debug: lng_gt_prev:%d \n", lng_gt_prev);
	printf("BusObject::debug: lng_lt_prev:%d \n", lng_lt_prev);
	
	w_obj.debug();
	for(int i=0;i<node_times.size();i++)
		node_times[i].debug();
}

StopObject::StopObject(Json::Value &json_obj, string _route_id)
{
	Json::Value member;
	
	//set to defaults
	clear();
	
	if(!json_obj.isObject()) return;
	
	member = json_obj["stopid"];
	if(!member.isNull() && member.isInt()) stop_id = member.asInt();
	
	member = json_obj["lat"];
	if(!member.isNull() && member.isDouble()) lat = member.asDouble();
	
	member = json_obj["lng"];
	if(!member.isNull() && member.isDouble()) lng = member.asDouble();
	
	member = json_obj["stopname"];
	if(!member.isNull() && member.isString()) stopname = member.asString();
	
	route_id = _route_id;
}
	
void StopObject::clear()
{
	ID = 0;
	stop_id = 0;
	lat = 0;
	lng = 0;
	stopname = "";
	route_id = "";
	direction_id = -1;
}

void StopObject::debug()
{
	printf("StopObject::debug: %p \n", this);
	printf("StopObject::debug: stop_id:%d \n", stop_id);
	printf("StopObject::debug: lat:%lf \n", lat);
	printf("StopObject::debug: lng:%lf \n", lng);
	printf("StopObject::debug: stopname:'%s' \n", stopname.c_str());
	printf("StopObject::debug: route_id:'%s' \n", route_id.c_str());
	printf("StopObject::debug: direction_id:%d \n", direction_id);
}

StopObject &StopObject::operator=(const StopObject &s)
{
	if(this == &s) return *this;
	
	ID = s.ID;
	stop_id = s.stop_id;
	lat = s.lat;
	lng = s.lng;
	stopname = s.stopname;
	route_id = s.route_id;
	direction_id = s.direction_id;
	
	return *this;
}

void WeatherObject::clear()
{
	cloudcover = 0;
	humidity = 0;
	precipMM = 0;
	pressure = 0;
	temp_C = 0;
	temp_F = 0;
	visibility = 0;
	weatherCode = 0;
	weatherDesc = "";
	winddir16Point = "";
	winddirDegree = 0;
	windspeedKmph = 0;
	windspeedMiles = 0;
	
	w_precipMM = 0;
	w_tempMaxC = 0;
	w_tempMaxF = 0;
	w_tempMinC = 0;
	w_tempMinF = 0;
	w_weatherCode = 0;
	w_weatherDesc = "";
	w_winddir16Point = "";
	w_winddirDegree = 0;
	w_winddirection = "";
	w_windspeedKmph = 0;
	w_windspeedMiles = 0;
	
	timestamp = 0;
}

WeatherObject &WeatherObject::operator=(const WeatherObject &b)
{
	if(this == &b) return *this;
	
	cloudcover = b.cloudcover;
	humidity = b.humidity;
	precipMM = b.precipMM;
	pressure = b.pressure;
	temp_C = b.temp_C;
	temp_F = b.temp_F;
	visibility = b.visibility;
	weatherCode = b.weatherCode;
	weatherDesc = b.weatherDesc;
	winddir16Point = b.winddir16Point;
	winddirDegree = b.winddirDegree;
	windspeedKmph = b.windspeedKmph;
	windspeedMiles = b.windspeedMiles;
	
	w_precipMM = b.w_precipMM;
	w_tempMaxC = b.w_tempMaxC;
	w_tempMaxF = b.w_tempMaxF;
	w_tempMinC = b.w_tempMinC;
	w_tempMinF = b.w_tempMinF;
	w_weatherCode = b.w_weatherCode;
	w_weatherDesc = b.w_weatherDesc;
	w_winddir16Point = b.w_winddir16Point;
	w_winddirDegree = b.w_winddirDegree;
	w_winddirection = b.w_winddirection;
	w_windspeedKmph = b.w_windspeedKmph;
	w_windspeedMiles = b.w_windspeedMiles;
	
	timestamp = b.timestamp;
	
	return *this;
}

void WeatherObject::debug()
{
	printf("WeatherObject::debug: %p \n", this);
	printf("WeatherObject::debug: cloudcover:%d \n", cloudcover);
	printf("WeatherObject::debug: humidity:%d \n", humidity);
	printf("WeatherObject::debug: precipMM:%lf \n", precipMM);
	printf("WeatherObject::debug: pressure:%d \n", pressure);
	printf("WeatherObject::debug: temp_C:%d \n", temp_C);
	printf("WeatherObject::debug: temp_F:%d \n", temp_F);
	printf("WeatherObject::debug: visibility:%d \n", visibility);
	printf("WeatherObject::debug: weatherCode:%d \n", weatherCode);
	printf("WeatherObject::debug: weatherDesc:'%s' \n", weatherDesc.c_str());
	printf("WeatherObject::debug: winddir16Point:'%s' \n", winddir16Point.c_str());
	printf("WeatherObject::debug: winddirDegree:%d \n", winddirDegree);
	printf("WeatherObject::debug: windspeedKmph:%d \n", windspeedKmph);
	printf("WeatherObject::debug: windspeedMiles:%d \n", windspeedMiles);
	
	printf("WeatherObject::debug: w_precipMM:%lf \n", w_precipMM);
	printf("WeatherObject::debug: w_tempMaxC:%d \n", w_tempMaxC);
	printf("WeatherObject::debug: w_tempMaxF:%d \n", w_tempMaxF);
	printf("WeatherObject::debug: w_tempMinC:%d \n", w_tempMinC);
	printf("WeatherObject::debug: w_tempMinF:%d \n", w_tempMinF);
	printf("WeatherObject::debug: w_weatherCode:%d \n", w_weatherCode);
	printf("WeatherObject::debug: w_weatherDesc:'%s' \n", w_weatherDesc.c_str());
	printf("WeatherObject::debug: w_winddir16Point:'%s' \n", w_winddir16Point.c_str());
	printf("WeatherObject::debug: w_winddirDegree:%d \n", w_winddirDegree);
	printf("WeatherObject::debug: w_winddirection:'%s' \n", w_winddirection.c_str());
	printf("WeatherObject::debug: w_windspeedKmph:%d \n", w_windspeedKmph);
	printf("WeatherObject::debug: w_windspeedMiles:%d \n", w_windspeedMiles);
	
	printf("WeatherObject::debug: timestamp:%d \n", timestamp);
}

void CoeffObject::clear()
{
	theta = 0;
	mu = 0;
	sigma = 0;
}

void CoeffObject::debug()
{
	printf("CoeffObject::debug: theta:%lf \n", theta);
	printf("CoeffObject::debug: mu:%lf \n", mu);
	printf("CoeffObject::debug: sigma:%lf \n", sigma);
}

CoeffObject &CoeffObject::operator=(const CoeffObject &b)
{
	if(this == &b) return *this;
	
	theta = b.theta;
	mu = b.mu;
	sigma = b.sigma;
	
	return *this;
}

void GoogDirObject::clear()
{
	distance = 0;
	duration = 0;
	steps = 0;
	timestamp = 0;
}

void GoogDirObject::debug()
{
	printf("GoogDirObject::debug: distance:%d \n", distance);
	printf("GoogDirObject::debug: duration:%d \n", duration);
	printf("GoogDirObject::debug: steps:%d \n", steps);
	printf("GoogDirObject::debug: timestamp:%d \n", timestamp);
}

GoogDirObject &GoogDirObject::operator=(const GoogDirObject &o)
{
	if(this == &o) return *this;
	
	distance = o.distance;
	duration = o.duration;
	steps = o.steps;
	timestamp = o.timestamp;
	
	return *this;
}

void RoutePerfObject::clear()
{
	data_points = 0;
	data_trips = 0;
	used_features = 0;
	total_features = 0;
	data_points_discarded = 0;
	data_trips_discarded = 0;
	discarded_bad_block_id = 0;
	discarded_bad_dest = 0;
	discarded_bad_dir = 0;
	discarded_bad_dir_id = 0;
	discarded_bad_stops_away = 0;
	discarded_bad_node_times = 0;
	discarded_bad_block_id_in = 0;
	discarded_bad_weather_googdir = 0;
	discarded_bad_stop_times = 0;
	iterations = 0;
	train_time = 0;
	mean_squared_error = 0;
	timestamp = 0;
	
	for(int i=0;i<7;i++) day_count[i] = 0;
	for(int i=0;i<24;i++) hour_count[i] = 0;
}

void RoutePerfObject::debug()
{
	printf("GoogDirObject::debug: data_points:%d \n", data_points);
	printf("GoogDirObject::debug: data_trips:%d \n", data_trips);
	printf("GoogDirObject::debug: used_features:%d \n", used_features);
	printf("GoogDirObject::debug: total_features:%d \n", total_features);
	printf("GoogDirObject::debug: data_points_discarded:%d \n", data_points_discarded);
	printf("GoogDirObject::debug: data_trips_discarded:%d \n", data_trips_discarded);
	printf("GoogDirObject::debug: discarded_bad_block_id:%d \n", discarded_bad_block_id);
	printf("GoogDirObject::debug: discarded_bad_dest:%d \n", discarded_bad_dest);
	printf("GoogDirObject::debug: discarded_bad_dir:%d \n", discarded_bad_dir);
	printf("GoogDirObject::debug: discarded_bad_dir_id:%d \n", discarded_bad_dir_id);
	printf("GoogDirObject::debug: discarded_bad_stops_away:%d \n", discarded_bad_stops_away);
	printf("GoogDirObject::debug: discarded_bad_node_times:%d \n", discarded_bad_node_times);
	printf("GoogDirObject::debug: discarded_bad_block_id_in:%d \n", discarded_bad_block_id_in);
	printf("GoogDirObject::debug: discarded_bad_weather_googdir:%d \n", discarded_bad_weather_googdir);
	printf("GoogDirObject::debug: discarded_bad_stop_times:%d \n", discarded_bad_stop_times);
	printf("GoogDirObject::debug: iterations:%d \n", iterations);
	printf("GoogDirObject::debug: train_time:%lf \n", train_time);
	printf("GoogDirObject::debug: mean_squared_error:%lf \n", mean_squared_error);
	printf("GoogDirObject::debug: timestamp:%d \n", timestamp);
	
	printf("GoogDirObject::debug: day_count:");
	for(int i=0;i<7;i++) printf("[%d]:%05d ", i, day_count[i]);
	printf("\n");
	
	printf("GoogDirObject::debug: hour_count:");
	for(int i=0;i<24;i++) printf("[%02d]:%05d ", i, hour_count[i]);
	printf("\n");
}

RoutePerfObject &RoutePerfObject::operator=(const RoutePerfObject &o)
{
	if(this == &o) return *this;
	
	data_points = o.data_points;
	data_trips = o.data_trips;
	used_features = o.used_features;
	total_features = o.total_features;
	data_points_discarded = o.data_points_discarded;
	data_trips_discarded = o.data_trips_discarded;
	discarded_bad_block_id = o.discarded_bad_block_id;
	discarded_bad_dest = o.discarded_bad_dest;
	discarded_bad_dir = o.discarded_bad_dir;
	discarded_bad_dir_id = o.discarded_bad_dir_id;
	discarded_bad_stops_away = o.discarded_bad_stops_away;
	discarded_bad_node_times = o.discarded_bad_node_times;
	discarded_bad_block_id_in = o.discarded_bad_block_id_in;
	discarded_bad_weather_googdir = o.discarded_bad_weather_googdir;
	discarded_bad_stop_times = o.discarded_bad_stop_times;
	iterations = o.iterations;
	train_time = o.train_time;
	mean_squared_error = o.mean_squared_error;
	timestamp = o.timestamp;
	
	for(int i=0;i<7;i++) day_count[i] = o.day_count[i];
	for(int i=0;i<24;i++) hour_count[i] = o.hour_count[i];
	
	return *this;
}


