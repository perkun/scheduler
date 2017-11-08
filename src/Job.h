#ifndef JOB_H_
#define JOB_H_

#include <iostream>
#include <stdio.h>
#include <time.h>

using namespace std;

class Job
{


public:
	Job();
	Job(string);
	void parseMessage(string);
	void printInfo();
	void printIdPriority();
	int getPriority();
	void setPriority(int);
	void setStatus(int);
	int getStatus();
	int getId();

	string getMessageToCrankshaft();

	double getExecutonTimeSeconds();
	void startClock();
	void stopClock();

	void estimateResources();
	void setComputerIp(string);
	void setComputerIp(const char *);
	string getComputerIp();



	// 	enum jobType {
	// 		PHOTOMETRY,
	// 		RADAR,
	// 		OCCULT,
	// 		BALANING
	// 	};

	enum Status {
		NEW,
// 		PENDING,		// ???
// 		EXECUTING,		// ???
		SENT,
		FINISHED,
		HALTED,
		ERROR,
		NUM_STATUSES
	};


protected:
	time_t start, end;

	int status;
	// rzeczy z kolejki zczytane
	int id, mutator_id, priority;
	string path, program;

	// do wykonywania
	// time start, stop
	double resources;
	int job_type;
	string computer_ip;



};

#endif /* JOB_H_ */
