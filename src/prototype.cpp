#include <stdio.h>
#include <iostream>
#include <string>
#include <curl/curl.h> //your directory may be different
#include <stdlib.h>
#include <math.h>
#include <map>
#include <vector>
#include <jsoncpp/json/json.h>
#include <string.h>
#include <string>
#include <unistd.h>
#ifdef _WIN32
#include "sqlite3.h"
#else
#include <sqlite3.h>
#endif
using namespace std;

class stop_info
{
public:
	stop_info()
	{
		lat = 0;
		lng = 0;
		stopid = -1;
	}
	double lat;
	double lng;
	int stopid;
	string stopname;

	stop_info &operator=(const stop_info &rhs)
	{
		lat = rhs.lat;
		lng = rhs.lng;
		stopid = rhs.stopid;
		stopname = rhs.stopname;
	}
};

class bus_loc_info
{
public:
	bus_loc_info()
	{
		lat = 0;
		lng = 0;
		label = -1;
		VehicleID = -1;
		BlockID = -1;
		Offset = -1;
		weather_code = -1;
		timestamp = -1;
	}
	double lat;
	double lng;
	int label;
	int VehicleID;
	int BlockID;
	int Offset;
	string Direction;
	string destination;
	int weather_code;
	string weather_desc;
	int timestamp;

	bus_loc_info &operator=(const bus_loc_info &rhs)
	{
		lat = rhs.lat;
		lng = rhs.lng;
		label = rhs.label;
		VehicleID = rhs.VehicleID;
		BlockID = rhs.BlockID;
		Offset = rhs.Offset;
		Direction = rhs.Direction;
		destination = rhs.destination;
		weather_code = rhs.weather_code;
		weather_desc = rhs.weather_desc;
		timestamp = rhs.timestamp;
	}
};

string data; //will hold the url's contents
sqlite3 *db = 0;
string weather_desc = "";
int weather_code = -1;
map<int,vector<stop_info> > stop_map;
map<int,map<int,vector<vector<bus_loc_info> > > > bus_map; //bus_map[route_number][bus label][trip_number]

size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up);
int normal_callback(void *NotUsed, int argc, char **argv, char **azColName);
void parse_json(int route_number);
bool load_database(string filename);
bool make_database_call(string input, int (*callback)(void *, int , char **, char **) = normal_callback, void* callback_input = NULL);
void get_data_for_route(int route_number);
void parse_weather();
void get_weather();
void collect_data();
void convert_data();
void get_stops(int route_number);
void parse_stops(int route_number);
void load_stops(int route_number);
int load_stops_callback(void *vfp, int argc, char **argv, char **azColName);
void debug_stop_list(int rn);
double distance(double x1, double y1, double x2, double y2);
double distance(stop_info &s, double lat, double lng);
void find_closest_rn_stops(int rn);
stop_info &find_closest_stop(int rn, double lat, double lng);
int load_bus_locs_callback(void *vfp, int argc, char **argv, char **azColName);
void load_bus_locs(int rn);
void debug_bus_locs(int rn);
void build_ml_data(int rn);
int timestamp_to_seg(int timestamp);
int load_weather_types_callback(void *vrn, int argc, char **argv, char **azColName);
void weather_types();

double distance(double x1, double y1, double x2, double y2)
{
	double xdiff = x1-x2;
	double ydiff = y1-y2;

	return pow(xdiff*xdiff + ydiff*ydiff, 0.5);
}

double distance(stop_info &s, double lat, double lng)
{
	return distance(s.lat, s.lng, lat, lng);
}

int load_weather_types_callback(void *vrn, int argc, char **argv, char **azColName)
{
	if(!vrn) return 0;

	int rn = *(int*)vrn;
	stop_info new_stop;

	for(int i=0; i<argc; i++)
	{
		if(!argv[i]) continue;

		if(!strcmp(azColName[i], "lat")) new_stop.lat = atof(argv[i]);
		else if(!strcmp(azColName[i], "lng")) new_stop.lng = atof(argv[i]);
		else if(!strcmp(azColName[i], "stopid")) new_stop.stopid = atoi(argv[i]);
		else if(!strcmp(azColName[i], "stopname")) new_stop.stopname = argv[i];
	}

	stop_map[rn].push_back(new_stop);

	return 0;
}

void weather_types()
{

}

