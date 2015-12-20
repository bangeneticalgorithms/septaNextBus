#include <string>
#include <vector>

using namespace std;

class Settings
{
public:
	
	static void readSettings();
	static void writeSettings();
	static void debug();
	
	static string makeWeatherLink();
	static void setupWeatherKeys();
	static void cycleWeatherKeys();
	static string getWeatherKey();
	
	static void setupGoogDirKeys();
	static void cycleGoogDirKeys();
	static string getGoogDirKey();
	
	static string mysql_user;
	static string mysql_pass;
	static string mysql_host;
	static string mysql_db;
	static string weather_key;
	static string google_directions_key;
	
	static vector<string> weather_keys;
	static int cur_weather_key;
	static vector<string> google_directions_keys;
	static int cur_google_directions_key;
};
