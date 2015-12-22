#include "trainer_linear.h"
#include "functions.h"
#include "trainer_fann.h"

#include <math.h>
#include <float.h>

#define COST_FUNC_DATATYPE double
#define COST_FUNC_DATATYPE_MIN (FLT_MIN*100)

#define RHO 0.01f
#define SIG 0.5f
#define INT 0.1f
#define EXT 3.0f
#define MAX 20
#define RATIO 100.0f

double TrainerLinear::trainData(vector< vector<double> > &train_data_unreduced, vector<double> &theta_results, vector<double> &mu_results, vector<double> &sigma_results)
{
    theta_results.clear();
    mu_results.clear();
    sigma_results.clear();
    double ret_cost = -1;

    if(!train_data_unreduced.size()) 
    {
        printf("TrainerLinear::trainData train_data empty \n");
        return ret_cost;
    }
    if(!train_data_unreduced[0].size()) 
    {
        printf("TrainerLinear::trainData no features \n");
        return ret_cost;
    }

    //remove duplicate columns for faster training
    vector< vector<double> > train_data;
    vector<bool> train_data_remaining;
    removeDuplicateColumns(train_data_unreduced, train_data, train_data_remaining);
    //train_data = train_data_unreduced;

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

    //train theta
    {
        int theta_dim = mu_results.size()+1;
        double *theta = new double[theta_dim];
        for(int i=0;i<theta_dim;i++) 
            theta[i] = 0;

        //optimize these variables
        entry_count = y_data.size();
        feature_count = x_data[0].size();

        //the actual training, fmincg
        fmincg(theta, theta_dim, iterations);

        //how effective was the training?
        double *grad = new double[theta_dim];
        costFunc(theta, &ret_cost, grad);

        //printf("TrainerLinear::trainData: final cost: %lf \n", ret_cost);
        //for(int i=0;i<theta_dim;i++) 
        //	printf("grad: %lf \t %lf \n", theta[i], grad[i]);

        delete grad;

        //writeTrainData(train_data, "data/route_" + route_id + "_stop_" + to_string(stop_id) + ".txt");
        //TrainerFANN::writeFANNInputData(train_data, "data/route_" + route_id + "_stop_" + to_string(stop_id) + "_fann.txt");

        //formalize theta output
        theta_results.resize(theta_dim);
        for(int i=0;i<theta_dim;i++)
            theta_results[i] = theta[i];
        delete theta;
    }

    //printf("theta_results %lu ... %lf\n", theta_results.size(), theta_results.size() ? theta_results[0] : 0);
    //printf("mu_results %lu ... %lf\n", mu_results.size(), mu_results.size() ? mu_results[0] : 0);
    //printf("sigma_results %lu ... %lf\n", sigma_results.size(), sigma_results.size() ? sigma_results[0] : 0);

    vector<double> test_item;
    test_item.resize(train_data[0].size()-1);
    for(int j=1;j<train_data[0].size();j++)
        test_item[j-1] = (train_data[0][j]);
    double result = testCoefficients(test_item, theta_results, mu_results, sigma_results);
    printf("TrainerLinear::trainData: result:%lf actual:%lf \n", result, train_data[0][0]);

    //add back in
    for(int i=1;i<train_data_remaining.size();i++)
    {
        //already in?
        if(train_data_remaining[i]) continue;

        //if not add it
        theta_results.insert(theta_results.begin()+i, 0);
        mu_results.insert(mu_results.begin()+(i-1), 0);
        sigma_results.insert(sigma_results.begin()+(i-1), 1);
    }

    return ret_cost;
}

void TrainerLinear::featureNormalize(vector< vector<double> > &train_data, vector<double> &mu_results, vector<double> &sigma_results)
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

void TrainerLinear::costFunc(double* theta, double* cost, double* grad)
{
    double lambda = 0;
    int m = entry_count;//y_data.size();
    int d = feature_count;//x_data[0].size();

    double cost_sum = 0;
    double grad_diff[m];
    for(int i=0;i<m;i++)
    {
        double h = theta[0];

        for(int j=0;j<d;j++)
            h += theta[j+1] * x_data[i][j];

        double diff = h - y_data[i];
        cost_sum += diff * diff;

        grad_diff[i] = diff;
    }

    *cost = cost_sum / (2 * m);

    for(int j=0;j<d;j++)
    {
        double grad_sum = 0;
        for(int i=0;i<m;i++)
            grad_sum += grad_diff[i] * x_data[i][j];

        grad[j+1] = grad_sum / m;
    }

    double grad_sum = 0;
    for(int i=0;i<m;i++)
        grad_sum += grad_diff[i];
    grad[0] = grad_sum / m;
}

