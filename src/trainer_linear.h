#ifndef TRAINER_LINEAR_H
#define TRAINER_LINEAR_H

#include "trainer_base.h"

class TrainerLinear : public TrainerBase
{
public:
	TrainerLinear() {}
	~TrainerLinear() {}
	
	double trainData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results);
	
	void featureNormalize(vector< vector<double> > &train_data, vector<double> &mu_results, vector<double> &sigma_results);
	void costFunc(double* theta, double* cost, double* gradVec);
	int fmincg(double* theta, int nDim, int maxCostCalls);

	vector<double> y_data;
	vector<vector<double>> x_data;
	
	int entry_count;
	int feature_count;
};

#endif