void find_closest_rn_stops(int rn)
{
	stop_info *s1 = 0, *s2 = 0;
	double least_distance;

	int len = stop_map[rn].size();

	if(len < 2)
	{
		printf("find_closest_rn_stops:: not enough stops:%d\n", rn);
		return;
	}

	//init
	s1 = &stop_map[rn][0];
	s2 = &stop_map[rn][1];
	least_distance = distance(*s1, s2->lat, s2->lng);

	for(int i=0;i<len;i++)
	{
		stop_info &ts1 = stop_map[rn][i];

		for(int j=0;j<len;j++)
		{
			if(i==j) continue;

			stop_info &ts2 = stop_map[rn][j];

			//if(ts1.lat == ts1.lat && ts1.lng == ts2.lng) continue;

			double dist = distance(ts1, ts2.lat, ts2.lng);
			if(dist<least_distance)
			{
				s1 = &ts1;
				s2 = &ts2;
				least_distance = dist;
			}
		}
	}

	if(!s1 || !s2) return;

	printf("closest stops %lf ... %d:%s --- %d:%s\n", least_distance, s1->stopid, s1->stopname.c_str(), s2->stopid, s2->stopname.c_str());
}

stop_info &find_closest_stop(int rn, double lat, double lng)
{
	static stop_info ret_stop;
	vector<stop_info> &stop_list = stop_map[rn];
	int len = stop_map[rn].size();

	if(!len)
	{
		printf("no stops loaded for %d\n", rn);
		return ret_stop;
	}

	//init
	stop_info *s1 = 0;
	double least_distance;
	s1 = &stop_map[rn][0];
	least_distance = distance(*s1, lat, lng);

	for(int i=0;i<len;i++)
	{
		stop_info &s = stop_list[i];
		double dist = distance(s, lat, lng);

		if(dist < least_distance)
		{
			least_distance = dist;
			s1 = &s;
		}
	}

	printf("find_closest_stop::%d:%s ---- dist:%lf\n", rn, s1->stopname.c_str(), least_distance);

	return *s1;
}

int load_stops_callback(void *vrn, int argc, char **argv, char **azColName)
{
	if(!vrn) return 0;

	int rn = *(int*)vrn;
	stop_info new_stop;

	for(int i=0; i<argc; i++)
	{
		if(!argv[i]) continue;

		if(!strcmp(azColName[i], "lat")) new_stop.lat = atof(argv[i]);
		else if(!strcmp(azColName[i], "lng")) new_stop.lng = atof(argv[i]);
		else if(!strcmp(azColName[i], "stopid")) new_stop.stopid = atoi(argv[i]);
		else if(!strcmp(azColName[i], "stopname")) new_stop.stopname = argv[i];
	}

	stop_map[rn].push_back(new_stop);

	return 0;
}

void debug_stop_list(int rn)
{
	int len = stop_map[rn].size();

	printf("stop length %03d:%d\n", rn, len);
	for(int i=0;i<len;i++)
		printf("stop %03d:%03d:%s\n", rn, i, stop_map[rn][i].stopname.c_str());
}

void load_stops(int rn)
{
	stop_map[rn].clear();

	char insert_str[5000];
	sqlite3_snprintf(5000, insert_str, "select lat,lng,stopid,stopname from stops where route=%d order by stopindex;", rn);
	make_database_call(insert_str, load_stops_callback, (void*)&rn);

	debug_stop_list(rn);
}

