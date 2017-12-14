#ifndef JOBLIST_H_
#define JOBLIST_H_

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>

#include "Job.h"

using namespace std;

class JobList
{
public:

// 	Job& operator[] (const int index);
	map<int, vector<Job> >::iterator begin();
	map<int, vector<Job> >::iterator end();
	map<int, vector<Job> >::reverse_iterator rbegin();
	map<int, vector<Job> >::reverse_iterator rend();

	void pushBackFromMessages( vector<string> &messages);
	void pushBack( Job );
	int findMaxPriority();
// 	int findIndexWithPriority(int);
	void erase(int, int);
// 	Job getJobAtIndex(int);
// 	Job* getPointerAt(int);
// 	void printAll();
	void clear();
	unsigned int size();

protected:
	map<int, vector<Job> > jobs;

};


// class ExecutingList : public JobList
// {
// public:
// 	int status;
//
// };



#endif /* JOBLIST_H_ */
