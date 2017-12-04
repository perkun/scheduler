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

using namespace std;
using namespace pqxx;

enum Status {
	OK,
	ERROR
};

int COMPUTER_MAX_RESOURCES = 1600;

int mutator_queue_id = 1234,
   	crankshaft_queue_id = 1234,
   	scheduler_queue_id = 1234,
	manager_queue_id = 1234,
	corba_status_queue_id = 6000,
	crankshaft_status_queue_id =  7000;


long mutator_scheduler_msgt = 1,
	 crankshaft_scheduler_msgt = 2,
	 scheduler_crankshaft_msgt = 3,
	 scheduler_manager_msgt = 4,
	 scheduler_mutator_msgt = 5;

MessageQueue msq;

long Job::ID = 1;


// connection services("dbname=grzeslaff user=grzeslaff password=grigori8 hostaddr=150.254.66.29 ");
connection services("dbname=grzeslaff user=grzeslaff password=grigori hostaddr=127.0.0.1");
transaction<> servicesTransaction(services, "Services transaction");
result res;

void moveFromPendingToExecuting(JobList &pending_jobs, JobList &executing_jobs);
void executeNewJobs(JobList &executing_jobs);
void executeWaitingJobs(JobList &executing_jobs);
void updateStatusFromCrankshaft(JobList &executing_jobs);
void manageFinishedJobs(JobList &executing_jobs);
void printTable(result &res);
// void takeJobsFromDatabase(JobList &executing_jobs);
void clearJobsInDatabase();
bool isMessageStatusOk(int k, long t);


