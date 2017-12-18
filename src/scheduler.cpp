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
#include "Options.h"
// #include "Database.h"


#define MAXSIZE 128
#define MAX_WAITING_JOBS 100
#define MAX_EXECUTING_JOBS 100
#define JOB_TIMEOUT_SECONDS 120

using namespace std;
using namespace pqxx;

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
void deleteFromBlocked(long unique_id);
void deleteFromJobs(long unique_id);
pqxx::result selectFreeComputer(Job::Service s);
int blockedComputerResources(string ip);
void insertIntoBlocked(string ip, long unique_id);
string findLeastOccupiedComputerIp(Job::Service s);
void printStats(JobList &executing_jobs);
void deleteFromServices(string ip);
void manageTimeoutedJobs(JobList &executing_jobs);
void manageErrorJobs(JobList &executing_jobs);

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

map<int, vector<Job> >::reverse_iterator current_iterator;
int current_priority_counter = 0;
int current_priority = 1;


connection services("dbname=grzeslaff user=grzeslaff password=grigori8 hostaddr=150.254.66.29 ");
// connection services("dbname=grzeslaff user=grzeslaff password=grigori hostaddr=127.0.0.1");
nontransaction servicesTransaction(services, "Services transaction");

map<string, int> resources_map;


int main(int argc, char *argv[]) {
// 	WINDOW *w = initscr();
// 	noecho();
// 	cbreak();
// 	nodelay(w, TRUE);
// 	noraw();

	Options options(argc, argv);

	if (Job::ID <= 0)
	{
		cout << "Job::ID was incorectly set. It must start from 1." << "\n";
		return 1;
	}


	if (!services.is_open()) {
		cout << "not able to connect to database" << endl;
	}

	JobList executing_jobs;
	current_iterator = executing_jobs.rbegin();
	current_priority_counter = 0;

	if (options.log_messages)
	{
		msq.log_messages = true;
		msq.log_messages_filename = options.log_messages_filename;
	}

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

	msq.num_received = 0;

	clearJobsInDatabase();


	cout << "main loop..." << "\n";
	//============================== MAIN LOOP: ==============================
	while (1)
	{
// 		wrefresh(w);

		if (executing_jobs.size() > 0 )
		{
			// przeczytaj kolejke Crankshafta i ustal odpowiednie statusy
			updateStatusFromCrankshaft(executing_jobs);

			// zakończone zadania zdejmij z kolejki i zwolnij zasoby komputerów
			manageFinishedJobs(executing_jobs);
// 			manageErrorJobs(executing_jobs);
		}

		usleep(213932);			// in MICROSECONDS

		executing_jobs.pushBackFromMessages(
		   	msq.readQueue(mutator_scheduler_queue, mutator_scheduler_msgt));

		if (executing_jobs.size() > 0 )
		{
			executeWaitingJobs(executing_jobs);
			executeNewJobs(executing_jobs);
		}

		if (options.verbose)
			printStats(executing_jobs);

		executing_jobs.clearEmtyItems();

		manageTimeoutedJobs(executing_jobs);


	}

	services.disconnect();

// 	endwin();
	return 0;
}

