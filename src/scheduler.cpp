#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <map>
#include <time.h>

#include "MessageQueue.h"
#include "Job.h"
#include "JobList.h"

#define MAXSIZE 128

using namespace std;

int mutator_queue_id = 1234, crankshaft_queue_id = 1234,
   	scheduler_queue_id = 1234;
long mutator_message_type = 1, scheduler_message_type = 2,
	 crankshaft_message_type = 3;
MessageQueue msq;

void moveFromPendingToExecuting(JobList &pending_jobs, JobList &executing_jobs);
void executeJobs(JobList &executing_jobs);
void updateStatusFromCrankshaft(JobList &executing_jobs);
void manageFinishedJobs(JobList &executing_jobs);


int main() {

	JobList pending_jobs;
	JobList executing_jobs;


	//============================== MAIN LOOP: ==============================
	while (1)
	{
		// poinformować mutator o zakonzeniu zadania
		// zdecydować co robić, jak wróci błąd wykonania zadania
		// update bazy danych z obciazeniami komputerów

		// przeczytaj kolejke Crankshafta i ustal odpowiednie statusy
		if (executing_jobs.size() > 0 )
		{
			updateStatusFromCrankshaft(executing_jobs);
			// zakończone zadania zdejmij z kolejki i zwolnij zasoby komputerów
			manageFinishedJobs(executing_jobs);

		}




		for (unsigned int i = 0; i < executing_jobs.size(); i++)
			cout << "id status: " << executing_jobs[i].getId() << " " <<
				executing_jobs[i].getStatus() << "\n";


		// czytaj wszystkie dostępne komunikaty czekające w kolejce
		vector<string> messages = msq.readQueue(mutator_queue_id,
			   									mutator_message_type);
		if (messages.empty())
			printf("msqueue empty\n");
		// wyślij je na koniec kolejki zadań
		pending_jobs.pushBackFromMessages(messages);

		// priorytetowanie
		moveFromPendingToExecuting(pending_jobs, executing_jobs);


		// w executing_jobs leżą teraz joby, które trzeba skwencyjnie od 0,1,...
		// wysłać do wykonania na komputerze (FIFO)
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
	for (unsigned int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].getStatus() == Job::Status::NEW)
		{

			// 1) oszacuj zasoby
			executing_jobs[i].estimateResources();

			// 2) find computer with free resources
			// 2.5) set ip adress
			executing_jobs[i].setComputerIp("150.254.66.0");

// 			cout <<  executing_jobs[i].getMessageToCrankshaft() << "\n";

			// start timer
			executing_jobs[i].startClock();

			// 3) send message to Crankshaft message queue
// 			if (
			msq.sendMessage(
					crankshaft_queue_id,
					crankshaft_message_type,
					executing_jobs[i].getMessageToCrankshaft()
				);
// 				   	!= -1)
// 				printf("wysłalem\n");

			// and set status to SENT
			executing_jobs[i].setStatus(Job::Status::SENT);
		}


}

void updateStatusFromCrankshaft(JobList &executing_jobs)
{
	/// przeczytaj kolejke od crankshafta i ustaw odpowiednie statusy zadaniom
	vector<string> messages =
		msq.readQueue(scheduler_queue_id, scheduler_message_type);

	if ( messages.empty() )
		return;

	for (string s: messages)
	{
		int id = -1, status = -1;
		sscanf(s.c_str(), "%d %d", &id, &status);

		if (status < 0 || status >= Job::Status::NUM_STATUSES)
		{
			printf("wrong status (%d) from crankshaft for job %d", status, id);
			status = Job::Status::ERROR;
// 			continue;
		}

		for (unsigned int i = 0; i < executing_jobs.size(); i++)
			if (executing_jobs[i].getId() == id)
				executing_jobs[i].setStatus(status);

	}

}

void manageFinishedJobs(JobList &executing_jobs)
{
	char buf[MAXSIZE];

	for (unsigned int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].getStatus() == Job::Status::FINISHED)
		{
			// send message to mutator
			sprintf(buf,
				   	"%d %d",
					executing_jobs[i].getId(),
					Job::Status::FINISHED );
			msq.sendMessage(mutator_message_type, scheduler_message_type, buf);

			// stop timer
			executing_jobs[i].stopClock();

			// release resources (DataBase)

			// remove from the list
			executing_jobs.erase(i);
			i--;
		}

}
