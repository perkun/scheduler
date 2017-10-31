#include "Job.h"

Job::Job()
{
	id = mutator_id = priority = job_type = -1;
	resources = -1;

	computer_ip = "150.254.66.29";

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
	char buff1[1000], buff2[1000];

	sscanf(msg.c_str(), "%d %d %d %s %s", &id, &mutator_id, &priority,
			buff1, buff2);
	path = buff1;
	program = buff2;

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

void Job::estimateResources()
{
	// TODO
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



string Job::getMessageToCrankshaft()
{
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


void Job::setStatus(int s)
{
	status = s;
}

int Job::getStatus()
{
	return status;
}


