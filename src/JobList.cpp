#include "JobList.h"


// Job& JobList::operator[](int index)
// {
// 	return jobs[index];
// }

void JobList::pushBackFromMessages(vector<string> &messages)
{
	for (auto m: messages)
	{
		Job j(m);

		jobs[j.getPriority()].push_back( j );
	}
}

void JobList::pushBack( Job j )
{
	jobs[j.getPriority()].push_back(j);
}

int JobList::findMaxPriority()
{
	if (jobs.empty())
		return 0;

	int MAX = 0;
	for (auto const& x: jobs)
		if (!x.second.empty() && x.first > MAX)
			MAX = x.first;
	return MAX;
}

void JobList::clear()
{
	jobs.clear();
}


void JobList::erase(int priority, int index)
{
	jobs[priority].erase(jobs[priority].begin() + index);
}

map<int, vector<Job> >::iterator JobList::begin()
{
	return jobs.begin();
}

map<int, vector<Job> >::iterator JobList::end()
{
	return jobs.end();
}

map<int, vector<Job> >::reverse_iterator JobList::rbegin()
{
	return jobs.rbegin();
}

map<int, vector<Job> >::reverse_iterator JobList::rend()
{
	return jobs.rend();
}

unsigned int JobList::size()
{
	return jobs.size();
}

// Job JobList::getJobAtIndex(int index)
// {
// 	return jobs[index];
// }
//
// Job* JobList::getPointerAt(int index)
// {
// 	return &jobs[index];
// }

// void JobList::printAll()
// {
// 	for (Job j: jobs)
// 		j.printIdPriority();
// }

// int JobList::findIndexWithPriority(int priority)
// {
// 	for (unsigned int i = 0; i < jobs.size(); i++)
// 	{
// 		if ( jobs[i].getPriority() == priority )
// 			return i;
// 	}
// 	return -1;
// }
//==============================================================================

