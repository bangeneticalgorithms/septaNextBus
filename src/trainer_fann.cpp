#include "trainer_fann.h"

#include <math.h>

#ifdef USE_FANN
#include <fann.h>
#endif

double TrainerFANN::trainData(vector< vector<double> > &train_data, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results)
{
    double ret_cost = -1;

    if(!train_data.size()) 
    {
        printf("TrainerFANN::trainData train_data empty \n");
        return ret_cost;
    }
    if(!train_data[0].size()) 
    {
        printf("TrainerFANN::trainData no features \n");
        return ret_cost;
    }

#ifdef USE_FANN

    const unsigned int num_input = train_data[0].size() - 1;
    const unsigned int num_output = 1;
    const unsigned int num_layers = 3;
    const unsigned int num_neurons_hidden = 300;
    const float desired_error = (const float) 0.001;
    const unsigned int max_epochs = 500000;
    const unsigned int epochs_between_reports = 50;

    struct fann *ann = fann_create_standard(num_layers, num_input, num_neurons_hidden, num_output);
    struct fann_train_data *data = fann_create_train(train_data.size(), num_input, num_output);

    vector<double> y_data;
    vector<vector<double>> x_data;

    y_data.resize(train_data.size());
    x_data.resize(train_data.size());
    for(int i=0;i<train_data.size();i++)
    {
        y_data[i] = train_data[i][0];

        x_data[i].resize(train_data[i].size()-1);
        for(int j=1;j<train_data[i].size();j++)
            x_data[i][j-1] = (train_data[i][j]);
    }

    //get mu and sigma
    featureNormalize(x_data, mu_results, sigma_results);

    for(int i=0;i<y_data.size();i++)
    {
        data->output[i][0] = y_data[i];

        for(int j=0;j<x_data[i].size();j++)
            data->input[i][j] = x_data[i][j];
    }

    //fann_scale_train_data(data, -1, 1);

    //fann_set_training_algorithm(ann, FANN_TRAIN_INCREMENTAL);
    fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
    fann_set_activation_function_output(ann, FANN_LINEAR);
    fann_set_train_error_function(ann, FANN_ERRORFUNC_LINEAR);

    fann_train_on_data(ann, data, max_epochs, epochs_between_reports, desired_error);

    ret_cost = fann_test_data(ann, data);

    fann_save(ann, "fann.net");

    fann_destroy_train(data);
    fann_destroy(ann);

#endif

    return ret_cost;
}

void TrainerFANN::featureNormalize(vector< vector<double> > &train_data, vector<double> &mu_results, vector<double> &sigma_results)
{
    int f_size = train_data[0].size();

    mu_results.clear();
    sigma_results.clear();
    mu_results.assign(f_size, 0);
    sigma_results.assign(f_size, 0);

    printf("featureNormalize %lu x %lu \n", train_data.size(), train_data[0].size());

    //get mu
    for(int i=0;i<train_data.size();i++)
        for(int j=0;j<train_data[i].size() && j<mu_results.size();j++)
            mu_results[j] += train_data[i][j];

    for(int i=0;i<mu_results.size();i++)
        mu_results[i] /= train_data.size();

    //X_norm = bsxfun(@minus, X, mu);
    for(int i=0;i<train_data.size();i++)
        for(int j=0;j<train_data[i].size() && j<mu_results.size();j++)
            train_data[i][j] -= mu_results[j];

    //std
    for(int j=0;j<sigma_results.size();j++)
    {
        for(int i=0;i<train_data.size();i++)
        {
            double diff = train_data[i][j];
            sigma_results[j] += diff * diff;
        }

        sigma_results[j] = sqrt(sigma_results[j] / (train_data.size() - 1));
    }

    //X_norm = bsxfun(@rdivide, X_norm, sigma);
    for(int i=0;i<train_data.size();i++)
        for(int j=0;j<train_data[i].size() && j<mu_results.size();j++)
            train_data[i][j] /= sigma_results[j];

    //any sigmas = 0?
    for(int j=0;j<sigma_results.size();j++)
    {
        if(fabs(sigma_results[j]) > 0.000001) continue;

        sigma_results[j] = 1;
        mu_results[j] = 0;
        for(int i=0;i<train_data.size();i++)
            train_data[i][j] = 0;
    }
}

void TrainerFANN::writeFANNInputData(vector< vector<double> > &train_data, string filename)
{
    FILE *fp = fopen(filename.c_str(), "w");

    if(!train_data.size() || !train_data[0].size())
    {
        printf("TrainerFANN::writeFANNInputData: bad train_data sizes\n");
        return;
    }

    if(!fp)
    {
        printf("TrainerFANN::writeFANNInputData: could not open '%s'\n", filename.c_str());
        return;
    }

    fprintf(fp, "%lu %lu %d\n", train_data.size(), train_data[0].size()-1, 1);

    for(int i=0;i<train_data.size();i++)
    {
        vector<double> &train_entry = train_data[i];

        //inputs
        for(int k=1;k<train_entry.size();k++)
            fprintf(fp, "%lf " , train_entry[k]);
        fprintf(fp, "\n");

        //outputs
        fprintf(fp, "%lf\n", train_entry[0]);
    }

    fclose(fp);
}

