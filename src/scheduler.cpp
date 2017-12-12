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
// #include <ncurses.h>

#include "MessageQueue.h"
#include "Job.h"
#include "JobList.h"
// #include "Database.h"

#define MAXSIZE 128
#define MAX_WAITING_JOBS 100
#define MAX_EXECUTING_JOBS 100

using namespace std;
using namespace pqxx;

// enum Status {
// 	OK,
// 	ERROR
// };

int COMPUTER_MAX_RESOURCES = 1900;

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
nontransaction servicesTransaction(services, "Services transaction");
// result res;

void moveFromPendingToExecuting(JobList &pending_jobs, JobList &executing_jobs);
void executeNewJobs(JobList &executing_jobs);
void executeWaitingJobs(JobList &executing_jobs);
void updateStatusFromCrankshaft(JobList &executing_jobs);
void manageFinishedJobs(JobList &executing_jobs);
void printTable(result &res);
// void takeJobsFromDatabase(JobList &executing_jobs);
void clearJobsInDatabase();
bool isMessageStatusOk(int k, long t);

void insertIntoJobs(Job *job);
void deleteFromBlocked(Job *job);
pqxx::result selectFreeComputer(Job::Service s);
int blockedComputerResources(long unique_id);
void insertIntoBlocked(string ip, long unique_id);
string findLeastOccupiedComputerIp(Job::Service s);


