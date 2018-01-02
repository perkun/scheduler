#include "Computer.h"

Computer::Computer()
{
	blocked = false;
	blocking_job_id = -1;
	resources = 0;
	num_jobs = 0;

}

bool Computer::operator<(const Computer &c2)
{
	return this->resources < c2.resources;
}

void Computer::block(long job_id)
{
	blocked = true;
	blocking_job_id = job_id;
}

void Computer::unblock()
{
	blocked = false;
	blocking_job_id = -1;
}

void Computer::addJob(int job_resources)
{
	resources += job_resources;
	num_jobs++;
}

bool Computer::hasService(int job_service)
{
	for (int service: services)
		if (service == job_service)
			return true;
	return false;
}