int load_bus_locs_callback(void *vrn, int argc, char **argv, char **azColName)
{
	if(!vrn) return 0;

	int rn = *(int*)vrn;
	bus_loc_info new_bus_loc;

	string test;
	for(int i=0; i<argc; i++)
	{
		if(!argv[i]) continue;

		if(!strcmp(azColName[i], "lat")) new_bus_loc.lat = atof(argv[i]);
		else if(!strcmp(azColName[i], "lng")) new_bus_loc.lng = atof(argv[i]);
		else if(!strcmp(azColName[i], "label")) new_bus_loc.label = atoi(argv[i]);
		else if(!strcmp(azColName[i], "VehicleID")) new_bus_loc.VehicleID = atoi(argv[i]);
		else if(!strcmp(azColName[i], "BlockID")) new_bus_loc.BlockID = atoi(argv[i]);
		else if(!strcmp(azColName[i], "Direction")) new_bus_loc.Direction = argv[i];
		else if(!strcmp(azColName[i], "destination")) new_bus_loc.destination = argv[i];
		else if(!strcmp(azColName[i], "Offset")) new_bus_loc.Offset = atoi(argv[i]);
		else if(!strcmp(azColName[i], "weather_code")) new_bus_loc.weather_code = atoi(argv[i]);
		else if(!strcmp(azColName[i], "weather_desc")) new_bus_loc.weather_desc = argv[i];
		else if(!strcmp(azColName[i], "timestamp")) new_bus_loc.timestamp = atoi(argv[i]);

		if(!strcmp(azColName[i], "destination"))
		{
			//printf("new_bus_loc.destination:%s\n", argv[i]);
			//test = argv[i];
		}
	}

	//interesting thing to look for
	if(new_bus_loc.label != new_bus_loc.VehicleID) printf("load_bus_locs_callback::label != VehicleID\n");

	//printf("test:%s\n", test.c_str());
	//printf("new_bus_loc.destination:%s\n", new_bus_loc.destination.c_str());

	//create a new trip?
	if(!bus_map[rn][new_bus_loc.label].size() || new_bus_loc.timestamp - bus_map[rn][new_bus_loc.label].back().back().timestamp > 30 * 60)
		bus_map[rn][new_bus_loc.label].push_back(vector<bus_loc_info>());

	bus_map[rn][new_bus_loc.label].back().push_back(new_bus_loc);

	return 0;
}

void debug_bus_locs(int rn)
{
	int len = bus_map[rn].size();

	printf("buses %03d:%d\n", rn, len);
	for(auto i=bus_map[rn].begin();i!=bus_map[rn].end();++i)
	{
		printf("bus %03d:%07d:%d\n", rn, i->first, i->second.size());
		for(int i2=0;i2<i->second.size();i2++)
			printf("bus %03d:%07d:%d:%d\n", rn, i->first, i2, i->second[i2].size());
	}
}

void load_bus_locs(int rn)
{
	stop_map[rn].clear();

	char insert_str[5000];
	sqlite3_snprintf(5000, insert_str, "select lat,lng,label,VehicleID,BlockID,Direction,destination,Offset,weather_code,weather_desc,timestamp from data where route=%d order by timestamp;", rn);
	make_database_call(insert_str, load_bus_locs_callback, (void*)&rn);

	debug_bus_locs(rn);
}

size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up)
{ //callback must have this declaration
    //buf is a pointer to the data that curl has for us
    //size*nmemb is the size of the buffer

    for (int c = 0; c<size*nmemb; c++)
    {
        data.push_back(buf[c]);
    }
    return size*nmemb; //tell curl how many bytes we handled
}

int normal_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for(i=0; i<argc; i++)
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	printf("\n");
	return 0;
}

void parse_weather()
{
	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( data, root );

	weather_desc = "";
	weather_code = -1;

	if ( !parsingSuccessful )
	{
		// report to the user the failure and their locations in the document.
		printf("error parsing json\n");
		return;
	}

	if(!root.isObject()) return;

	Json::Value root_data = root["data"];
	Json::Value root_data_weather = root_data["weather"];

	//data.weather[0].weatherDesc[0]

	if(root_data_weather.size())
	{
		Json::Value root_data_weather_zero = root_data_weather[(unsigned int)0];
		Json::Value root_data_weather_desc = root_data_weather_zero["weatherDesc"];

		if(root_data_weather_desc.size() && root_data_weather_desc[(unsigned int)0]["value"].isString())
			weather_desc = root_data_weather_desc[(unsigned int)0]["value"].asString();

		if(root_data_weather_zero["weatherCode"].isString())
		weather_code = atoi(root_data_weather_zero["weatherCode"].asCString());
	}

	printf("weather parsed:%d:%s\n", weather_code, weather_desc.c_str());
}

