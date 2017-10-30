#include "Job.h"

Job::Job()
{
	id = mutator_id = priority = -1;
}

Job::Job(string msg)
{
	id = mutator_id = priority = -1;
	parseMessage(msg);
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
