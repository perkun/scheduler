#include "JobList.h"

void JobList::pushBackJobs(vector<string> &messages)
{

	for (auto m: messages)
		jobs.push_back( Job(m) );
}

int JobList::getMaxPriority()
{
	if (jobs.empty())
		return 0;

	int max = -1e9;

	for (Job job: jobs)
		if (job.priority > max)
			max = job.priority;

	return max;
}

void JobList::clear()
{
	jobs.clear();
}

int JobList::getPriorityIndex(int priority)
{
	for (int i = 0; i < jobs.size(); i++)
	{
		if ( jobs[i].priority == priority )
			return i;
	}
	return -1;
}

void JobList::erase(int index)
{
	jobs.erase(jobs.begin() + index);
}
