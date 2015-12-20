#include "settings.h"

#include <sstream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <string.h>

using namespace std;

string Settings::mysql_user = "snb_user";
string Settings::mysql_pass = "snb_password";
string Settings::mysql_host = "localhost";
string Settings::mysql_db = "septa_next_bus_db";
string Settings::weather_key = "6583699e77c876fb03f7f6ab325d4;ba440518ea3837ee0e8f15361ce38;04d3a39f1cfad591b9289e54bce08";
string Settings::google_directions_key = "AIzaSyDrHrg6DFMfsFv4Ox4xYJwipmIJBn6ih6U;AIzaSyCCvcJeiCWaLRfJyI2mnZIvy2bbQagRREs;AIzaSyAgOJFWxWAS77frm11Z_zXALVpusBjgiZI;AIzaSyASImJrsMjrirG387oJ_gd0GfdYOelhnUQ;AIzaSyBO_g-izEM41PqRabYXoePUef5R4pN-ySs;AIzaSyC3Tph90VxChnSl6WsKBuBS73LFtGikd10;AIzaSyAzstY8gpuq7bFRE6-TzIHc9M2MczuXrBY;AIzaSyCseAokrfA3boTZ5Z7O9dcS3j9LpYM7sPA";

vector<string> Settings::weather_keys;
int Settings::cur_weather_key = 0;
vector<string> Settings::google_directions_keys;
int Settings::cur_google_directions_key = 0;

void Settings::readSettings()
{
	ifstream infile("settings.txt");
	
	//if not found create it
	if(!infile.is_open())
	{
		printf("could not load settings.txt\n");
		
		writeSettings();
		
		setupWeatherKeys();
		setupGoogDirKeys();
		
		return;
	}
	
	string line;
	while (getline(infile, line))
	{
		if(!line.find("mysql_user="))
			mysql_user = line.substr(strlen("mysql_user="));
		else if(!line.find("mysql_pass="))
			mysql_pass = line.substr(strlen("mysql_pass="));
		else if(!line.find("mysql_host="))
			mysql_host = line.substr(strlen("mysql_host="));
		else if(!line.find("mysql_db="))
			mysql_db = line.substr(strlen("mysql_db="));
		else if(!line.find("weather_key="))
			weather_key = line.substr(strlen("weather_key="));
		else if(!line.find("google_directions_key="))
			google_directions_key = line.substr(strlen("google_directions_key="));
	}
	
	setupWeatherKeys();
	setupGoogDirKeys();
}

void Settings::writeSettings()
{
	FILE *fp;
	
	fp = fopen("settings.txt", "w");
	
	if(!fp)
	{
		printf("could not open settings.txt to write\n");
		return;
	}
	
	fprintf(fp, "#lines starting with # are ignored\n");
	fprintf(fp, "mysql_user=%s\n", mysql_user.c_str());
	fprintf(fp, "mysql_pass=%s\n", mysql_pass.c_str());
	fprintf(fp, "mysql_host=%s\n", mysql_host.c_str());
	fprintf(fp, "mysql_db=%s\n", mysql_db.c_str());
	fprintf(fp, "weather_key=%s\n", weather_key.c_str());
	fprintf(fp, "google_directions_key=%s\n", google_directions_key.c_str());
	
	fclose(fp);
	
	printf("created default settings.txt\n");
}

string Settings::makeWeatherLink()
{
	return "http://api.worldweatheronline.com/free/v2/weather.ashx?q=philadelphia&num_of_days=1&tp=1&format=json&key=" + getWeatherKey();
}

void Settings::setupWeatherKeys()
{
	weather_keys.clear();
	
	stringstream ss(weather_key);
	string s;

	while (getline(ss, s, ';'))
		weather_keys.push_back(s);
	
	cur_weather_key = 0;
}

void Settings::cycleWeatherKeys()
{
	if(!weather_keys.size()) return;
	
	cur_weather_key++;
	cur_weather_key = cur_weather_key % weather_keys.size();
}

string Settings::getWeatherKey()
{
	if(!weather_keys.size()) return "";
	
	if(cur_weather_key < 0) cycleWeatherKeys();
	if(cur_weather_key >= weather_keys.size()) cycleWeatherKeys();
	
	return weather_keys[cur_weather_key];
}

void Settings::setupGoogDirKeys()
{
	google_directions_keys.clear();
	
	stringstream ss(google_directions_key);
	string s;

	while (getline(ss, s, ';'))
		google_directions_keys.push_back(s);
	
	cur_google_directions_key = 0;
}

void Settings::cycleGoogDirKeys()
{
	if(!google_directions_keys.size()) return;
	
	cur_google_directions_key++;
	cur_google_directions_key = cur_google_directions_key % google_directions_keys.size();
}

string Settings::getGoogDirKey()
{
	if(!google_directions_keys.size()) return "";
	
	if(cur_google_directions_key < 0) cycleGoogDirKeys();
	if(cur_google_directions_key >= google_directions_keys.size()) cycleGoogDirKeys();
	
	return google_directions_keys[cur_google_directions_key];
}

void Settings::debug()
{
	printf("Settings::debug()\n");
	printf("mysql_user=%s\n", mysql_user.c_str());
	printf("mysql_pass=%s\n", mysql_pass.c_str());
	printf("mysql_host=%s\n", mysql_host.c_str());
	printf("mysql_db=%s\n", mysql_db.c_str());
	printf("weather_key=%s\n", weather_key.c_str());
	printf("google_directions_key=%s\n", google_directions_key.c_str());
}

