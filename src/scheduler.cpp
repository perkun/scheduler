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
#include <pqxx/pqxx>
#include <unistd.h>

#include "MessageQueue.h"
#include "Job.h"
#include "JobList.h"
// #include "Database.h"

#define MAXSIZE 128
#define MAX_WAITING_JOBS 100

using namespace std;
using namespace pqxx;

enum Status {
	OK,
	ERROR
};

int COMPUTER_MAX_RESOURCES = 1600;

int mutator_scheduler_queue = 1000,
	scheduler_mutator_queue = 1001,
   	scheduler_crankshaft_queue = 2001,
   	crankshaft_scheduler_queue = 2000,
	scheduler_manager_queue = 5000,
	mutator_status_queue = 6000,
	crankshaft_status_queue =  7000;

long mutator_scheduler_msgt = 1,
	 crankshaft_scheduler_msgt = 1,
	 scheduler_crankshaft_msgt = 1,
	 scheduler_manager_msgt = 1,
	 scheduler_mutator_msgt = 1;

MessageQueue msq;

long Job::ID = 1;


connection services("dbname=grzeslaff user=grzeslaff password=grigori8 hostaddr=150.254.66.29 ");
// connection services("dbname=grzeslaff user=grzeslaff password=grigori hostaddr=127.0.0.1");
transaction<> servicesTransaction(services, "Services transaction");
result res;

void moveFromPendingToExecuting(JobList *pending_jobs, JobList *executing_jobs);
void executeNewJobs(JobList *executing_jobs);
void executeWaitingJobs(JobList *executing_jobs);
void updateStatusFromCrankshaft(JobList *executing_jobs);
void manageFinishedJobs(JobList *executing_jobs);
void printTable(result &res);
// void takeJobsFromDatabase(JobList &executing_jobs);
void clearJobsInDatabase();
bool isMessageStatusOk(int k, long t);
void insertIntoJobs(Job *job);
void deleteFromBlocked(Job *job);





int main() {

	if (Job::ID <= 0)
	{
		cout << "Job::ID was incorectly set. It must start from 1." << "\n";
		return 1;
	}


	if (!services.is_open()) {
		cout << "not able to connect to database" << endl;
	}

	JobList *pending_jobs = new JobList;
	JobList *executing_jobs = new JobList;

	// wyczysc kolejke
	msq.cleanQueue(mutator_scheduler_queue);
	msq.cleanQueue(scheduler_mutator_queue);
	msq.cleanQueue(crankshaft_scheduler_queue);
	msq.cleanQueue(crankshaft_status_queue);
	msq.cleanQueue(scheduler_crankshaft_queue);
	msq.cleanQueue(mutator_status_queue);
	msq.cleanQueue(scheduler_manager_queue);

	// wysłać info, że sie uruchomiło
	msq.sendMessage(scheduler_manager_queue, scheduler_manager_msgt,
			"scheduler started");
	// wczytać cokolwiek zalega w bazie jobs (czyli ze statusem SENT)
	clearJobsInDatabase();


	cout << "main loop..." << "\n";
	//============================== MAIN LOOP: ==============================
	while (1)
	{

		// zdecydować co robić, jak wróci błąd wykonania zadania

		// przeczytaj kolejke Crankshafta i ustal odpowiednie statusy
		if (executing_jobs->size() > 0 )
		{
			updateStatusFromCrankshaft(executing_jobs);

			// zakończone zadania zdejmij z kolejki i zwolnij zasoby komputerów
			manageFinishedJobs(executing_jobs);

			// manageErrorJobs
		}



		// czytaj wszystkie dostępne komunikaty od mutatora w kolejce
		vector<string> messages = msq.readQueue(mutator_scheduler_queue,
			   									mutator_scheduler_msgt);
// 		if (messages.empty())
// 			printf("msqueue empty\n");

		// wyślij je na koniec kolejki zadań
		pending_jobs->pushBackFromMessages(messages);

		// priorytetowanie
		moveFromPendingToExecuting(pending_jobs, executing_jobs);

		// w executing_jobs leżą teraz joby, które trzeba skwencyjnie od 0,1,...
		// wysłać do wykonania na komputerze (FIFO)
		executeWaitingJobs(executing_jobs);
		executeNewJobs(executing_jobs);



// 		printf("%u\t%u\tPending, Executing\n",
// 			   	pending_jobs.size(),
// 				executing_jobs.size());
// 		printf("%u\tpending_jobs size\n", pending_jobs.size());
// 		printf("%u\texecuting_jobs size\n", executing_jobs.size());

// 		printf("======= pending =======\n");
// 		pending_jobs.printAll();
// 		printf("=======================\n");
// 		printf("====== executing ======\n");
// 		executing_jobs.printAll();
// 		printf("=======================\n");

	///////////////////// USER CONTROL ///////////////////
// 		char c = getchar();
//
// 		if (c == 'c')
// 			pending_jobs.clear();
// 		if (c == 'x')
// 			executing_jobs.clear();
// 		if (c == 'q')
// 			break;
// 		else if (c == 'p')
// 			pending_jobs.printAll();
// 		else if ( c == 'e' )
// 			executing_jobs.printAll();

 		sleep(1);

		servicesTransaction.exec("COMMIT");
	}

	servicesTransaction.commit();
	services.disconnect();

	return 0;
}


