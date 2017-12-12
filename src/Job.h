#ifndef JOB_H_
#define JOB_H_

#include <iostream>
#include <stdio.h>
#include <time.h>

using namespace std;


class Job
{
public:

	enum class Status : int {
		NEW = 0,
		SENT,
		FINISHED,
		WAITING,
		HALTED,
		ERROR,
		NUM_STATUSES
	};

	enum class Service : int {
		FOTO = 0,
		RADAR
	};

	enum class Type : int {
		NORMAL = 0,
		ESTIMATE_RESOURCES
	};

	enum class CrancshaftStatus : int {
		OK = 0,
		ERROR,
		NUM_STAUTSES
	};

	static long ID;

	Job();
	Job(string);
	void parseMessage(string);
	void printInfo();
	void printIdPriority();

	void setPriority(int);
	int getPriority();

	void setStatus(Status);
	Status getStatus();
	bool isStatus(Status s);

	bool isType(Type t);

	long getUniqueId();
	int getTaskId();

	void setResources(int);
	int getResources();
	int estimateResources(int MAX);

	void setComputerIp(string);
	void setComputerIp(const char *);
	string getComputerIp();

	Service getService();

	int getMutatorId();

	string getPath();

	string getMessageToCrankshaft();


	double getExecutonTimeSeconds();
	void startClock();
	void stopClock();




protected:
	time_t start, end;

	Status status;
	Type type;
	// rzeczy z kolejki zczytane
	int task_id, mutator_id, priority;
	long unique_id;
	string path, program;

	// do wykonywania
	// time start, stop
	int resources;
	Service service;
	string computer_ip;



};

#endif /* JOB_H_ */
