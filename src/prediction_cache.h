#ifndef PREDICTION_CACHE_H
#define PREDICTION_CACHE_H

#include "objects.h"

#include <vector>

using namespace std;

class WeatherObject;
class BusObject;

class PredictionCache
{
public:
	PredictionCache();
	~PredictionCache();
	
	void setWeather(WeatherObject &new_w_obj);
	void processBusList(vector<BusObject> &bus_list);
	void processBusStop(BusObject &bus, StopObject &stop);
	bool busIsNew(BusObject &bus);
	void setRouteID(string _route_id, bool _route_all);
	void setStopID(int _stop_id, bool _stop_all);
	
	static PredictionCache* getInstance();
	static PredictionCache* instance;
	
	WeatherObject w_obj;
	bool route_all;
	bool stop_all;
	string route_id;
	int stop_id;
};

#endif