void parse_stops(int route_number)
{
	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( data, root );
	int the_time = time(0);

	if ( !parsingSuccessful )
	{
		// report to the user the failure and their locations in the document.
		printf("error parsing json\n");
		return;
	}

	//delete previos entries
	char insert_str[5000];
	sqlite3_snprintf(5000, insert_str, "DELETE FROM stops WHERE route=%d", route_number);
	make_database_call(insert_str);

	//insert in
	for ( int index = 0; index < root.size(); ++index )  // Iterates over the sequence elements.
	{
		Json::Value &item = root[index];

		if(!item.isObject()) continue;

		
		double lat = 0;
		double lng = 0;
		int stopid = -1;
		string stopname;

		if(item["lat"].isDouble()) lat = item["lat"].asDouble();
		if(item["lng"].isDouble()) lng = item["lng"].asDouble();
		if(item["stopid"].isInt()) stopid = item["stopid"].asInt();
		if(item["stopname"].isString()) stopname = item["stopname"].asString();

		sqlite3_snprintf(5000, insert_str, "INSERT INTO stops (lat, lng, stopid, stopname, stopindex, route) VALUES ('%lf', '%lf', '%d', '%q', '%d', '%d');",
			lat,
			lng,
			stopid,
			stopname.c_str(),
			index,
			route_number);

		make_database_call(insert_str);
	}

	printf("stops for %d parsed\n", route_number);
}

void parse_json(int route_number)
{
	Json::Value root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( data, root );
	int the_time = time(0);

	if ( !parsingSuccessful )
	{
		// report to the user the failure and their locations in the document.
		printf("error parsing json\n");
		return;
	}

	for ( int index = 0; index < root.size(); ++index )  // Iterates over the sequence elements.
	{
		Json::Value &item = root[index];

		//printf("item type:%d\n", item.type());

		if(!item.isObject()) continue;

		/*
		printf("lat:%lf\n", item["lat"].asDouble());
		printf("lat:%lf\n", item["lng"].asDouble());
		printf("label:%d\n", item["label"].asInt());
		printf("VehicleID:%d\n", item["VehicleID"].asInt());
		printf("BlockID:%d\n", item["BlockID"].asInt());
		printf("Direction:%s\n", item["Direction"].asCString());
		printf("destination:%s\n", item["destination"].asCString());
		printf("Offset:%d\n", item["Offset"].asInt());
		*/

		char insert_str[5000];
		double lat = 0;
		double lng = 0;
		int label = -1;
		int VehicleID = -1;
		int BlockID = -1;
		string Direction;
		string destination;
		int Offset = 0;

		if(item["lat"].isDouble()) lat = item["lat"].asDouble();
		if(item["lng"].isDouble()) lng = item["lng"].asDouble();
		if(item["label"].isInt()) label = item["label"].asInt();
		if(item["VehicleID"].isInt()) VehicleID = item["VehicleID"].asInt();
		if(item["BlockID"].isInt()) BlockID = item["BlockID"].asInt();
		if(item["Direction"].isString()) Direction = item["Direction"].asString();
		if(item["destination"].isString()) destination = item["destination"].asString();
		if(item["Offset"].isInt()) Offset = item["Offset"].asInt();

		sqlite3_snprintf(5000, insert_str, "INSERT INTO data (lat, lng, label, VehicleID, BlockID, Direction, destination, Offset, weather_code, weather_desc, route, timestamp) VALUES ('%lf', '%lf', '%d', '%d', '%d', '%q', '%q', '%d', '%d', '%q', '%d', '%d');",
			lat,
			lng,
			label,
			VehicleID,
			BlockID,
			Direction.c_str(),
			destination.c_str(),
			Offset,
			weather_code,
			weather_desc.c_str(),
			route_number,
			the_time);

		make_database_call(insert_str);

		//printf("value:%s\n", root[index].asString().c_str());
	}
}

bool load_database(string filename)
{
	bool ret_val;
	
	if(db) sqlite3_close(db);
	db = 0;

	//open
	ret_val = !sqlite3_open(filename.c_str(), &db);

	//init table
	if(ret_val)
	{
		//make_database_call("DROP TABLE data;");
		make_database_call("CREATE TABLE data (ID INTEGER PRIMARY KEY,\
						   lat double,\
						   lng double,\
						   label int,\
						   VehicleID int,\
						   BlockID int,\
						   Direction varchar(128),\
						   destination varchar(128),\
						   Offset int,\
						   weather_code int,\
						   weather_desc varchar(64),\
						   route int,\
						   timestamp int);");

		make_database_call("CREATE TABLE stops (ID INTEGER PRIMARY KEY,\
						   lat double,\
						   lng double,\
						   stopid int,\
						   stopname varchar(128),\
						   stopindex int,\
						   route int);");
	}

	return ret_val;
}