int main() {
// 	WINDOW *w = initscr();
// 	noecho();
// 	cbreak();
// 	nodelay(w, TRUE);
// 	noraw();

	if (Job::ID <= 0)
	{
		cout << "Job::ID was incorectly set. It must start from 1." << "\n";
		return 1;
	}


	if (!services.is_open()) {
		cout << "not able to connect to database" << endl;
	}

// 	JobList *pending_jobs = new JobList;
// 	JobList *executing_jobs = new JobList;
	JobList pending_jobs, executing_jobs;

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
// 	while(wgetch(w) != 'q')
	while (1)
	{
// 		wrefresh(w);

		// zdecydować co robić, jak wróci błąd wykonania zadania

		// przeczytaj kolejke Crankshafta i ustal odpowiednie statusy
		if (executing_jobs.size() > 0 )
		{
			updateStatusFromCrankshaft(executing_jobs);

			// zakończone zadania zdejmij z kolejki i zwolnij zasoby komputerów
			manageFinishedJobs(executing_jobs);
		}

		// czytaj wszystkie dostępne komunikaty od mutatora w kolejce
		vector<string> messages =
		   	msq.readQueue(mutator_scheduler_queue, mutator_scheduler_msgt);

		// wyślij je na koniec kolejki zadań
		pending_jobs.pushBackFromMessages(messages);

		// priorytetowanie
		if (executing_jobs.size() <= MAX_EXECUTING_JOBS)
			moveFromPendingToExecuting(pending_jobs, executing_jobs);

		executeWaitingJobs(executing_jobs);
		executeNewJobs(executing_jobs);

// 		sleep(1);
		usleep(513932);			// in MICROSECONDS
	}


// 	servicesTransaction.commit();
	services.disconnect();

// 	endwin();

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

	while (maximum > 0)
	{
		if (priority_counter[maximum] < maximum)
		{
			// find index of a job in pending_jobs with priority==maximum
			int max_index = pending_jobs.findIndexWithPriority(maximum);

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
}

void executeWaitingJobs(JobList &executing_jobs)
{
	for (unsigned int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].isStatus(Job::Status::WAITING) )
		{
			Job *job = &executing_jobs[i];

			int resources = job->getResources();

			int computer_resources = // sprawdz zasoby zablokowanego komputera
				blockedComputerResources(job->getUniqueId());

			// sprawdzić, czy jest dostatecznie duzo wolnych zasobow
			if (computer_resources + resources <= COMPUTER_MAX_RESOURCES)
			{
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

int blockedComputerResources(long unique_id)
{
// 	cout << "blockedComputerResources" << "\n";
	char query[1000];

	sprintf(query,
			"SELECT ip, sum(resources) AS resources FROM jobs WHERE ip IN "
			"(select ip from blocked WHERE jobid = %ld) GROUP BY ip;",
			unique_id
		   );
	result res = servicesTransaction.exec(query);

	int computer_resources = -1;

	if (res.size() != 0)
	{
		if (res[0]["resources"].is_null() )
			computer_resources = 0;
		else
			computer_resources = res[0]["resources"].as<int>();
	}
	else
		computer_resources = 0;

	return computer_resources;
}

void executeNewJobs(JobList &executing_jobs)
{
	/**
	 * Jedzie po executing_jobs i szuka tych ze statusem NEW, szuka kompa i
	 * wysyła do kolejki Crankshaftu
	 */

	// jak jest za duzo WAITING JOBS to nie ma sensu jechać dalej...
	unsigned int c = 0;
	for (unsigned int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].isStatus(Job::Status::WAITING))
			c++;
	if (c > MAX_WAITING_JOBS)
		return;

	for (unsigned int i = 0; i < executing_jobs.size(); i++)
		if (executing_jobs[i].isStatus(Job::Status::NEW) )
		{
			Job *job = &executing_jobs[i];

			int resources = job->estimateResources(COMPUTER_MAX_RESOURCES);

			result res = selectFreeComputer(job->getService());

			if (res.size() == 0) // brak komputerow do zablokowania nawet
				break;

			int computer_resources = -1;
			if (res[0]["resources"].is_null() )
				computer_resources = 0.;
			else
				computer_resources = res[0]["resources"].as<int>();

			job->setComputerIp(res[0]["ip"].as<string>());

			if (computer_resources + resources <= COMPUTER_MAX_RESOURCES)
			{   // sprawdzić, czy jest dostatecznie duzo wolnych zasobow

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
					job->setStatus(Job::Status::SENT);
					job->startClock();
					insertIntoJobs(job);	 		// zajmujemy komputer w DB
				}
				else
					job->setStatus(Job::Status::NEW);
			}
			else
			{
				insertIntoBlocked(						 // zablokuj komputer
						findLeastOccupiedComputerIp(job->getService()),
					   	job->getUniqueId()	);
				job->setStatus(Job::Status::WAITING);
			}
		}
}

pqxx::result selectFreeComputer(Job::Service s)
{
// 	cout << "selectFreeComputer" << "\n";
	char query[1000];
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
			(int) s,
			(int) s
		   );
	return  servicesTransaction.exec(query);
}

string findLeastOccupiedComputerIp(Job::Service s)
{ // znajdź komputer z najmniejszą ilością zadań (bo jedno zadanie
  // uwolni duzo zasobów -> krótkie czekanie itp...)
  // i zajmij ten komputer (dodaj do tablicy 'blocked')
// 	cout << "FindLeastOccupied..." << "\n";
	char query[1000];

	sprintf(query,
			"SELECT ip, count(ip) FROM jobs WHERE ip IN "
			"(SELECT ip from services where service = %d) "
			"AND ip NOT IN (SELECT ip FROM blocked) "
			"GROUP BY ip ORDER BY count ASC;",
			(int) s
		   );
	result res = servicesTransaction.exec(query);

	return res[0]["ip"].as<string>();
}

void insertIntoJobs(Job *job)
{
// 	cout << "InsertIntoJobs" << "\n";
	char query[1000];

	sprintf(query, "INSERT INTO jobs "
			"(jobid, ip, service, resources, mutatorid, priority, path)"
			" VALUES "
			"(%ld, '%s', %d, %d, %d, %d, '%s');",
			job->getUniqueId(),
			job->getComputerIp().c_str(),
			(int)job->getService(),
			job->getResources(),
			job->getMutatorId(),
			job->getPriority(),
			job->getPath().c_str()
		   );
	result res = servicesTransaction.exec(query);
}

void insertIntoBlocked(string ip, long unique_id)
{
// 	cout << "insertIntoBlocked" << "\n";
	char query[1000];

	sprintf(query, "INSERT INTO blocked VALUES ('%s', %ld);",
			ip.c_str(), unique_id );

	servicesTransaction.exec(query);
	// 				servicesTransaction.exec("COMMIT;");
}


void deleteFromBlocked(Job *job)
{
// 	cout << "deleteFromBlocked" << "\n";
	char query[1000];

	sprintf(query, "DELETE FROM blocked WHERE jobid = %ld;",
			job->getUniqueId());
	servicesTransaction.exec(query);
}


void updateStatusFromCrankshaft(JobList &executing_jobs)
{
	/// przeczytaj kolejke od crankshafta i ustaw odpowiednie statusy zadaniom
	//  czyli
	vector<string> messages =
		msq.readQueue(crankshaft_scheduler_queue, crankshaft_scheduler_msgt);

	if ( messages.empty() )
		return;


	for (string s: messages)
	{
		int id = -1, crankshaft_status = -1;
		sscanf(s.c_str(), "%d %d", &id, &crankshaft_status);


		Job::Status new_status;
		if (crankshaft_status != (int)Job::CrancshaftStatus::OK)
			new_status = Job::Status::ERROR;
		else
			new_status = Job::Status::FINISHED;

		for (unsigned int i = 0; i < executing_jobs.size(); i++)
			if (executing_jobs[i].getUniqueId() == id)
				if (executing_jobs[i].isStatus(Job::Status::SENT) )
					executing_jobs[i].setStatus(new_status);

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
				   	"%ld %d %d %d",
					job->getUniqueId(),
					job->getMutatorId(),
					job->getTaskId(),
					(int)job->getStatus() );
// 					Job::Status::FINISHED );

			msq.sendMessage(scheduler_mutator_queue, scheduler_mutator_msgt, buf);


			string status_message =
				msq.readMessageLock(mutator_status_queue,
						job->getUniqueId());
				if (status_message == "0")
			{
				cout << "FINISHED JOB "<<job->getUniqueId()<< ": "<< buf << "\n";

				// stop timer
				job->stopClock();

				// release resources (DataBase)
				sprintf(query, "DELETE FROM jobs WHERE jobid=%ld",
						job->getUniqueId() );
				servicesTransaction.exec(query);
// 				servicesTransaction.exec("COMMIT");

				// remove from the list
				executing_jobs.erase(job_index);
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
// 	servicesTransaction.exec("COMMIT;");
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
