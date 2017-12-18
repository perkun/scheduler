#include "Job.h"

Job::Job()
{
	unique_id = mutator_id = priority =  -1;
	resources = 0;

	computer_ip = "unknown";

	status = Status::NEW;
	type = Type::NORMAL;
}

Job::Job(string msg)
{

	unique_id = mutator_id = priority = -1;
	if (parseMessage(msg) < 0)
		return;

	if (resources < 0)
	{
		type = Type::ESTIMATE_RESOURCES;
	}
	else
		type = Type::NORMAL;


	computer_ip = "unknown";

	status = Status::NEW;
}



int Job::parseMessage(string msg)
{
// 	cout << msg << "\n";
	char buff1[1000]; //buff2[1000];
	memset(buff1, '\0', sizeof buff1);

	int s;
	sscanf(msg.c_str(),
		   	"%d %d %d %s %d %d",
			&mutator_id, &task_id, &priority, buff1, &s, &resources);
	path = buff1;

	service = (Job::Service) s;

 	unique_id = ID;
	ID++;

// 	program = buff2;

// 	printf("service: %d\n", service);
	if (task_id < 0 || mutator_id < 0 || priority < 0)
	{
		perror("invalid message");
		cout << msg.c_str() << "\n";
		return -1;
	}

	return 0;
}

void Job::printInfo()
{

	printf("\ntask_id: %d\nmutator_id: %d\npriority: %d\npath: %s\nprogram: %s\n",
			task_id, mutator_id, priority, path.c_str(), program.c_str());

}

void Job::printIdPriority()
{
	printf("unique_id: %ld\tpriority: %d\n", unique_id, priority);
}

int Job::getPriority()
{
	return priority;
}

void Job::setPriority(int p)
{
	priority = p;
}

int Job::estimateResources(int MAX)
{
	if (isType(Type::ESTIMATE_RESOURCES))
	{
		resources = MAX;
		return MAX;
	}
	else
		return resources;
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

Job::Service Job::getService()
{
	return service;
}

int Job::getMutatorId()
{
	return mutator_id;
}

void Job::setResources(int r)
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

string Job::getMessageToMutator()
{
	char buf[1000];
	sprintf(buf,
			"%ld %d %d %d",
			getUniqueId(),
			getMutatorId(),
			getTaskId(),
			(int)getStatus() );

	string s = buf;
	return s;
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

	sprintf(buf, " %ld", unique_id);
	s += buf;

	sprintf(buf, " %d ", mutator_id);
	s += buf;

	s += path;

	// SPACJE !!!!!!!

// 	cout << s << "\n";
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

void Job::setStatus(Status s)
{
	status = s;
}

Job::Status Job::getStatus()
{
	return status;
}


bool Job::isStatus(Status s)
{
	if (status == s)
		return true;
	else
		return false;
}

bool Job::isType(Type t)
{
	if (type == t)
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


