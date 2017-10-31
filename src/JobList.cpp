#include "JobList.h"


Job& JobList::operator[](int index)
{
	return jobs[index];
}

void JobList::pushBackFromMessages(vector<string> &messages)
{

	for (auto m: messages)
		jobs.push_back( Job(m) );
}

void JobList::pushBack( Job j )
{
	jobs.push_back(j);
}

int JobList::getMaxPriority()
{
	if (jobs.empty())
		return 0;

	int max = -1e9;

	for (Job job: jobs)
		if (job.getPriority() > max)
			max = job.getPriority();

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
		if ( jobs[i].getPriority() == priority )
			return i;
	}
	return -1;
}

void JobList::erase(int index)
{
	jobs.erase(jobs.begin() + index);
}

Job JobList::getJobAtIndex(int index)
{
	return jobs[index];
}

void JobList::printAll()
{
	for (Job j: jobs)
		j.printIdPriority();
}

unsigned int JobList::size()
{
	return jobs.size();
}

//==============================================================================

