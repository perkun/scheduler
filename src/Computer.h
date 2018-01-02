#ifndef COMPUTER_H_
#define COMPUTER_H_

#include <iostream>
#include <stdio.h>
#include <vector>

using namespace std;

class Computer
{
public:
	Computer();
	bool operator<(const Computer &c2);
	void block(long job_id);
	void unblock();
	void addJob(int job_resources);
	bool hasService(int job_service);

	bool blocked; ///< if true, no new jobs will be assigned
	vector<int> services; ///< list of available services on this computer
	string ip; ///< IP adress
	int resources; ///< resources in MB currently in use
	int	num_jobs; ///< number of assigned jobs
	long blocking_job_id; ///< unique id of a job for which this computer is blocked


};

#endif /* COMPUTER_H_ */
