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

	void setPriority(int);
	int getPriority();

	void setStatus(int);
	int getStatus();

	int getId();

	void setResources(double);
	int getResources();

	void setComputerIp(string);
	void setComputerIp(const char *);
	string getComputerIp();

	int getService();

	int getMutatorId();

	string getPath();

	string getMessageToCrankshaft();

	double estimateResources();

	double getExecutonTimeSeconds();
	void startClock();
	void stopClock();


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

	enum Service {
		FOTO,
		RADAR
	};


protected:
	time_t start, end;

	int status;
	// rzeczy z kolejki zczytane
	int id, mutator_id, priority;
	string path, program;

	// do wykonywania
	// time start, stop
	int resources;
	int service;
	string computer_ip;



};

#endif /* JOB_H_ */
