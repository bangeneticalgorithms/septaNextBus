#ifndef PERFORMANCE_RECORDER_H
#define PERFORMANCE_RECORDER_H

#include <string>
#include <vector>
#include <map>

using namespace std;

class RoutePerfObject;

class PerfRec
{
    public:
        PerfRec();
        ~PerfRec();

        void storeRoutePerformance(string route_id, int stop_id, RoutePerfObject &perf_obj);
        void storeRouteModelApply(string route_id, int stop_id, vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results, int train_timestamp);
        void storeRouteCostGraph(string route_id, int stop_id, vector<double> &cost_graph, int train_timestamp);
        void storeRouteBlockIdTrips(string route_id, int stop_id, map<int, int> &perf_block_id_trips, map<int, int> &perf_block_id_discarded_trips, int train_timestamp);

        static PerfRec* getInstance();
        static PerfRec* instance;
};

#endif