int main() {

	if (Job::ID <= 0)
	{
		cout << "Job::ID was incorectly set. It must start from 1." << "\n";
		return 1;
	}

	JobList pending_jobs;
	JobList executing_jobs;

	if (!services.is_open()) {
		cout << "not able to connect to database" << endl;
	}

	// wyczysc kolejke
	msq.recreate(mutator_queue_id);
	msq.recreate(crankshaft_status_queue_id);

	// wysłać info, że sie uruchomiło
	msq.sendMessage(manager_queue_id, scheduler_manager_msgt,
			"scheduler started");
	// wczytać cokolwiek zalega w bazie jobs (czyli ze statusem SENT)
	clearJobsInDatabase();


	//============================== MAIN LOOP: ==============================
	while (1)
	{

		// zdecydować co robić, jak wróci błąd wykonania zadania

		// przeczytaj kolejke Crankshafta i ustal odpowiednie statusy
		if (executing_jobs.size() > 0 )
		{
			updateStatusFromCrankshaft(executing_jobs);

			// zakończone zadania zdejmij z kolejki i zwolnij zasoby komputerów
			manageFinishedJobs(executing_jobs);

			// manageErrorJobs
		}



		// czytaj wszystkie dostępne komunikaty czekające w kolejce
		vector<string> messages = msq.readQueue(mutator_queue_id,
			   									mutator_scheduler_msgt);
// 		if (messages.empty())
// 			printf("msqueue empty\n");

		// wyślij je na koniec kolejki zadań
		pending_jobs.pushBackFromMessages(messages);

		// priorytetowanie
		moveFromPendingToExecuting(pending_jobs, executing_jobs);

		// w executing_jobs leżą teraz joby, które trzeba skwencyjnie od 0,1,...
		// wysłać do wykonania na komputerze (FIFO)
		executeWaitingJobs(executing_jobs);
		executeNewJobs(executing_jobs);



		printf("%u\t%u\tPendint, Executing\n",
			   	pending_jobs.size(),
				executing_jobs.size());
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

void executeWaitingJobs(JobList &executing_jobs)
{
	char query[1000];

	for (unsigned int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].isStatus(Job::Status::WAITING) )
		{
			Job *job = &executing_jobs[i];

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
			if (computer_resources + resources > COMPUTER_MAX_RESOURCES)
				continue;
			else
			{
				// dodaj zadanie
				job->setComputerIp(res[0]["ip"].as<string>());

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

				servicesTransaction.exec(query);

				sprintf(query, "DELETE FROM blocked WHERE jobid = %ld;",
						job->getUniqueId());
				servicesTransaction.exec(query);
				servicesTransaction.exec("COMMIT");

				job->startClock();

				msq.sendMessage(
						crankshaft_queue_id,
						scheduler_crankshaft_msgt,
						job->getMessageToCrankshaft()
						);

				job->setStatus(Job::Status::SENT);
			}

		}


}

void executeNewJobs(JobList &executing_jobs)
{
	/**
	 * Jedzie po executing_jobs i szuka tych ze statusem NEW, szuka kompa i
	 * wysyła do kolejki Crankshaftu
	 */

	char query[1000];

	// KOLEJKA FIFO, zdejmujemy (ustawiamy status SENT) z kolejki
	for (unsigned int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].isStatus(Job::Status::NEW) )
		{
			Job *job = &executing_jobs[i];

			// 1) oszacuj zasoby (TODO)
			// 			executing_jobs[i].estimateResources();
			// 			odniesc sie do bazy danych ze statystykami itp
			int resources = job->getResources();
			cout << "execNewJ: " << job->getUniqueId() << " " << resources << "\n";
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

			if (res.size() == 0)
				break;


			int computer_resources = -1;
			if (res[0]["resources"].is_null() )
				computer_resources = 0.;
			else
				computer_resources = res[0]["resources"].as<int>();

			job->setComputerIp(res[0]["ip"].as<string>());

			// wiadomosc do Crankshafta
			msq.sendMessage(
					crankshaft_queue_id,
					scheduler_crankshaft_msgt,
					job->getMessageToCrankshaft()
			);

			// sprawdz ino czy Crankshaft wyslal zadanie na kompa
			if (isMessageStatusOk(
						crankshaft_status_queue_id, job->getUniqueId()))
			{

				job->setStatus(Job::Status::SENT);
				job->startClock();
			}
			else
			{
				job->setStatus(Job::Status::NEW);
				return;
			}


			// sprawdzić, czy jest dostatecznie duzo wolnych zasobow
			if (computer_resources + resources <= COMPUTER_MAX_RESOURCES)
			{
				// jak mamy dostatecznie duzo zasobow, to jedziemy dalej...
				// czyli pozyskujemy ip z DB
// 				job->setComputerIp(res[0]["ip"].as<string>());


				// zajmujemy komputer w DB
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
			else
			{
				// znajdź komputer z najmniejszą ilością zadań (bo jedno zadanie
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
// 				break;
				continue;
			}


		}


}

bool isMessageStatusOk(int k, long t)
{
	string message = msq.readMessageLock(k, t);
	int status = -1;
	sscanf(message.c_str(), "%d", &status);

	if (status != Status::OK)
	{
		cout << "error: "<<status<< " from Crankshaft for job " << "\n";
		return false;
	}
	else
		return true;

}

void updateStatusFromCrankshaft(JobList &executing_jobs)
{
	/// przeczytaj kolejke od crankshafta i ustaw odpowiednie statusy zadaniom
	vector<string> messages =
		msq.readQueue(scheduler_queue_id, crankshaft_scheduler_msgt);

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
			if (executing_jobs[i].getUniqueId() == id)
				if (executing_jobs[i].isStatus(Job::Status::SENT) )
					executing_jobs[i].setStatus(status);

	}

}

void manageFinishedJobs(JobList &executing_jobs)
{
	char buf[MAXSIZE];
	char query[1000];

	for (unsigned int job_index=0;job_index < executing_jobs.size();job_index++)
	{
		Job *job = &executing_jobs[job_index];

		if (job->isStatus(Job::Status::FINISHED))
		{
			// send message to mutator
			sprintf(buf,
				   	"%ld %d",
					job->getUniqueId(),
					job->getStatus() );
// 					Job::Status::FINISHED );
			msq.sendMessage(mutator_queue_id, scheduler_mutator_msgt, buf);



			// stop timer
			job->stopClock();

			// release resources (DataBase)
			sprintf(query, "DELETE FROM jobs WHERE jobid=%ld",
					job->getUniqueId() );
			servicesTransaction.exec(query);
			servicesTransaction.exec("COMMIT");

			// remove from the list
			executing_jobs.erase(job_index);
			job_index--;
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
