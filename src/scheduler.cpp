#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <map>

#include "MessageQueue.h"
#include "Job.h"
#include "JobList.h"

#define MAXSIZE 128

using namespace std;



int main() {

	int mutator_queue_id = 1234;
	long mutator_message_type = 1;

	MessageQueue msq;
	JobList jobs;
	map<int, int> priority_counter;

	// TESTING VARIABLES
	vector<Job> executingJobs;


	while (1)
	{
		//czytaj wszystkie dostępne komunikaty czekające w kolejce
		vector<string> messages = msq.readQueue(mutator_queue_id,
			   									mutator_message_type);
		if (messages.empty())
			printf("msqueue empty\n");
		// wyślij je na koniec kolejki zadań
		jobs.pushBackJobs(messages);


		// priorytetowanie
		int maximum = jobs.getMaxPriority();
		// wyczysc counter
		priority_counter.clear();
// 		printf("max priority: %d\n", maximum);
		while (maximum > 0)
		{
			if (priority_counter[maximum] < maximum)
			{
				// find index of a job in jobs with priority==maximum
				int max_index = jobs.getPriorityIndex(maximum);

				if (max_index == -1) // flaga na nieznalezienie
				{
					maximum--;
					continue;
				}

				// EXECUTE JOB
				executingJobs.push_back(jobs.jobs[max_index]);
				// zdejmij z kolejki do wykonywania
				jobs.erase(max_index);


				priority_counter[maximum] = priority_counter[maximum] + 1;

			}
			else
				maximum--;
		}


		printf("======= pending =======\n");
		for (auto j: jobs.jobs)
			j.printIdPriority();
		printf("=======================\n");
		printf("====== executing ======\n");
		for (auto j: executingJobs)
			j.printIdPriority();
		printf("=======================\n");
	///////////////////// USER CONTROL ///////////////////
		char c = getchar();

		if (c == 'c')
			jobs.clear();
		if (c == 'x')
			executingJobs.clear();
		if (c == 'q')
			break;
		else if (c == 'p')
			for (auto j: jobs.jobs)
				j.printIdPriority();
// 				j.printInfo();
		else if ( c == 'e' )
			for (auto j: executingJobs)
				j.printIdPriority();
	}



	return 0;
}



