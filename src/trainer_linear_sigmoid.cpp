#include "trainer_linear_sigmoid.h"

#include <math.h>

inline double sigmoid(double z)
{
    return 1.0 / (1.0 + exp(-z));
}

void TrainerLinearSigmoid::costFunc(double* theta, double* cost, double* grad)
{
    int m = entry_count;//y_data.size();
    int d = feature_count;//x_data[0].size();

    double cost_sum = 0;
    double h_i[m];
    for(int i=0;i<m;i++)
    {
        double tx = theta[0];

        for(int j=0;j<d;j++)
            tx += theta[j+1] * x_data[i][j];

        double h = sigmoid(tx);
        h_i[i] = h;

        double diff = h - y_data[i];
        cost_sum += (-y_data[i] * log(h)) - ((1 - y_data[i]) * log(1 - h));
    }

    *cost = cost_sum / m;

    for(int j=0;j<d;j++)
    {
        double grad_sum = 0;
        for(int i=0;i<m;i++)
            grad_sum += (h_i[i] - y_data[i]) * x_data[i][j];

        grad[j+1] = grad_sum / m;
    }

    double grad_sum = 0;
    for(int i=0;i<m;i++)
        grad_sum += (h_i[i] - y_data[i]);
    grad[0] = grad_sum / m;
}

