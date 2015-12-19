#ifndef TRAINER_LINEAR_SIGMOID_H
#define TRAINER_LINEAR_SIGMOID_H

#include "trainer_linear.h"

class TrainerLinearSigmoid : public TrainerLinear
{
public:
	TrainerLinearSigmoid() {}
	~TrainerLinearSigmoid() {}
	
	void costFunc(double* theta, double* cost, double* gradVec);
};

#endif

