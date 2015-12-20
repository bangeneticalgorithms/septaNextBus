#include "performance_recorder.h"
#include "mysql_db.h"
#include "objects.h"
#include "functions.h"

#include <stdio.h>
#include <time.h>

PerfRec* PerfRec::instance = NULL;

PerfRec::PerfRec()
{
	
}

PerfRec::~PerfRec()
{

}

PerfRec* PerfRec::getInstance()
{
	if(!instance) instance = new PerfRec();
	
	return instance;
}

void PerfRec::storeRoutePerformance(string route_id, int stop_id, RoutePerfObject &perf_obj)
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	myobj->deleteRoutePerformance(route_id, stop_id);
	myobj->insertRoutePerformance(route_id, stop_id, perf_obj);
}

void PerfRec::storeRouteModelApply(string route_id, int stop_id, vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results, int train_timestamp)
{
	if(!train_data.size() || !train_data[0].size())
	{
		printf("PerfRec::storeRouteModelApply: bad train_data\n");
		return;
	}
	
	MysqlDB *myobj = MysqlDB::getInstance();
	vector<CoeffObject> coeff_list;
	
	//build coeff_list
	{
		CoeffObject new_coeff;
		new_coeff.theta = theta_results[0];
		new_coeff.mu = 0;
		new_coeff.sigma = 0;
		coeff_list.push_back(new_coeff);
		
		for(int i=0;i<mu_results.size();i++)
		{
			CoeffObject new_coeff;
			new_coeff.theta = theta_results[i+1];
			new_coeff.mu = mu_results[i];
			new_coeff.sigma = sigma_results[i];
			coeff_list.push_back(new_coeff);
		}
	}
	
	myobj->deleteRouteApplyModel(route_id, stop_id);
	
	//make predictions
	for(int i=0;i<train_data.size();i++)
	{
		vector<double> train_entry = train_data[i];
		
		train_entry.erase(train_entry.begin());
		
		BusObject bus;
		bus.timestamp = time(0);
		double prediction = get_prediction(bus, train_entry, coeff_list);
		
		myobj->insertRouteApplyModel(route_id, stop_id, i, train_data[i][0], prediction, train_timestamp);
		
		//printf("PerfRec::storeRouteModelApply: route_id:'%s' stop_id:%d i:%05d actual vs predicted: %lf vs %lf\n", 
		//		 route_id.c_str(), stop_id, i, train_data[i][0], prediction);
	}
}

void PerfRec::storeRouteCostGraph(string route_id, int stop_id, vector<double> &cost_graph, int train_timestamp)
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	myobj->deleteRouteCostGraph(route_id, stop_id);
	myobj->insertRouteCostGraph(route_id, stop_id, cost_graph, train_timestamp);
}

void PerfRec::storeRouteBlockIdTrips(string route_id, int stop_id, map<int, int> &perf_block_id_trips, map<int, int> &perf_block_id_discarded_trips, int train_timestamp)
{
	map<int, bool> perf_block_id_in;
	
	//build perf_block_id_in
	set<int> &route_stop_block_ids = gtfs_route_stop_block_ids(route_id, stop_id);
	
	for(auto iter=perf_block_id_trips.begin();iter!=perf_block_id_trips.end();iter++)
	{
		if(route_stop_block_ids.find(iter->first)!=route_stop_block_ids.end())
			perf_block_id_in[iter->first] = true;
		else
			perf_block_id_in[iter->first] = false;
	}
	
	MysqlDB *myobj = MysqlDB::getInstance();
	
	myobj->deleteRouteBlockIdTrips(route_id, stop_id);
	myobj->insertRouteBlockIdTrips(route_id, stop_id, perf_block_id_trips, perf_block_id_discarded_trips, perf_block_id_in, train_timestamp);
}