void moveFromPendingToExecuting(JobList *pending_jobs, JobList *executing_jobs)
{
	/** bierze pending_jobs i wrzuca do executing_jobs w kolejności zależnej od
	 * priorytetów. Zadania z priorytetm x pojawią się x razy. Nie wszystkie
	 * zadanie zostaną przełożone; tylko $$ Sum_{i}^{i=x} i $$
	 */

	map<int, int> priority_counter;
	int maximum = pending_jobs->getMaxPriority();
	// wyczysc counter
	priority_counter.clear();

	// ================ PRIORYTETOWANIE ================
	while (maximum > 0)
	{
		if (priority_counter[maximum] < maximum)
		{
			// find index of a job in pending_jobs with priority==maximum
			int max_index = pending_jobs->getPriorityIndex(maximum);

			if (max_index == -1) // flaga na nieznalezienie
			{
				maximum--;
				continue;
			}

			// add to executing jobs.
			executing_jobs->pushBack( pending_jobs->getJobAtIndex(max_index) );
			// remove from pending jobs
			pending_jobs->erase(max_index);

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

void executeWaitingJobs(JobList *executing_jobs)
{
	char query[1000];


	for (unsigned int i = 0; i < executing_jobs->size(); i++)
		if (executing_jobs->getPointerAt(i)->isStatus(Job::Status::WAITING) )
		{
			Job *job = executing_jobs->getPointerAt(i);

			int resources = job->getResources();
			if (resources == -1)
				printf("DUUUUUPA\n");

			// sprawdz zasoby zablokowanego zadania
			sprintf(query,
					"SELECT ip, sum(resources) AS resources FROM jobs WHERE ip IN "
					"(select ip from blocked WHERE jobid = %ld) GROUP BY ip;",
					job->getUniqueId()
				   );
			res = servicesTransaction.exec(query);

			if (res.size() == 0)
				continue;

			int computer_resources = -1;
			if (res[0]["resources"].is_null() )
				computer_resources = 0.;
			else
				computer_resources = res[0]["resources"].as<int>();

			// sprawdzić, czy jest dostatecznie duzo wolnych zasobow
			if (computer_resources + resources <= COMPUTER_MAX_RESOURCES)
			{
				// dodaj zadanie

				// wiadomosc do Crankshafta
				msq.sendMessage(
						scheduler_crankshaft_queue,
						scheduler_crankshaft_msgt,
						job->getMessageToCrankshaft()
						);
				// sprawdz ino czy Crankshaft wyslal zadanie na kompa
				string status_message =
					msq.readMessageLock(crankshaft_status_queue,
							job->getUniqueId());

				if (status_message == "0")
				{
					//cout << "message OK for NEW job "<< job->getUniqueId() << "\n";
					job->setStatus(Job::Status::SENT);
					job->startClock();

					insertIntoJobs(job);

					deleteFromBlocked(job);
				}
				else
					job->setStatus(Job::Status::NEW);

			}
		}


}


void executeNewJobs(JobList *executing_jobs)
{
	/**
	 * Jedzie po executing_jobs i szuka tych ze statusem NEW, szuka kompa i
	 * wysyła do kolejki Crankshaftu
	 */

	// jezeli jest duzo waiting jobs, to nie ma sensu nawet sie pytac i zawalac
	// baze danych
	// 1) policz ile jest WAITING jobs
	unsigned int c = 0;
	for (unsigned int i = 0; i < executing_jobs->size(); i++)
		if (executing_jobs->getPointerAt(i)->isStatus(Job::Status::WAITING))
			c++;
	if (c > MAX_WAITING_JOBS)
		return;
		// SLEEP(1) tu dać ????



	char query[1000];

	// KOLEJKA FIFO, zdejmujemy (ustawiamy status SENT) z kolejki
	for (unsigned int i = 0; i < executing_jobs->size(); i++)
		if (executing_jobs->getPointerAt(i)->isStatus(Job::Status::NEW) )
		{
			Job *job = executing_jobs->getPointerAt(i);

			int resources = job->getResources();
// 			cout << "execNewJ: " << job->getUniqueId() << " " << resources << "\n";
// 			job->setResources(resources);


			// 2) find computer with free resources
			sprintf(query,
					"SELECT ip, sum(resources) AS resources FROM "
					"(SELECT ip, sum(resources) AS resources FROM jobs WHERE "
					"ip IN "
					"(SELECT ip FROM services WHERE service = %d) "
					"GROUP BY ip "
					"UNION "
					"SELECT ip, null FROM services WHERE service = %d) AS a "
				  	"WHERE ip NOT IN (SELECT ip FROM blocked) "
					"GROUP BY ip ORDER BY resources ASC NULLS FIRST; ",
					job->getService(),
					job->getService()
				   );
			res = servicesTransaction.exec(query);


			if (res.size() == 0) // brak komputerow do zablokowania nawet
				break;


			int computer_resources = -1;
			if (res[0]["resources"].is_null() )
				computer_resources = 0.;
			else
				computer_resources = res[0]["resources"].as<int>();

			job->setComputerIp(res[0]["ip"].as<string>());



			// sprawdzić, czy jest dostatecznie duzo wolnych zasobow
			if (computer_resources + resources <= COMPUTER_MAX_RESOURCES)
			{
				// jak mamy dostatecznie duzo zasobow, to jedziemy dalej...

				// wiadomosc do Crankshafta
				msq.sendMessage(
						scheduler_crankshaft_queue,
						scheduler_crankshaft_msgt,
						job->getMessageToCrankshaft()
						);

				// sprawdz ino czy Crankshaft wyslal zadanie na kompa
 				string status_message =
					msq.readMessageLock(crankshaft_status_queue,
						  				job->getUniqueId());
				if (status_message == "0")
				{
					//cout << "message OK for NEW job "<< job->getUniqueId() << "\n";
					job->setStatus(Job::Status::SENT);
					job->startClock();

					// zajmujemy komputer w DB
					insertIntoJobs(job);

				}
				else
					job->setStatus(Job::Status::NEW);
			}
			else
			{	// znajdź komputer z najmniejszą ilością zadań (bo jedno zadanie
				// uwolni duzo zasobów -> krótkie czekanie itp...)
				// i zajmij ten komputer (dodaj do tablicy 'blocked')

				sprintf(query,
						"SELECT ip, count(ip) FROM jobs WHERE ip IN "
						"(SELECT ip from services where service = %d) "
						"AND ip NOT IN (SELECT ip FROM blocked) "
						"GROUP BY ip ORDER BY count ASC;",
						job->getService()
				);
				res = servicesTransaction.exec(query);

				//zablokuj komputer
				sprintf(query, "INSERT INTO blocked VALUES ('%s', %ld);",
					  res[0]["ip"].as<string>().c_str(), job->getUniqueId() );

				res = servicesTransaction.exec(query);
				servicesTransaction.exec("COMMIT;");

				job->setStatus(Job::Status::WAITING);
				continue;
			}
		}


}

void insertIntoJobs(Job *job)
{
	char query[1000];

	sprintf(query, "INSERT INTO jobs "
			"(jobid, ip, service, resources, mutatorid, priority, path)"
			" VALUES "
			"(%ld, '%s', %d, %d, %d, %d, '%s');",
			job->getUniqueId(),
			job->getComputerIp().c_str(),
			job->getService(),
			job->getResources(),
			job->getMutatorId(),
			job->getPriority(),
			job->getPath().c_str()
		   );
	res = servicesTransaction.exec(query);
	servicesTransaction.exec("COMMIT");
}

void deleteFromBlocked(Job *job)
{
	char query[1000];

	sprintf(query, "DELETE FROM blocked WHERE jobid = %ld;",
			job->getUniqueId());
	servicesTransaction.exec(query);
	servicesTransaction.exec("COMMIT");
}

bool isMessageStatusOk(int k, long t)
{
	string message = msq.readMessageLock(k, t);
	int status = -1;
	sscanf(message.c_str(), "%d", &status);

// 	status = 0;

	if (status != Status::OK)
	{
		cout << "error: status "<<status<< " from Crankshaft" << "\n";
		return false;
	}
	else
		return true;
}

void updateStatusFromCrankshaft(JobList *executing_jobs)
{
	/// przeczytaj kolejke od crankshafta i ustaw odpowiednie statusy zadaniom
	//  czyli
	vector<string> messages =
		msq.readQueue(crankshaft_scheduler_queue, crankshaft_scheduler_msgt);

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

		for (unsigned int i = 0; i < executing_jobs->size(); i++)
			if (executing_jobs->getPointerAt(i)->getUniqueId() == id)
				if (executing_jobs->getPointerAt(i)->isStatus(Job::Status::SENT) )
					executing_jobs->getPointerAt(i)->setStatus(status);

	}

}

void manageFinishedJobs(JobList *executing_jobs)
{
	char buf[MAXSIZE];
	char query[1000];

	for (unsigned int job_index=0;job_index < executing_jobs->size();job_index++)
	{
		Job *job = executing_jobs->getPointerAt(job_index);

		if (job->isStatus(Job::Status::FINISHED))
		{
			// send message to mutator
			sprintf(buf,
				   	"%ld %d %d %d",
					job->getUniqueId(),
					job->getMutatorId(),
					job->getTaskId(),
					job->getStatus() );
// 					Job::Status::FINISHED );

			msq.sendMessage(scheduler_mutator_queue, scheduler_mutator_msgt, buf);


			if (isMessageStatusOk(mutator_status_queue, job->getUniqueId()))
			{
				cout << "FINISHED JOB "<<job->getUniqueId()<< ": "<< buf << "\n";

				// stop timer
				job->stopClock();

				// release resources (DataBase)
				sprintf(query, "DELETE FROM jobs WHERE jobid=%ld",
						job->getUniqueId() );
				servicesTransaction.exec(query);
				servicesTransaction.exec("COMMIT");

				// remove from the list
				executing_jobs->erase(job_index);
				job_index--;
			}
		}
	}

}

void printTable(result &res)
{

	for (result::const_iterator item = res.begin(); item != res.end(); ++item)
	{ 	// i to tuple  z wynikami ("nazwa_pola", "wartość")
		for (unsigned int i = 0; i < item.size(); i++ )
			cout << item[i] << " \t ";
		cout << endl;
		// 		cout << item["status"] << " \t " << item["name"] << endl;
	}
}

void clearJobsInDatabase()
{
	servicesTransaction.exec("delete from jobs;");
	servicesTransaction.exec("delete from blocked;");
	servicesTransaction.exec("COMMIT;");
}




// void takeJobsFromDatabase(JobList &executing_jobs)
// {
// 	res = servicesTransaction.exec("SELECT * FROM jobs;");
//
// 	for (result::const_iterator item = res.begin(); item != res.end(); ++item)
// 	{ 	// item to tuple  z wynikami ("nazwa_pola", "wartość")
// 		// 		for (unsigned int i = 0; i < item.size(); i++ )
//
// 		Job job
//
// 		executing_jobs.pushBack( Job(
// 	}
//
// }
