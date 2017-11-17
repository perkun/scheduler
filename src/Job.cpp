#include "Job.h"

Job::Job()
{
	id = mutator_id = priority = service = -1;
	resources = -1;

	computer_ip = "unknown";

	status = Status::NEW;
}

Job::Job(string msg)
{
	id = mutator_id = priority = -1;
	parseMessage(msg);

	computer_ip = "unknown";

	status = Status::NEW;
}



void Job::parseMessage(string msg)
{
	char buff1[1000]; //buff2[1000];

	sscanf(msg.c_str(),
		   	"%d %d %d %s %d %d",
			&id, &mutator_id, &priority, buff1, &service, &resources);
	path = buff1;
// 	program = buff2;

// 	printf("service: %d\n", service);
	if (id < 0 || mutator_id < 0 || priority < 0)
		perror("invalid message");

}

void Job::printInfo()
{

	printf("\nid: %d\nmutator_id: %d\npriority: %d\npath: %s\nprogram: %s\n",
			id, mutator_id, priority, path.c_str(), program.c_str());

}

void Job::printIdPriority()
{
	printf("id: %d\tpriority: %d\n", id, priority);
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
	return 0.2;
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
	//		task_id
	//		mutator_id
	//		path
	//		program

	char buf[100];
	string s;

	s = computer_ip;

	sprintf(buf, " %d", id);
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

int Job::getId()
{
	return id;
}