bool make_database_call(string input, int (*callback)(void *, int , char **, char **), void* callback_input)
{
	int rc;
	char *zErrMsg;

	rc = sqlite3_exec(db, input.c_str(), callback, callback_input, &zErrMsg);
	if( rc!=SQLITE_OK )
	{
		printf("SQL error: %s\n", zErrMsg);
		printf("SQL input: %s\n", input.c_str());
		sqlite3_free(zErrMsg);
		return false;
	}

	return true;
}

bool make_curl_call(string url)
{
	data.clear();

	CURL* curl; //our curl object

    curl_global_init(CURL_GLOBAL_ALL); //pretty obvious
    curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); //tell curl to output its progress

    curl_easy_perform(curl);

    cout << endl << data << endl;
    //cin.get();

    curl_easy_cleanup(curl);
    curl_global_cleanup();

	return true;
}

void get_data_for_route(int route_number)
{
	char url[2000];
	sprintf(url, "http://www3.septa.org/systemstatus/get_bus_data.php?route=%d", route_number);
	make_curl_call(url);

	parse_json(route_number);
}

void get_weather()
{
	make_curl_call("http://api.worldweatheronline.com/free/v1/weather.ashx?q=Philadelphia&format=JSON&extra=undefined&num_of_days=1&date=&fx=yes&cc=yes&includelocation=yes&show_comments=yes&key=xkq544hkar4m69qujdgujn7w");

	parse_weather();
}

void collect_data()
{
	for(int i=0;1;i++)
	{
		if(i%15==0) get_weather();

		get_data_for_route(9);
		get_data_for_route(21);
		get_data_for_route(42);

		//make_database_call("select * from data order by timestamp;");
		printf("call made at:%d\n", time(0));

#ifdef _WIN32
		Sleep(60 * 1000);
#else
		sleep(60);
#endif
	}
}

int convert_callback(void *vfp, int argc, char **argv, char **azColName)
{
	FILE *fp = (FILE*)vfp;

	for(int i=0; i<argc; i++)
	{
		if(i) fprintf(fp, ",");
		fprintf(fp, argv[i] ? argv[i] : "NULL");
	}
	fprintf(fp, "\n");

	return 0;
}

void convert_data()
{
	FILE *fp;

	fp = fopen("output.txt", "w");

	if(!fp)
	{
		printf("could not open output.txt\n");
		return;
	}

	make_database_call("select lat,lng from data where route=21 order by timestamp;", convert_callback, (void*)fp);

	fclose(fp);

	printf("output.txt written\n");
}

void get_stops(int route_number)
{
	char url[2000];
	sprintf(url, "http://www3.septa.org/hackathon/Stops/%d", route_number);
	make_curl_call(url);

	parse_stops(route_number);
}

int timestamp_to_seg(int timestamp)
{
	return (timestamp/3600)%24;
}

