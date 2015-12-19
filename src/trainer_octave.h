#ifndef TRAINER_OCTAVE_H
#define TRAINER_OCTAVE_H

#include "trainer_base.h"

class TrainerOctave : public TrainerBase
{
public:
	TrainerOctave() {}
	~TrainerOctave() {}
	
	double trainData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results);
	
	vector<double> readResults(string filename);
};

#endif
