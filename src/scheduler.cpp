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

int mutator_queue_id = 1234, crankshaft_queue_id = 1234;
long mutator_message_type = 1, scheduler_message_type = 2,
	 crankshaft_message_type = 3;
MessageQueue msq;

void moveFromPendingToExecuting(JobList &pending_jobs, JobList &executing_jobs);
void executeJobs(JobList &executing_jobs);


int main() {


	JobList pending_jobs;
	JobList executing_jobs;


	//============================== MAIN LOOP: ==============================
	while (1)
	{
		//czytaj wszystkie dostępne komunikaty czekające w kolejce
		vector<string> messages = msq.readQueue(mutator_queue_id,
			   									mutator_message_type);
		if (messages.empty())
			printf("msqueue empty\n");
		// wyślij je na koniec kolejki zadań
		pending_jobs.pushBackFromMessages(messages);

		// priorytetowanie
		moveFromPendingToExecuting(pending_jobs, executing_jobs);

		// w executing_jobs leżą teraz joby, które trzeba skwencyjnie od 0,1,...
		// zdjąć do wykonania na komputerze (FIFO)

		executeJobs(executing_jobs);






		printf("pending_jobs size: %u\n", pending_jobs.size());
		printf("executing_jobs size: %u\n", executing_jobs.size());

		printf("======= pending =======\n");
		pending_jobs.printAll();
		printf("=======================\n");
		printf("====== executing ======\n");
		executing_jobs.printAll();
		printf("=======================\n");



	///////////////////// USER CONTROL ///////////////////
		char c = getchar();

		if (c == 'c')
			pending_jobs.clear();
		if (c == 'x')
			executing_jobs.clear();
		if (c == 'q')
			break;
		else if (c == 'p')
			pending_jobs.printAll();
		else if ( c == 'e' )
			executing_jobs.printAll();
	}



	return 0;
}


void moveFromPendingToExecuting(JobList &pending_jobs, JobList &executing_jobs)
{
	/** bierze pending_jobs i wrzuca do executing_jobs w kolejności zależnej od
	 * priorytetów. Zadania z priorytetm x pojawią się x razy. Nie wszystkie
	 * zadanie zostaną przełożone; tylko $$ Sum_{i}^{i=x} i $$
	 */

	map<int, int> priority_counter;
	int maximum = pending_jobs.getMaxPriority();
	// wyczysc counter
	priority_counter.clear();
	// 		printf("max priority: %d\n", maximum);
	//
	// ================ PRIORYTETOWANIE ================
	while (maximum > 0)
	{
		if (priority_counter[maximum] < maximum)
		{
			// find index of a job in pending_jobs with priority==maximum
			int max_index = pending_jobs.getPriorityIndex(maximum);

			if (max_index == -1) // flaga na nieznalezienie
			{
				maximum--;
				continue;
			}

			// add to executing jobs.
			executing_jobs.pushBack( pending_jobs.getJobAtIndex(max_index) );
			// remove from pending jobs
			pending_jobs.erase(max_index);

			priority_counter[maximum]++;
		}
		else
			maximum--;
	}
	// ========================================

	// STAN RZECZY po priorytetowaniu:
	// 		w executing jobs leżą zadania do policzenia w odpowiedniej
	// 		kolejności, tj. trzeba zdejmować od indeksów 0,1,... (FIFO)
	//
	// 		pending jobs odpowednio pomniejszona o zadania, które poszły do
	// 		executing jobs
}


void executeJobs(JobList &executing_jobs)
{
	/**
	 * Jedzie po executing_jobs i szuka tych ze statusem NEW, szuka kompa i
	 * wysyła do kolejki Crankshaftu
	 */

	// KOLEJKA FIFO, zdejmujemy (ustawiamy status SENT) z kolejki
	for (int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].getStatus() == Job::Status::NEW)
		{

			// 1) oszacuj zasoby
			executing_jobs[i].estimateResources();

			// 2) find computer with free resources

			// 3) send message to Crankshaft message queue
			msq.sendMessage(
					crankshaft_queue_id,
					crankshaft_message_type,
					executing_jobs[i].getMessageToCrankshaft()
					);

			// and set status to SENT
			executing_jobs[i].setStatus(Job::Status::SENT);
		}
}