void build_ml_data(int rn)
{
	FILE *fp;

	fp = fopen("data.txt", "w");

	if(!fp)
	{
		printf("could not open data.txt!\n");
		return;
	}

	for(auto it=bus_map[rn].begin();it!=bus_map[rn].end();++it)
		for(int trip=0;trip<it->second.size();trip++)
	{
		vector<bus_loc_info> &the_list = it->second[trip];
		int len = the_list.size();
		
		//find a point
		bus_loc_info *found_item = 0;
		for(int i=len-1;i>=0;i--)
		{
			bus_loc_info &item = the_list[i];
			double dist;

			//chestnut & 15th
			dist = distance(39.951012, -75.165694, item.lat, item.lng);

			//at chestnut & 15th
			if(dist < 0.003865 && item.Direction == "EastBound")
				found_item = &item;

			if(found_item && found_item->timestamp - item.timestamp < 30*60)
			{
				int is_eastbound = item.Direction == "EastBound" ? 1 : 0;
				time_t timestamp_t = item.timestamp;
				tm *ti = gmtime(&timestamp_t);
				int day = (ti->tm_wday - (ti->tm_hour-4<0 ? 1:0))%7;
				int hour = (ti->tm_hour-4)%24;
				int min = ti->tm_min;

				//ti = localtime(&timestamp_t);
				printf("current time :%d %d:%d\n", day, hour, min);

				//FANN style
				/*
				fprintf(fp, "%lf %lf %lf %lf %d %d %d ", item.lat, item.lng, item.lat * item.lng, item.lat / item.lng, item.Offset, is_eastbound, item.timestamp%(60*60*24));
				fprintf(fp,"\n");
				fprintf(fp, "%d", found_item->timestamp - item.timestamp);
				*/
				fprintf(fp, "%d", found_item->timestamp - item.timestamp);
				fprintf(fp, ",%lf", item.lat);
				fprintf(fp, ",%lf", item.lng);
				fprintf(fp, ",%lf", item.lat * item.lng);
				fprintf(fp, ",%lf", item.lat / item.lng);
				fprintf(fp, ",%d", item.Offset);
				fprintf(fp, ",%d", is_eastbound);
				fprintf(fp, ",%d", day);
				fprintf(fp, ",%d", hour);
				//fprintf(fp, ",%d", min);
				fprintf(fp, ",%d", hour*60 + min);
				//fprintf(fp, ",%d", item.timestamp%(60*60));
				//fprintf(fp, ",%d", item.timestamp%(60*60*24));
				//fprintf(fp, ",%d", timestamp_to_seg(item.timestamp));
				//fprintf(fp, ",%d", found_item->Offset - item.Offset);
				fprintf(fp, ",%lf", dist);
				fprintf(fp, ",%lf", 39.951012 - item.lat);
				fprintf(fp, ",%lf", -75.165694 - item.lng);

				fprintf(fp, "%d,%lf,%lf,%lf,%lf,%d,%d,%d,%d,%d,%d", found_item->timestamp - item.timestamp, item.lat, item.lng, item.lat * item.lng, item.lat / item.lng, item.Offset, is_eastbound, item.timestamp%(60*60), item.timestamp%(60*60*24), timestamp_to_seg(item.timestamp), found_item->Offset - item.Offset);
				//fprintf(fp, "%d,%lf,%lf,%d,%d", found_item->timestamp - item.timestamp, item.lat, item.lng, item.Offset, is_eastbound);

				//hour within
				/*
				int hour_seg[24];
				memset(hour_seg, 0, sizeof(int) * 24);
				hour_seg[timestamp_to_seg(item.timestamp)] = 1;
				for(int t=0;t<24;t++)
					fprintf(fp, ",%d", hour_seg[t]);
				*/
				
				//fprintf(fp, ",%d", timestamp_to_seg(item.timestamp));

				//end line
				fprintf(fp,"\n");
			}
		}
	}

	fclose(fp);

	printf("build_ml_data:: data.txt written\n");
}

int main()
{
	if(!load_database("data.db"))
	{
		printf("could not load sqlite database\n");
		return 0;
	}

	//make_database_call("select * from data order by timestamp;");

	//collect_data();
	//convert_data();
	//get_stops(9);
	//get_stops(21);
	//get_stops(42);

	load_stops(21);
	//find_closest_rn_stops(21);
	//find_closest_stop(21, 39.957626, -75.217651);

	//printf("the distance:%lf\n", distance(39.951012, -75.165694, 39.95054, -75.161858));

	load_bus_locs(21);
	build_ml_data(21);

	time_t timestamp_t = time(0);
	tm *ti = gmtime(&timestamp_t);
	int day = (ti->tm_wday - (ti->tm_hour-4<0 ? 1:0))%7;
	int hour = (ti->tm_hour-4)%24;
	int min = ti->tm_min;

	//ti = localtime(&timestamp_t);
	printf("current time :%d %d:%d\n", day, hour, min);

	/*
	int timestamp;
	time_t timestamp_t;
	tm *ti;

	timestamp = time(0);
	timestamp_t = timestamp;
	time(&timestamp_t);
	ti = gmtime(&timestamp_t);
	//ti = localtime(&timestamp_t);
	printf("current time :%d %d:%d\n", ti->tm_wday, ti->tm_hour-4, ti->tm_min);

	timestamp = time(0);
	timestamp_t = timestamp;
	//tm *ti = gmtime(&timestamp_t);
	ti = localtime(&timestamp_t);
	printf("current time :%d %d:%d\n", ti->tm_wday, ti->tm_hour, ti->tm_min);

	printf("current time seg:%d\n", timestamp_to_seg(time(0)));
	*/

	sqlite3_close(db);
	cin.get();

    return 0;
}
