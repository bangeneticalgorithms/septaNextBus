#include "weather.h"
#include "functions.h"
#include "objects.h"
#include "mysql_db.h"
#include "settings.h"

#include <jsoncpp/json/json.h>

Weather* Weather::instance = NULL;

Weather::Weather()
{
	last_timestamp = 0;
}

Weather* Weather::getInstance()
{
	if(!instance) instance = new Weather();

	return instance;
}

bool Weather::getRecentWeather()
{
	WeatherObject w_obj;
	
	if(getRecentWeather(w_obj))
	{
		MysqlDB *myobj = MysqlDB::getInstance();
		return myobj->insertWeather(w_obj);
	}
	
	return false;
}

bool Weather::newRecentFrontEndWeatherAvailable()
{
	MysqlDB *myobj = MysqlDB::getInstance();
	
	WeatherObject w_obj = myobj->getWeatherFrontEnd();
	
	//if timestamp is the same or older
	//or the timestamp is too old
	if(w_obj.timestamp <= last_timestamp) return false;
	if(abs(time(0) - w_obj.timestamp) > 60) return false;
	
	printf("Weather::recentFrontEndWeatherAvailable: %ld seconds old\n", abs(time(0) - w_obj.timestamp));
	
	return true;
}

//v2 parser
bool Weather::getRecentWeather(WeatherObject &w_obj, bool check_mysql_front_end)
{
	printf("Weather::getRecentWeather()\n");
	
	if(check_mysql_front_end)
	{
		MysqlDB *myobj = MysqlDB::getInstance();
		
		w_obj = myobj->getWeatherFrontEnd();
		
		//it "good enough" to skip collecting it again?
		if(abs(time(0) - w_obj.timestamp) < 120) 
		{
			printf("Weather::getRecentWeather: using front_end data: %ld < %d\n", abs(time(0) - w_obj.timestamp), 120);
			
			last_timestamp = w_obj.timestamp;
			
			return true;
		}
	}
	
	Json::Value root;
	Json::Value value;
	Json::Reader reader;
	string call_data = make_curl_call(Settings::makeWeatherLink());
	
	bool ret = reader.parse( call_data, root );
	
	if(!ret) return false;
	
	//root should be an object
	if(!root.isObject()) return false;
	
	//should have a member "data"
	Json::Value real_root = root["data"];
	if(!real_root.isObject() || real_root.isNull())
	{
		//did we exceed our daily call limit?
		
		printf("Weather::getRecentWeather: no data object in call data '%s'\n", call_data.c_str());
		
		Json::Value results = root["results"];
		if(!results.isObject()) return false;
		if(results.isNull()) return false;
		
		Json::Value error = results["error"];
		if(!error.isObject()) return false;
		if(error.isNull()) return false;
		
		Json::Value type = error["type"];
		if(!type.isString()) return false;
		if(type.isNull()) return false;
		
		//cycle the keys if we exceeded the daily limit
		if(type.asString() == "QpdExceededError")
		{
			string old_key = Settings::getWeatherKey();
			
			Settings::cycleWeatherKeys();
			
			printf("Weather::getRecentWeather: daily limit exceeded for key '%s'!! now using '%s'\n", old_key.c_str(), Settings::getWeatherKey().c_str());
			
			//try to make the call etc again...
			{
				call_data = make_curl_call(Settings::makeWeatherLink());

				ret = reader.parse( call_data, root );
				if(!ret) return false;
				
				//root should be an object
				if(!root.isObject()) return false;
				
				//should have a member "data"
				real_root = root["data"];
				if(!real_root.isObject()) return false;
				if(real_root.isNull()) return false;
			}
		}
		else
		{
			printf("Weather::getRecentWeather: an error but not 'QpdExceededError' ... '%s'\n", call_data.c_str());
			return false;
		}
	}
	
	Json::Value current_condition_array = real_root["current_condition"];
	if(!current_condition_array.isArray()) return false;
	if(current_condition_array.isNull()) return false;
	if(!current_condition_array.size()) return false;
	
	Json::Value current_condition_object = current_condition_array[(Json::Value::UInt)0];
	if(!current_condition_object.isObject()) return false;
	if(current_condition_object.isNull()) return false;
	
	value = current_condition_object["cloudcover"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.cloudcover = atoi(value.asString().c_str());
	
	value = current_condition_object["humidity"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.humidity = atoi(value.asString().c_str());
	
	value = current_condition_object["precipMM"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.precipMM = atof(value.asString().c_str());
	
	value = current_condition_object["pressure"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.pressure = atoi(value.asString().c_str());
	
	value = current_condition_object["temp_C"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.temp_C = atoi(value.asString().c_str());
	
	value = current_condition_object["temp_F"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.temp_F = atoi(value.asString().c_str());
	
	value = current_condition_object["visibility"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.visibility = atoi(value.asString().c_str());
	
	value = current_condition_object["weatherCode"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.weatherCode = atoi(value.asString().c_str());
	
	value = current_condition_object["weatherDesc"];
	if(value.isNull()) return false;
	if(!value.isArray()) return false;
	if(!value.size()) return false;
	value = value[(Json::Value::UInt)0];
	if(value.isNull()) return false;
	if(!value.isObject()) return false;
	value = value["value"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.weatherDesc = value.asString();
	
	value = current_condition_object["winddir16Point"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.winddir16Point = value.asString();
	
	value = current_condition_object["winddirDegree"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.winddirDegree = atoi(value.asString().c_str());
	
	value = current_condition_object["windspeedKmph"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.windspeedKmph = atoi(value.asString().c_str());
	
	value = current_condition_object["windspeedMiles"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.windspeedMiles = atoi(value.asString().c_str());
	
	//--------------------------------------------------
	//--------------------------------------------------
	
	Json::Value weather_array = real_root["weather"];
	if(!weather_array.isArray()) return false;
	if(weather_array.isNull()) return false;
	if(!weather_array.size()) return false;
	
	Json::Value weather_object = weather_array[(Json::Value::UInt)0];
	if(!weather_object.isObject()) return false;
	if(weather_object.isNull()) return false;
	
// 	value = weather_object["precipMM"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_precipMM = atof(value.asString().c_str());
	
	value = weather_object["maxtempC"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMaxC = atoi(value.asString().c_str());
	
	value = weather_object["maxtempF"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMaxF = atoi(value.asString().c_str());
	
	value = weather_object["mintempC"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMinC = atoi(value.asString().c_str());
	
	value = weather_object["mintempF"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMinF = atoi(value.asString().c_str());
	
// 	value = weather_object["weatherCode"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_weatherCode = atoi(value.asString().c_str());
// 	
// 	value = weather_object["weatherDesc"];
// 	if(value.isNull()) return false;
// 	if(!value.isArray()) return false;
// 	if(!value.size()) return false;
// 	value = value[(Json::Value::UInt)0];
// 	if(value.isNull()) return false;
// 	if(!value.isObject()) return false;
// 	value = value["value"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_weatherDesc = value.asString();
// 	
// 	value = weather_object["winddir16Point"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_winddir16Point = value.asString();
// 	
// 	value = weather_object["winddirDegree"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_winddirDegree = atoi(value.asString().c_str());
// 	
// 	value = weather_object["winddirection"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_winddirection = value.asString();
// 	
// 	value = weather_object["windspeedKmph"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_windspeedKmph = atoi(value.asString().c_str());
// 	
// 	value = weather_object["windspeedMiles"];
// 	if(value.isNull()) return false;
// 	if(!value.isString()) return false;
// 	w_obj.w_windspeedMiles = atoi(value.asString().c_str());
	
	w_obj.timestamp = time(0);
	
	last_timestamp = w_obj.timestamp;
	
	return true;
}

//v1 parser
/*
bool Weather::getRecentWeather()
{
	printf("Weather::getRecentWeather()\n");
	
	Json::Value root;
	Json::Value value;
	Json::Reader reader;
	WeatherObject w_obj;
	string call_data = make_curl_call(Settings::weather_link);
	
	bool ret = reader.parse( call_data, root );
	
	if(!ret) return false;
	
	//root should be an object
	if(!root.isObject()) return false;
	
	//should have a member "data"
	Json::Value real_root = root["data"];
	if(!real_root.isObject()) return false;
	if(real_root.isNull()) return false;
	
	Json::Value current_condition_array = real_root["current_condition"];
	if(!current_condition_array.isArray()) return false;
	if(current_condition_array.isNull()) return false;
	if(!current_condition_array.size()) return false;
	
	Json::Value current_condition_object = current_condition_array[(Json::Value::UInt)0];
	if(!current_condition_object.isObject()) return false;
	if(current_condition_object.isNull()) return false;
	
	value = current_condition_object["cloudcover"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.cloudcover = atoi(value.asString().c_str());
	
	value = current_condition_object["humidity"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.humidity = atoi(value.asString().c_str());
	
	value = current_condition_object["precipMM"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.precipMM = atof(value.asString().c_str());
	
	value = current_condition_object["pressure"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.pressure = atoi(value.asString().c_str());
	
	value = current_condition_object["temp_C"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.temp_C = atoi(value.asString().c_str());
	
	value = current_condition_object["temp_F"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.temp_F = atoi(value.asString().c_str());
	
	value = current_condition_object["visibility"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.visibility = atoi(value.asString().c_str());
	
	value = current_condition_object["weatherCode"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.weatherCode = atoi(value.asString().c_str());
	
	value = current_condition_object["weatherDesc"];
	if(value.isNull()) return false;
	if(!value.isArray()) return false;
	if(!value.size()) return false;
	value = value[(Json::Value::UInt)0];
	if(value.isNull()) return false;
	if(!value.isObject()) return false;
	value = value["value"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.weatherDesc = value.asString();
	
	value = current_condition_object["winddir16Point"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.winddir16Point = value.asString();
	
	value = current_condition_object["winddirDegree"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.winddirDegree = atoi(value.asString().c_str());
	
	value = current_condition_object["windspeedKmph"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.windspeedKmph = atoi(value.asString().c_str());
	
	value = current_condition_object["windspeedMiles"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.windspeedMiles = atoi(value.asString().c_str());
	
	//--------------------------------------------------
	//--------------------------------------------------
	
	Json::Value weather_array = real_root["weather"];
	if(!weather_array.isArray()) return false;
	if(weather_array.isNull()) return false;
	if(!weather_array.size()) return false;
	
	Json::Value weather_object = weather_array[(Json::Value::UInt)0];
	if(!weather_object.isObject()) return false;
	if(weather_object.isNull()) return false;
	
	value = weather_object["precipMM"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_precipMM = atof(value.asString().c_str());
	
	value = weather_object["tempMaxC"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMaxC = atoi(value.asString().c_str());
	
	value = weather_object["tempMaxF"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMaxF = atoi(value.asString().c_str());
	
	value = weather_object["tempMinC"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMinC = atoi(value.asString().c_str());
	
	value = weather_object["tempMinF"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_tempMinF = atoi(value.asString().c_str());
	
	value = weather_object["weatherCode"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_weatherCode = atoi(value.asString().c_str());
	
	value = weather_object["weatherDesc"];
	if(value.isNull()) return false;
	if(!value.isArray()) return false;
	if(!value.size()) return false;
	value = value[(Json::Value::UInt)0];
	if(value.isNull()) return false;
	if(!value.isObject()) return false;
	value = value["value"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_weatherDesc = value.asString();
	
	value = weather_object["winddir16Point"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_winddir16Point = value.asString();
	
	value = weather_object["winddirDegree"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_winddirDegree = atoi(value.asString().c_str());
	
	value = weather_object["winddirection"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_winddirection = value.asString();
	
	value = weather_object["windspeedKmph"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_windspeedKmph = atoi(value.asString().c_str());
	
	value = weather_object["windspeedMiles"];
	if(value.isNull()) return false;
	if(!value.isString()) return false;
	w_obj.w_windspeedMiles = atoi(value.asString().c_str());
	
	w_obj.timestamp = time(0);
	
	last_timestamp = w_obj.timestamp;
	
	MysqlDB *myobj = MysqlDB::getInstance();
	return myobj->insertWeather(w_obj);
}
*/
