#include "trainer_octave.h"
#include "mysql_db.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

double TrainerOctave::trainData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results)
{
	string train_file;
	string theta_file;
	string mu_file;
	string sigma_file;
	string octave_command;
	
	mkdir("octave", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
	chdir("octave");
	
	train_file = "route_" + route_id + "_stop_" + to_string(stop_id) + ".txt";
	theta_file = "route_" + route_id + "_stop_" + to_string(stop_id) + "_theta.txt";
	mu_file = "route_" + route_id + "_stop_" + to_string(stop_id) + "_mu.txt";
	sigma_file = "route_" + route_id + "_stop_" + to_string(stop_id) + "_sigma.txt";
	
	//write out train data
	writeTrainData(train_data, train_file);
	
	//setup and call octave
	octave_command = "octave oc.m " + train_file + " " + theta_file + " " + mu_file + " " + sigma_file;
	printf("running octave: '%s'\n", octave_command.c_str());
	system(octave_command.c_str());
	
	//read the results
	theta_results = readResults(theta_file);
	mu_results = readResults(mu_file);
	sigma_results = readResults(sigma_file);
	
	printf("theta_results %lu ... %lf\n", theta_results.size(), theta_results.size() ? theta_results[0] : 0);
	printf("mu_results %lu ... %lf\n", mu_results.size(), mu_results.size() ? mu_results[0] : 0);
	printf("sigma_results %lu ... %lf\n", sigma_results.size(), sigma_results.size() ? sigma_results[0] : 0);
	
	chdir("..");
	
	return 0;
}

vector<double> TrainerOctave::readResults(string filename)
{
	vector<double> ret_vec;
	ifstream infile(filename.c_str());
	
	string line_str;
	while( getline( infile, line_str ) ) 
	{
		//get rid of char 13 \r if present
		line_str.erase(remove(line_str.begin(), line_str.end(), 13), line_str.end());

		if(!line_str.size()) continue;
		if(line_str[0] == '#') continue;
		
		ret_vec.push_back(atof(line_str.c_str()));
	}

	infile.close();
	
	return ret_vec;
}
