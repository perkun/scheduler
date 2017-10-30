#ifndef JOB_H_
#define JOB_H_

#include <iostream>
#include <stdio.h>

using namespace std;

class Job
{
public:
	Job();
	Job(string);
	void parseMessage(string);
	void printInfo();
	void printIdPriority();

	int id, mutator_id;
	string path;
	string program;
	int priority;

};

#endif /* JOB_H_ */
