#ifndef GOOGLE_DIRECTIONS_H
#define GOOGLE_DIRECTIONS_H

#include <vector>
#include <string>
#include <set>
#include <map>

using namespace std;

class StopObject;
class BusObject;
class GoogDirObject;

class GoogDir
{
public:
	GoogDir();
	~GoogDir();
	
	void doProcess();
	void makeRouteList(vector<BusObject> &bus_list);
	int getRouteLastTimestamp(const string &route_id);
	void setRouteID(string _route_id, bool _route_all);
	bool processCallData(string &call_data, vector<GoogDirObject> &node_times, bool &limit_reached);
	bool makeGoogleCall(vector<StopObject> &stops, string &call_data);
	bool getTrafficDataForRoute(string route_id, vector<GoogDirObject> &node_times, bool &limit_reached);
	bool shouldGetFrontEndData(string route_id);
	
	void testRoute(string route_id);
	
	bool to_front_end;
	bool restrict_route_all;
	string restrict_route_id;
	set<string> route_list;
	map< string, int > last_route_timestamp;
	map< string, vector<GoogDirObject> > last_route_node_times;
	
	static GoogDir* getInstance();
	static GoogDir* instance;
};

#endif
