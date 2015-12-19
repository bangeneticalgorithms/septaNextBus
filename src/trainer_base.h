#ifndef TRAINER_BASE_H
#define TRAINER_BASE_H

#include <vector>
#include <string>

using namespace std;

class TrainerBase
{
public:
	TrainerBase() { iterations=1000; }
	~TrainerBase() {}
	
	virtual double trainData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results);
	
	static void writeTrainData(vector< vector<double> > &train_data, string filename);
	static void writeResultData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results, string filename = "data/results.txt");
	static double testCoefficients(vector<double> &item, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results);
	static void removeDuplicateColumns(vector< vector<double> > &train_data, vector< vector<double> > &train_data_reduced, vector<bool> &train_data_remaining);
	static void debugTrainData(vector< vector<double> > &train_data, string data_name);
	
	string route_id;
	int stop_id;
	int iterations;
	vector<double> cost_graph;
};

#endif
