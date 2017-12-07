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

	Job& operator[] (const int index);

	void pushBackFromMessages( vector<string> &messages);
	void pushBack( Job );
	int getMaxPriority();
	int getPriorityIndex(int);
	void erase(int);
	Job getJobAtIndex(int);
	Job* getPointerAt(int);
	void printAll();
	void clear();
	unsigned int size();

protected:
	vector<Job> jobs;

};


// class ExecutingList : public JobList
// {
// public:
// 	int status;
//
// };



#endif /* JOBLIST_H_ */
