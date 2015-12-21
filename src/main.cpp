#include "functions.h"
#include "arguments.h"
#include "objects.h"
#include "gtfs.h"
#include "mysql_db.h"
#include "weather.h"
#include "settings.h"
#include "prediction_cache.h"
#include "google_directions.h"

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	Arguments args;
	
	Settings::readSettings();
	
	args.readArguments(argc, argv);
	
	if(args.print_mini_help) args.printMiniHelp();
	if(args.print_help) args.printHelp();
	
	args.printSetup();
	
	//load gtfs databases to mysql
	load_gtfs(args.reload_gtfs);
	
	//collect stop data
	if(args.read_stop_data)
	{
		if(args.route_all)
		{
			//get all the routes
			string call_data = make_curl_call("http://www3.septa.org/hackathon/TransitViewAll/");
			vector<string> route_list;
			parse_bus_data(call_data, &route_list);
			
			//get and store stop data for all routes
			for(int i=0;i<route_list.size();i++)
				get_and_store_stop_data(route_list[i]);
		}
		else
		{
			//store stop data for just the specified route
			get_and_store_stop_data(args.route_id);
		}
	}
	
	//collect bus data
	if(args.read_bus_data)
	{
		MysqlDB *myobj = MysqlDB::getInstance();
		GoogDir *googdir = GoogDir::getInstance();
		map<int, BusObject> old_buses_cache;
		
		//setup google directions api
		googdir->to_front_end = false;
		googdir->setRouteID(args.route_id, args.route_all);
		
		for(int s=0;1;s++)
		{
			//let googdir manage itself
			googdir->doProcess();
			
			//read new weather if past 5 minutes or every 15 seconds check if there is a new update in the front end
			if((abs(time(0) - Weather::getInstance()->last_timestamp) > WEATHER_TIME_REGRAB + 30) 
				|| (s%15 && Weather::getInstance()->newRecentFrontEndWeatherAvailable()))
			{
				if(!Weather::getInstance()->getRecentWeather())
					printf("Weather::getRecentWeather() failed!\n");
			}
			
			//read bus data every 5 seconds
			if(!(s%5))
			{
				stop_watch();
				
				//get all the routes
				string call_data = make_curl_call("http://www3.septa.org/hackathon/TransitViewAll/");
				vector<BusObject> bus_list = parse_bus_data(call_data);
				
				//needed for google directions
				googdir->makeRouteList(bus_list);
				
				//get and store stop data for all routes
				for(int i=0;i<bus_list.size();i++)
				{
					BusObject &bus = bus_list[i];
					
					if(!bus_is_new(bus, old_buses_cache)) continue;
					
					//don't bother with buses 1 second + old
					if(bus.offset) continue;
					
					//this a route we care about?
					if(!args.route_all && bus.route_id != args.route_id) continue;
					
					//make sure weather and googdir timestamp are reasonable
					if(!bus_weather_and_googdir_timestamp_good(bus)) continue;
					
					myobj->insertBusData(bus_list[i]);
				}
				
				stop_watch("read_bus_data");
			}
			
			//pause and grab data again
			sleep(1);
		}
	}
	
	//create coefficients
	if(args.create_coeff_data)
	{
		MysqlDB *myobj = MysqlDB::getInstance();
		
		if(args.route_all)
		{
			//get all the routes
			string call_data = make_curl_call("http://www3.septa.org/hackathon/TransitViewAll/");
			vector<string> route_list = myobj->getBusDataRoutes();
			parse_bus_data(call_data);
			
			//create_coefficient_data for all routes
			for(int i=0;i<route_list.size();i++)
				create_coefficient_data(route_list[i], args.stop_all, args.stop_id, args.iterations, args.retrain_time);
		}
		else
		{
			//create_coefficient_data for just the specified route
			create_coefficient_data(args.route_id, args.stop_all, args.stop_id, args.iterations, args.retrain_time);
		}
	}
	
	//keep front end php scripts up to date with bus / weather data
	if(args.optimize_front_end)
	{
		MysqlDB *myobj = MysqlDB::getInstance();
		GoogDir *googdir = GoogDir::getInstance();
		PredictionCache *pcache = PredictionCache::getInstance();
		
		pcache->setRouteID(args.route_id, args.route_all);
		pcache->setStopID(args.stop_id, args.stop_all);
		
		//setup google directions api
		googdir->to_front_end = true;
		googdir->setRouteID(args.route_id, args.route_all);
		
		//clear out old bus predictions - 
		myobj->clearPredictions();
		
		for(int s=0;1;s++)
		{
			//let googdir manage itself
			googdir->doProcess();
			
			//read weather data every 5 minutes
			//if(!(s%(WEATHER_TIME_REGRAB)))
			if(abs(time(0) - Weather::getInstance()->last_timestamp) > WEATHER_TIME_REGRAB) 
			{
				WeatherObject w_obj;
				
				if(Weather::getInstance()->getRecentWeather(w_obj, false))
				{
					pcache->setWeather(w_obj);
					
					if(!myobj->insertWeatherFrontEnd(w_obj))
						printf("Weather::getRecentWeather() failed!\n");
				}
				else
					printf("Weather::getRecentWeather() failed!\n");
			}
			
			//read bus data every 5 seconds
			if(!(s%5))
			{
				stop_watch();
				
				//get all the routes
				string call_data = make_curl_call("http://www3.septa.org/hackathon/TransitViewAll/");
				vector<BusObject> bus_list = parse_bus_data(call_data);
				
				//needed for google directions
				googdir->makeRouteList(bus_list);
				
				//clear old stored bus data
				myobj->clearBusFrontEnd();
				
				//get and store stop data for all routes
				for(int i=0;i<bus_list.size();i++)
				{
					BusObject &bus = bus_list[i];
					
					if(args.route_all || bus.route_id == args.route_id)
						myobj->insertBusData(bus, true);
				}
				
				stop_watch("read_bus_data");
				
				stop_watch();
				pcache->processBusList(bus_list);
				stop_watch("cache_bus_predictions");
			}
			
			//pause and grab data again
			sleep(1);
		}
	}
	
	return 0;
}
