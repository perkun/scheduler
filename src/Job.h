#ifndef JOB_H_
#define JOB_H_

#include <iostream>
#include <stdio.h>
#include <time.h>
#include <string.h>

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

	static long ID; ///< Increments when a new Job object is created. Starts at 1

	Job();
	Job(string);
	int parseMessage(string);
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
	string getMessageToMutator();


	double getExecutonTimeSeconds();
	void startClock();
	void stopClock();




protected:
	time_t start, end;

	Status status; ///< status of the job
	Type type;		///< job is handled in different ways based on type
	// rzeczy z kolejki zczytane
	int task_id, mutator_id, priority;

	/** unique id number.
	  Resets to 1 on scheduler startup and increments when new jobs are added*/
	long unique_id;
	string path, program;

	// do wykonywania
	int resources; ///< GPU resources in MB needed for the job
	Service service; ///< computer has to be capable of executing this job
	string computer_ip;



};

#endif /* JOB_H_ */