void manageTimeoutedJobs(JobList &executing_jobs)
{
	for (auto it = executing_jobs.begin(); it != executing_jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
		if (it->second[i].isStatus(Job::Status::SENT) )
		{
			Job *job = &it->second[i];

			if ( job->getExecutonTimeSeconds() > JOB_TIMEOUT_SECONDS )
			{
				char buf[1000];
				char formatted_time[64];

				job->setStatus(Job::Status::NEW);
				deleteFromServices(job->getComputerIp());
				deleteFromJobs(job->getUniqueId());

				time_t t = time(NULL);
				struct tm *tm = localtime(&t);
				strftime(formatted_time, sizeof(formatted_time), "%c", tm);

				sprintf(buf, "Computer %s was deleted from services at %s\n",
						job->getComputerIp().c_str(), formatted_time);

				cout << buf << "\n";

				FILE *f = fopen("computers.log", "a");
				fprintf(f, "%s", buf);
				fclose(f);
			}
		}
}

void printStats(JobList &executing_jobs)
{
	int num_sent = 0, num_waiting = 0, num_error = 0;

	cout << "p:num(p)" << "\t";
	map<int, vector<Job> >::iterator it;
	for (it = executing_jobs.begin(); it != executing_jobs.end(); it++)
	{
		cout << it->first<<":"<< it->second.size() << "\t";
		// 			int num_sent = 0, num_waiting = 0;
		for (unsigned int i = 0; i < it->second.size(); i++)
		{
			if (it->second[i].isStatus(Job::Status::SENT))
				num_sent++;
			if (it->second[i].isStatus(Job::Status::WAITING))
				num_waiting++;
			if (it->second[i].isStatus(Job::Status::ERROR))
				num_error++;
		}
		// 			cout << it->first <<":"<< it->second.size() <<","<<num_sent
		// 				<<","<<num_waiting << " \t ";
	}
	// 		cout << "s,r:"<<num_sent<<" "<< num_received ;
	cout << "\n";
}

void executeWaitingJobs(JobList &executing_jobs)
{
	map<int, vector<Job> >::iterator it;

	for (it = executing_jobs.begin(); it != executing_jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
		if (it->second[i].isStatus(Job::Status::WAITING) )
		{
			// 		Job *job = &executing_jobs[i];
			Job *job = &it->second[i];

			int resources = job->getResources();

			int computer_resources =	//sprawdz zasoby zablokowanego komputera
// 				blockedComputerResources(job->getUniqueId());
				blockedComputerResources(job->getComputerIp());

			if (computer_resources == 0)
				cout << "cp=0 "<< resources_map[job->getComputerIp()]<< "\n";
			if (computer_resources != resources_map[job->getComputerIp()])
				cout << computer_resources<<"\t"<< resources_map[job->getComputerIp()]<< " ::WJ\n";

			// sprawdzić, czy jest dostatecznie duzo wolnych zasobow
			if (computer_resources + resources <= COMPUTER_MAX_RESOURCES)
// 			if (resources_map[job->getComputerIp()] + resources <= COMPUTER_MAX_RESOURCES)
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
				if (status_message[0] == '0')
				{
					job->setStatus(Job::Status::SENT);
					job->startClock();
					insertIntoJobs(job);
					resources_map[job->getComputerIp()] += job->getResources();
					deleteFromBlocked(job->getUniqueId());
				}
				else
					job->setStatus(Job::Status::NEW);
			}
		}
}


void executeNewJobs(JobList &executing_jobs)
{
	/**
	 * Jedzie po executing_jobs i szuka tych ze statusem NEW, szuka kompa i
	 * wysyła do kolejki Crankshaftu
	 */

	if (executing_jobs.size() == 0 ) // MUSI BYĆ, BO current_iterator musi byc
	{
// 		current_iterator = executing_jobs.rbegin();
		return;						 // dobrze zdefiniowany, a jak puste jest
	}
                                     // to niestety nie jest i seg fault

	// znajdz iterator do current priority
	map<int, vector<Job> >::iterator it;
	it = executing_jobs.find(current_priority);
	if (it == executing_jobs.end())
		it = executing_jobs.begin();

	// Priorytetowanie na bierząco
	map<int, int> priority_counter;
	priority_counter[current_iterator->first] = current_priority_counter;

// 	for (auto it = current_iterator; it != executing_jobs.rend(); ++it)
	for ( ; it != executing_jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
		if (it->second[i].isStatus(Job::Status::NEW) )
		{
			if (priority_counter[it->first] >= it->first
// 				&& executing_jobs.findMaxPriority() > 1)
				&& executing_jobs.size() > 1)
				break;

			Job *job = &it->second[i];

			int resources = job->estimateResources(COMPUTER_MAX_RESOURCES);

			result res = selectFreeComputer(job->getService());

			if (res.size() == 0) // brak komputerow do zablokowania nawet
			{
				current_priority = it->first;
				current_priority_counter = priority_counter[it->first];
				return;
			}

			int computer_resources = -1;
			if (res[0]["resources"].is_null() )
				computer_resources = 0;
			else
				computer_resources = res[0]["resources"].as<int>();

			job->setComputerIp(res[0]["ip"].as<string>());


			if (computer_resources + resources <= COMPUTER_MAX_RESOURCES)
// 			if (resources_map[job->getComputerIp()] + resources <= COMPUTER_MAX_RESOURCES)
			{   // sprawdzić, czy jest dostatecznie duzo wolnych zasobow
				// wiadomosc do Crankshafta
				msq.sendMessage(
						scheduler_crankshaft_queue,
						scheduler_crankshaft_msgt,
						job->getMessageToCrankshaft()
						);
				//
				// sprawdz ino czy Crankshaft wyslal zadanie na kompa
 				string status_message = msq.readMessageLock(
													crankshaft_status_queue,
													job->getUniqueId() );
				if (status_message[0] == '0')
				{
					job->setStatus(Job::Status::SENT);
					job->startClock();
					insertIntoJobs(job);	 		// zajmujemy komputer w DB
					////////
					resources_map[job->getComputerIp()] += job->getResources();
				}
				else
					job->setStatus(Job::Status::NEW);
			}
			else //if (resources == COMPUTER_MAX_RESOURCES)
			{
				insertIntoBlocked(						 // zablokuj komputer
						findLeastOccupiedComputerIp(
										job->getService()),
					   					job->getUniqueId() );
				job->setStatus(Job::Status::WAITING);
			}

			priority_counter[it->first]++;
		}
// 	current_iterator = executing_jobs.rbegin();
	current_priority = 1;
	current_priority_counter = 0;
}

void updateStatusFromCrankshaft(JobList &executing_jobs)
{ /// przeczytaj kolejke od crankshafta i ustaw odpowiednie statusy zadaniom
	vector<string> messages =
		msq.readQueue(crankshaft_scheduler_queue, crankshaft_scheduler_msgt);

	for (string s: messages)
	{
		int id = -1, crankshaft_status = -1;
		sscanf(s.c_str(), "%d %d", &id, &crankshaft_status);

		Job::Status new_status;
		if (crankshaft_status != (int)Job::CrancshaftStatus::OK)
			new_status = Job::Status::ERROR;
		else
			new_status = Job::Status::FINISHED;

		for (auto it = executing_jobs.begin(); it != executing_jobs.end(); it++)
			for (unsigned int i = 0; i < it->second.size(); i++)
				if (it->second[i].getUniqueId() == id)
					if (it->second[i].isStatus(Job::Status::SENT) )
						it->second[i].setStatus(new_status);
	}
}

void manageErrorJobs(JobList &executing_jobs)
{
	for (auto it = executing_jobs.begin(); it != executing_jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
	{
		Job *job = &it->second[i];

		if (job->isStatus(Job::Status::ERROR))
			job->setStatus(Job::Status::NEW);
	}
}

void manageFinishedJobs(JobList &executing_jobs)
{

	for (auto it = executing_jobs.begin(); it != executing_jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
	{
		Job *job = &it->second[i];

		if (job->isStatus(Job::Status::FINISHED))
		{
			// send message to mutator
			msq.sendMessage(scheduler_mutator_queue, scheduler_mutator_msgt,
					job->getMessageToMutator());

			string status_message =
				msq.readMessageLock(mutator_status_queue, job->getUniqueId());
			if (status_message[0] == '0')
			{
				// 				cout << "FINISHED JOB "<<job->getUniqueId()<< ": "<< buf << "\n";

				// stop timer
				job->stopClock();

				// release resources (DataBase)
				deleteFromJobs(job->getUniqueId());
				resources_map[job->getComputerIp()] -= job->getResources();
				// 				servicesTransaction.exec("COMMIT");

				// remove from the list
				executing_jobs.erase(job->getPriority(), i);
				i--;
			}
		}
	}
}

// int blockedComputerResources(long unique_id)
int blockedComputerResources(string ip)
{
// 	transaction<isolation_level::serializable> servicesTransaction(services);
	char query[1000];
	sprintf(query,
// 			"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE; "
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; "
			"SELECT ip, sum(resources) AS resources FROM jobs "
		    "WHERE ip = '%s' GROUP BY ip",
// 			"IN (select ip from blocked WHERE jobid = %ld) GROUP BY ip;",
// 			"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE; "
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; ",
// 			unique_id
			ip.c_str()
		   );
	result res = servicesTransaction.exec(query);

// 	servicesTransaction.commit();

	int computer_resources = -1;

	if (res.size() != 0)
	{
		if (res[0]["resources"].is_null() )
			computer_resources = 0;
		else
			computer_resources = res[0]["resources"].as<int>();
	}
	else
		computer_resources = COMPUTER_MAX_RESOURCES;

	return computer_resources;
}



pqxx::result selectFreeComputer(Job::Service s)
{
// 	transaction<isolation_level::serializable> servicesTransaction(services);
// 	cout << "selectFreeComputer" << "\n";
	char query[1000];
	sprintf(query,
// 			"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE; "
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; "
			"SELECT ip, sum(resources) AS resources FROM "
			"(SELECT ip, sum(resources) AS resources FROM jobs WHERE "
			"ip IN "
			"(SELECT ip FROM services WHERE service = %d) "
			"GROUP BY ip "
			"UNION "
			"SELECT ip, null FROM services WHERE service = %d) AS a "
			"WHERE ip NOT IN (SELECT ip FROM blocked) "
			"GROUP BY ip ORDER BY resources ASC NULLS FIRST; ",
// 			"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE; "
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; ",
			(int) s,
			(int) s
		   );
	result res = servicesTransaction.exec(query);
// 	servicesTransaction.commit();

	return  res;
}

string findLeastOccupiedComputerIp(Job::Service s)
{ // znajdź komputer z najmniejszą ilością zadań (bo jedno zadanie
  // uwolni duzo zasobów -> krótkie czekanie itp...)
  // i zajmij ten komputer (dodaj do tablicy 'blocked')
// 	cout << "FindLeastOccupied..." << "\n";
	char query[1000];
// 	transaction<isolation_level::serializable> servicesTransaction(services);

	sprintf(query,
// 			"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE;"
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE;"
		    "SELECT ip, count(ip) FROM jobs WHERE ip IN "
			"(SELECT ip from services where service = %d) "
			"AND ip NOT IN (SELECT ip FROM blocked) "
			"GROUP BY ip ORDER BY count ASC;",
// 			"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE;"
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE;",
			(int) s
		   );
	result res = servicesTransaction.exec(query);
// 	servicesTransaction.commit();

	return res[0]["ip"].as<string>();
}

void insertIntoJobs(Job *job)
{
// 	cout << "InsertIntoJobs" << "\n";
	char query[1000];
// 	transaction<isolation_level::serializable> servicesTransaction(services);

	sprintf(query,
		   	"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE;"
		    "INSERT INTO jobs "
			"(jobid, ip, service, resources, mutatorid, priority, path)"
			" VALUES "
			"(%ld, '%s', %d, %d, %d, %d, '%s');LOCK TABLE jobs;"
		   	"LOCK TABLE jobs IN ACCESS EXCLUSIVE MODE;",
			job->getUniqueId(),
			job->getComputerIp().c_str(),
			(int)job->getService(),
			job->getResources(),
			job->getMutatorId(),
			job->getPriority(),
			job->getPath().c_str()
		   );
	result res = servicesTransaction.exec(query);
// 	servicesTransaction.commit();
}

void insertIntoBlocked(string ip, long unique_id)
{
// 	cout << "insertIntoBlocked" << "\n";
	char query[1000];
// 	transaction<isolation_level::serializable> servicesTransaction(services);

	sprintf(query,
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; "
			"INSERT INTO blocked VALUES ('%s', %ld);"
			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; ",
			ip.c_str(), unique_id );

	servicesTransaction.exec(query);
// 	servicesTransaction.commit();
	// 				servicesTransaction.exec("COMMIT;");
}

void deleteFromJobs(long unique_id)
{
	char query[1000];
// 	transaction<isolation_level::serializable> servicesTransaction(services);
	sprintf(query,
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; "
		    "DELETE FROM jobs WHERE jobid=%ld; ",
// 			"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; ",
			unique_id);
	servicesTransaction.exec(query);
// 	servicesTransaction.commit();
}


void deleteFromBlocked(long unique_id)
{
// 	cout << "deleteFromBlocked" << "\n";
	char query[1000];
// 	transaction<isolation_level::serializable> servicesTransaction(services);

	sprintf(query,
// 		   	"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; "
		    "DELETE FROM blocked WHERE jobid = %ld;",
// 		   	"LOCK TABLE blocked IN ACCESS EXCLUSIVE MODE; ",
			unique_id);
	servicesTransaction.exec(query);
// 	servicesTransaction.commit();
}

void deleteFromServices(string ip)
{
	char query[1000];
// 	transaction<isolation_level::serializable> servicesTransaction(services);
	sprintf(query,
// 		   	"LOCK TABLE services IN ACCESS EXCLUSIVE MODE; "
			"DELETE FROM services WHERE ip = %s",
// 		   	"LOCK TABLE services IN ACCESS EXCLUSIVE MODE; ",
		   	ip.c_str());
	servicesTransaction.exec(query);
// 	servicesTransaction.commit();
}

void clearJobsInDatabase()
{
// 	transaction<isolation_level::serializable> servicesTransaction(services);
	servicesTransaction.exec("delete from jobs;");
	servicesTransaction.exec("delete from blocked;");
// 	servicesTransaction.commit();
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

