#include "Job.h"

Job::Job()
{
	unique_id = mutator_id = priority = service = -1;
	resources = -1;

	computer_ip = "unknown";

	status = Status::NEW;


}

Job::Job(string msg)
{
	unique_id = mutator_id = priority = -1;
	parseMessage(msg);

	computer_ip = "unknown";

	status = Status::NEW;
}



void Job::parseMessage(string msg)
{
	cout << msg << "\n";
	char buff1[1000]; //buff2[1000];

	sscanf(msg.c_str(),
		   	"%d %d %d %s %d %d",
			&mutator_id, &task_id, &priority, buff1, &service, &resources);
	path = buff1;

 	unique_id = ID;
	ID++;

// 	program = buff2;

// 	printf("service: %d\n", service);
	if (task_id < 0 || mutator_id < 0 || priority < 0)
		perror("invalid message");

}

void Job::printInfo()
{

	printf("\ntask_id: %d\nmutator_id: %d\npriority: %d\npath: %s\nprogram: %s\n",
			task_id, mutator_id, priority, path.c_str(), program.c_str());

}

void Job::printIdPriority()
{
	printf("unique_id: %d\tpriority: %d\n", unique_id, priority);
}

int Job::getPriority()
{
	return priority;
}

void Job::setPriority(int p)
{
	priority = p;
}

double Job::estimateResources()
{
	// TODO
	return 200;
}

void Job::setComputerIp(string s)
{
	computer_ip = s;
}

void Job::setComputerIp(const char *s)
{
	computer_ip = s;
}

string Job::getComputerIp()
{
	return computer_ip;
}

int Job::getService()
{
	return service;
}

int Job::getMutatorId()
{
	return mutator_id;
}

void Job::setResources(double r)
{
	resources = r;
}

int Job::getResources()
{
	return resources;
}

string Job::getPath()
{
	return path;
}



string Job::getMessageToCrankshaft()
{
	/// returns string with:
	//		computer_ip
	//		job_id AKA unique_id
	//		mutator_id
	//		path

	char buf[1000];
	string s;

	s = computer_ip;

	sprintf(buf, " %d", unique_id);
	s += buf;

	sprintf(buf, " %d ", mutator_id);
	s += buf;

	s += path + " " + program ;

	// SPACJE !!!!!!!

	cout << s << "\n";
	return s;
}

double Job::getExecutonTimeSeconds()
{
// 	time(&end);
	return difftime(end, start);
}

void Job::startClock()
{
	time(&start);
}

void Job::stopClock()
{
	time(&end);
}


void Job::setStatus(int s)
{
	status = s;
}

int Job::getStatus()
{
	return status;
}

bool Job::isStatus(int s)
{
	if (status == s)
		return true;
	else
		return false;
}

long Job::getUniqueId()
{
	return unique_id;
}

int Job::getTaskId()
{
	return task_id;
}


