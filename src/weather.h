#ifndef WEATHER_H
#define WEATHER_H

class WeatherObject;

class Weather
{
    public:
        Weather();

        bool getRecentWeather();
        bool getRecentWeather(WeatherObject &w_obj, bool check_mysql_front_end = true);
        bool newRecentFrontEndWeatherAvailable();

        static Weather* getInstance();
        static Weather* instance;

        int last_timestamp;
};

#endif
