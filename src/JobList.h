#ifndef JOBLIST_H_
#define JOBLIST_H_

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>

#include "Job.h"

using namespace std;

class JobList
{
public:

	void pushBackJobs( vector<string> &messages);
	int getMaxPriority();
	void clear();
	int getPriorityIndex(int);
	void erase(int);

	vector<Job> jobs;

};

#endif /* JOBLIST_H_ */
