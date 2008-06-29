#pragma once
#include "stdafx.h"

struct WeakHyp
{
	double alpha, threshold;
	int column;
	map<string, short> parities;
};

class Classifier
{
public:
	Classifier(const char* shypFile);
	~Classifier(void);
	string Classify(const double *data);
	WeakHyp& operator[](int i) {
		return _weakHyps[i];
	}

	map<string, double> votes;
private:
	vector<WeakHyp> _weakHyps;
};
