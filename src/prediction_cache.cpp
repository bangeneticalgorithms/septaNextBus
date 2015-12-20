#include "prediction_cache.h"
#include "functions.h"
#include "mysql_db.h"
#include "google_directions.h"

#include <time.h>

PredictionCache* PredictionCache::instance = NULL;

PredictionCache::PredictionCache()
{
	stop_id = 0;
	route_all = true;
	stop_all = true;
}

PredictionCache::~PredictionCache()
{
	
}

PredictionCache* PredictionCache::getInstance()
{
	if(!instance) instance = new PredictionCache();
	
	return instance;
}

void PredictionCache::setWeather(WeatherObject &new_w_obj)
{
	w_obj = new_w_obj;
}

void PredictionCache::processBusStop(BusObject &bus, StopObject &stop)
{
	vector<double> train_entry;
	
	build_bus_train_entry(bus, stop, train_entry);
	
	vector<CoeffObject> &coeff_list = get_coefficients(bus.route_id, stop.stop_id);
	
	if(get_prediction_check(train_entry, coeff_list))
	{
		double prediction = get_prediction(bus, train_entry, coeff_list);
		
		//printout_debug_array("train", train_entry);
		//printout_debug_array("coeff", coeff_list);
		
		printf("PredictionCache::processBusStop:: bus_id:%d route:'%s' stop:%d prediction:%lf \n", 
				 bus.vehicle_id, bus.route_id.c_str(), stop.stop_id, prediction);
		
		MysqlDB *myobj = MysqlDB::getInstance();
		
		myobj->deletePrediction(bus.vehicle_id, stop.stop_id);
		myobj->insertPrediction(bus.vehicle_id, stop.stop_id, prediction);
	}
	else
		printf("PredictionCache::processBusStop:: !get_prediction_check [%lu vs %lu] route:'%s' stop:%d \n", 
				 train_entry.size(), coeff_list.size(), bus.route_id.c_str(), stop.stop_id);
}

bool PredictionCache::busIsNew(BusObject &bus)
{
	static map<int, BusObject> old_buses_cache;

	return bus_is_new(bus, old_buses_cache);
}

void PredictionCache::setRouteID(string _route_id, bool _route_all)
{
	route_id = _route_id;
	route_all = _route_all;
}

void PredictionCache::setStopID(int _stop_id, bool _stop_all)
{
	stop_id = _stop_id;
	stop_all = _stop_all;
}

void PredictionCache::processBusList(vector<BusObject> &bus_list)
{
	//weather recent within 30 minutes?
	if(abs(time(0) - w_obj.timestamp) > WEATHER_TIME_GOOD)
	{
		printf("PredictionCache::processBusList() weather not recent!\n");
		return;
	}
	
	for(int i=0;i<bus_list.size();i++)
	{
		BusObject &bus = bus_list[i];
		
		//were we told to only do one route?
		if(!route_all && bus.route_id != route_id) continue;
		
		//google directions recent within 45 minutes?
		if(abs(time(0) - GoogDir::getInstance()->getRouteLastTimestamp(bus.route_id)) > GOOGDIR_TIME_GOOD)
		{
			static string last_err_route;
			
			//don't repeat this error
			if(last_err_route != bus.route_id)
			{
				printf("PredictionCache::processBusList() google directions not recent for route:'%s'!\n", bus.route_id.c_str());
				last_err_route = bus.route_id;
			}
			
			continue;
		}
		
		//is the bus data actually new?
		if(!busIsNew(bus)) continue;
		
		vector<StopObject> &stops = gtfs_route_stops(bus.route_id);
		
		bus.w_obj = w_obj;
		bus.node_times = GoogDir::getInstance()->last_route_node_times[bus.route_id];
		
		for(int i=0;i<stops.size();i++)
		{
			StopObject &s = stops[i];
			
			if(stop_all || s.stop_id == stop_id)
				processBusStop(bus, s);
		}
	}
}