int TrainerLinear::fmincg(double* xVector, int nDim, int maxCostCalls)
{
    int i,success = 0,costFuncCount=0,lineSearchFuncCount=0;
    double ls_failed,f1,d1,z1,f0,f2,d2,f3,d3,z3,limit,z2,A,B,C;
    double df1[nDim],s[nDim],x0[nDim],df0[nDim],df2[nDim],tmp[nDim];
    double* x = xVector;

    ls_failed = 0;

    //clear to collect later
    cost_graph.clear();

    if(costFuncCount >= maxCostCalls) return 1; else costFuncCount++;
    costFunc(xVector,&f1,df1);

    for(i=0;i<nDim;i++)
    {
        s[i] = -df1[i];
    }

    d1 = 0;
    for(i=0;i<nDim;i++)
    {
        d1 += -s[i]*s[i];
    }
    z1 = 1.0f / (1 - d1);

    while(1)
    {
        for(i=0;i<nDim;i++)
        {
            x0[i] = x[i];
            df0[i] = df1[i];
        }
        f0 = f1;

        for(i=0;i<nDim;i++)
        {
            x[i] = x[i] + (z1)*s[i];
        }
        if(costFuncCount >= maxCostCalls) return 1; else costFuncCount++;
        costFunc(x,&f2,df2);

        d2 = 0;
        for(i=0;i<nDim;i++)
        {
            d2 += df2[i]*s[i];
        }

        f3 = f1;
        d3 = d1;
        z3 = -z1;

        success = 0; 
        limit = -1;
        lineSearchFuncCount = 0;
        // begin line search
        while(1)
        {
            while((( (f2) > ((f1) + RHO*(z1)*(d1))) || ( (d2) > -SIG*(d1) )) && lineSearchFuncCount < MAX)
            {
                limit = z1;
                if( (f2) > (f1) )
                {
                    z2 = z3 - (0.5f*(d3)*(z3)*(z3))/((d3)*(z3)+(f2)-(f3));
                }
                else
                {
                    A = 6*((f2)-(f3))/(z3)+3*((d2)+(d3));
                    B = 3*((f3)-(f2))-(z3)*((d3)+2*(d2));
                    z2 = (sqrt(B*B-A*(d2)*(z3)*(z3))-B)/A;
                }
                if(isnan(z2) || isinf(z2))
                {
                    z2 = (z3) * 0.5f;
                }
                A = ((z2 < INT*(z3)) ? z2 : INT*(z3));
                B = (1-INT)*(z3);
                z2 = A > B ? A : B;
                z1 = z1 + z2;

                for(i=0;i<nDim;i++)
                {
                    x[i] = x[i] + (z2)*s[i];
                }
                if(costFuncCount >= maxCostCalls) return 1; else costFuncCount++;
                lineSearchFuncCount++;
                costFunc(x,&f2,df2);

                d2 = 0;
                for(i=0;i<nDim;i++)
                {
                    d2 += df2[i] * s[i];
                }
                z3 = z3 - z2;
            }
            if( (f2 > f1 + (z1)*RHO*(d1)) || ((d2) > -SIG*(d1)) )
            {
                break; //failure
            }
            else if( d2 > SIG*(d1) )
            {
                success = 1; break; 
            }
            else if(lineSearchFuncCount >= MAX)
            {
                break;
            }
            A = 6*(f2-f3)/z3+3*(d2+d3);
            B = 3*(f3-f2)-z3*(d3+2*d2);
            z2 = -d2*z3*z3/(B+sqrt(B*B-A*d2*z3*z3));
            if(!(B*B-A*d2*z3*z3 >= 0) || isnan(z2) || isinf(z2) || z2 < 0)
            {
                if(limit < -0.5f)
                {
                    z2 = z1 * (EXT-1);
                }
                else
                {
                    z2 = (limit-z1)/2;
                }
            }
            else if((limit > -0.5) && (z2+z1 > limit))
            {
                z2 = (limit-z1)/2; 
            }	
            else if((limit < -0.5) && (z2+z1 > z1*EXT))
            {
                z2 = z1*(EXT-1.0);
            }
            else if(z2 < -z3*INT)
            {
                z2 = -z3*INT;
            }
            else if((limit > -0.5) && (z2 < (limit-z1)*(1.0-INT)))
            {
                z2 = (limit-z1)*(1.0-INT);
            }
            f3 = f2; d3 = d2; z3 = -z2;
            z1 = z1 + z2;
            for(i=0;i<nDim;i++)
            {
                x[i] = x[i] + z2*s[i];
            }
            if(costFuncCount >= maxCostCalls) return 1; else costFuncCount++;
            lineSearchFuncCount++;
            costFunc(x,&f2,df2);
            d2 = 0;
            for(i=0;i<nDim;i++)
            {
                d2 += df2[i]*s[i];
            }
        }
        // line search ended
        if(success)
        {
            f1 = f2;
            //printf("Cost: %e\n", f1);
            cost_graph.push_back(f1);

            A = 0;
            B = 0;
            C = 0;
            for(i=0;i<nDim;i++)
            {
                A += df1[i]*df1[i];
                B += df2[i]*df2[i];
                C += df1[i]*df2[i];
            }
            for(i=0;i<nDim;i++)
            {
                s[i] = ((B-C)/A)*s[i] - df2[i];
            }
            for(i=0;i<nDim;i++)
            {
                tmp[i] = df1[i]; df1[i] = df2[i]; df2[i] = tmp[i];
            }
            d2 = 0;
            for(i=0;i<nDim;i++)
            {
                d2 += df1[i] * s[i];
            }
            if(d2 > 0)
            {
                for(i=0;i<nDim;i++)
                {
                    s[i] = -df1[i];
                }
                d2 = 0;
                for(i=0;i<nDim;i++)
                {
                    d2 += -s[i]*s[i];
                }
            }
            A = d1/(d2-COST_FUNC_DATATYPE_MIN);
            z1 = z1 * ((RATIO < A) ? RATIO : A);
            d1 = d2;
            ls_failed = 0;
        }
        else
        {
            f1 = f0;
            for(i=0;i<nDim;i++)
            {
                x[i] = x0[i];
                df1[i] = df0[i];
            }
            if(ls_failed)
            {
                break;
            }
            for(i=0;i<nDim;i++)
            {
                tmp[i] = df1[i]; df1[i] = df2[i]; df2[i] = tmp[i];
            }
            for(i=0;i<nDim;i++)
            {
                s[i] = -df1[i]; 
            }
            d1 = 0;
            for(i=0;i<nDim;i++)
            {
                d1 += -s[i]*s[i];
            }
            z1 = 1/(1-d1);
            ls_failed = 1;
        }
    }
    return 2;
}
