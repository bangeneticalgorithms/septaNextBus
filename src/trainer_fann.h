#ifndef TRAINER_FANN_H
#define TRAINER_FANN_H

#include "trainer_base.h"

class TrainerFANN : public TrainerBase
{
    public:
        TrainerFANN() {}
        ~TrainerFANN() {}

        double trainData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results);

        void featureNormalize(vector< vector<double> > &train_data, vector<double> &mu_results, vector<double> &sigma_results);

        static void writeFANNInputData(vector< vector<double> > &train_data, string filename);
};

#endif
