#include "trainer_base.h"
#include "functions.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string>

using namespace std;

double TrainerBase::trainData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results)
{
	mkdir("data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
	writeTrainData(train_data, "data/route_" + route_id + "_stop_" + to_string(stop_id) + ".txt");
	
	return 0;
}

void TrainerBase::writeTrainData(vector< vector<double> > &train_data, string filename)
{
	FILE *fp = fopen(filename.c_str(), "w");
	
	if(!fp)
	{
		printf("TrainerBase::writeTrainData: could not open '%s'\n", filename.c_str());
		return;
	}
	
	printf("TrainerBase::writeTrainData: %lu ... %lu\n", train_data.size(), train_data[0].size());
	
	for(int i=0;i<train_data.size();i++)
	{
		vector<double> &vd = train_data[i];
		
		for(int j=0;j<vd.size();j++)
		{
			double &d = vd[j];
			
			if(j) fprintf(fp, ",%lf", d);
			else fprintf(fp, "%lf", d);
		}
		
		fprintf(fp, "\n");
	}
	
	fclose(fp);
}

void TrainerBase::writeResultData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results, string filename)
{
	FILE *fp = fopen(filename.c_str(), "w");
	
	if(!fp)
	{
		printf("TrainerBase::writeResultData::could not open '%s'\n", filename.c_str());
		return;
	}
	
	for(int i=0;i<train_data.size();i++)
	{
		double result;
		
		vector<double> item;
		item.resize(train_data[i].size()-1);
		for(int j=1;j<train_data[i].size();j++)
			item[j-1] = (train_data[i][j]);
		
		result = testCoefficients(item, theta_results, mu_results, sigma_results);
		
		fprintf(fp, "%lf,%lf\n", result, train_data[i][0]);
	}
	
	fclose(fp);
}

double TrainerBase::testCoefficients(vector<double> &item, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results)
{
	if(item.size()+1 != theta_results.size() || !theta_results.size())
	{
		printf("TrainerBase::testCoefficients: bad sizes - item:%lu theta:%lu \n", item.size(), theta_results.size());
		return -1;
	}
	
	if(mu_results.size() != sigma_results.size() || mu_results.size()+1 != theta_results.size())
	{
		printf("TrainerBase::testCoefficients: wrong mu / sigma / theta sizes \n");
		return -1;
	}
	
	double total = theta_results[0];
	for(int i=0;i<item.size();i++)
	{
		double value = item[i];
		
		//printf("TrainerBase::testCoefficients: value %02d:%lf \n", i, value);
		
		value -= mu_results[i];
		value /= sigma_results[i];
		total += value * theta_results[i+1];
	}
	
	return total;
}

void TrainerBase::removeDuplicateColumns(vector< vector<double> > &train_data, vector< vector<double> > &train_data_reduced, vector<bool> &train_data_remaining)
{
	train_data_remaining.clear();
	train_data_reduced.clear();
	
	if(!train_data.size() || !train_data[0].size())
	{
		printf("TrainerBase::removeDuplicateColumns: bad train_data sizes\n");
		return;
	}
	
	//check which cols are the same
	
	//init arrays
	for(int i=0;i<train_data[0].size();i++)
		train_data_remaining.push_back(false);
	train_data_remaining[0] = true;
	
	//check
	vector<double> &first_entry = train_data[0];
	for(int i=1;i<train_data.size();i++)
	{
		vector<double> &train_entry = train_data[i];
		
		for(int k=1;k<train_entry.size();k++)
			if(!double_compare(train_entry[k], first_entry[k]))
				train_data_remaining[k] = true;
	}
	
	//init
	train_data_reduced.clear();
	
	//create train_data_reduced
	for(int i=0;i<train_data.size();i++)
	{
		train_data_reduced.push_back(vector<double>());
		
		vector<double> &train_entry = train_data[i];
		vector<double> &train_entry_reduced = train_data_reduced[i];
		
		for(int k=0;k<train_data_remaining.size();k++)
		{
			if(!train_data_remaining[k]) continue;
			
			train_entry_reduced.push_back(train_entry[k]);
		}
	}
	
	//print out for debugging
	//debugTrainData(train_data, "train_data");
	//debugTrainData(train_data_reduced, "train_data_reduced");
	
	//for(int i=0;i<train_data_remaining.size();i++)
	//	printf("train_data_remaining[%03d]:%d\n", i, train_data_remaining[i] ? 1 : 0);
}

void TrainerBase::debugTrainData(vector< vector<double> > &train_data, string data_name)
{
	for(int i=0;i<train_data[0].size();i++)
		printf("%s[%03d]:%lf\n", data_name.c_str(), i, train_data[0][i]);
}

