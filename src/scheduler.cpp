/**
\mainpage Scheduler
@author Grzegorz Dudziński<br> g.dudzinski@amu.edu.pl
@date december 2017
# Scheduler for OA Cluster

## Program arguments:

	--help | -h                    print this help
	--logm <filename>              output all received and sent messages to file
	--verbose | -v                 print stuff on da screen
	--noclear                      do NOT clear message queues

## Description
This program reads IPC messages queues from Mutators (programs handling
computational tasks), then schedules the tasks to computers and send message to
Crankshaft program, which distributes tasks to Work Stations using CORBA.

## Main program documentation:
##<a href="scheduler_8cpp.html">scheduler.cpp</a>

*/

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
#include <algorithm>

#include "MessageQueue.h"
#include "Job.h"
#include "Options.h"
#include "Computer.h"

#define MAXSIZE 128
#define MAX_WAITING_JOBS 100
#define MAX_EXECUTING_JOBS 100
#define JOB_TIMEOUT_SECONDS 120
#define MAX_COMPUTER_RESOURCES  1900

#define mutator_scheduler_queue 1000
#define scheduler_mutator_queue 1001
#define scheduler_crankshaft_queue 2001
#define crankshaft_scheduler_queue 2000
#define scheduler_manager_queue 5000
#define mutator_status_queue 6000
#define crankshaft_status_queue  7000

#define mutator_scheduler_msgt 1
#define crankshaft_scheduler_msgt 1
#define scheduler_crankshaft_msgt 1
#define scheduler_manager_msgt 1
#define scheduler_mutator_msgt 1

using namespace std;
using namespace pqxx;

void executeNewJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers);
void executeWaitingJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers);
void manageFinishedJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers);
void manageTimeoutedJobs(map<int, vector<Job> > &jobs);
void manageErrorJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers);

void updateStatusFromCrankshaft(map<int, vector<Job> > &jobs);

void deleteFromServices(string ip);
void importServices(vector<Computer> &computers);

void pushBackJobsFromMessages(map<int, vector<Job> > &jobs, vector<string> messages);
void clearEmtyItems(map<int, vector<Job> > &jobs);

Computer* findFreeComputer(vector<Computer> &computers, int service, int resources);
Computer* findLeastOccupiedComputer(vector<Computer> &computers, int job_service);
Computer* findBlockedComputer(vector<Computer> &computers, long unique_id, int job_resources);
void releaseComputer(vector<Computer> &computers, Job *job);

void printStats(map<int, vector<Job> > &jobs, vector<Computer> computers);
inline const char * const boolToString(bool b);


MessageQueue msq;
long Job::ID = 1;
int current_priority_counter = 0;
int current_priority = 1;
connection services("dbname=grzeslaff user=grzeslaff password=grigori8 hostaddr=150.254.66.29 ");
// connection services("dbname=grzeslaff user=grzeslaff password=grigori hostaddr=127.0.0.1");
nontransaction servicesTransaction(services, "Services transaction");


/**
 * ## MAIN FUNCTION
 * ### Initialization:
 *
 * * connects to database `grzeslaff` with (computer_ip, service) table
 * * sets `current_priority = 1`, `current_priority_counter = 0`
 * * clears queues
 * * sends message to MANAGER that it started
 * * enters Main Loop
 */
int main(int argc, char *argv[]) {

	Options options(argc, argv);

	if (Job::ID <= 0)
	{
		cout << "Job::ID was incorrectly set. It must start from 1." << "\n";
		return 1;
	}


	if (!services.is_open())
		cout << "not able to connect to database" << endl;

	/** list of available computers. Updated at the beginning of every iteration*/
	vector<Computer> computers;

	map<int, vector<Job> > jobs;
	current_priority_counter = 0;
	current_priority = 1;

	if (options.log_messages)
	{
		msq.log_messages = true;
		msq.log_messages_filename = options.log_messages_filename;
	}

	// wyczysc kolejke
	if (options.clear_queues)
	{
		msq.cleanQueue(mutator_scheduler_queue);
		msq.cleanQueue(scheduler_mutator_queue);
		msq.cleanQueue(crankshaft_scheduler_queue);
		msq.cleanQueue(crankshaft_status_queue);
		msq.cleanQueue(scheduler_crankshaft_queue);
		msq.cleanQueue(mutator_status_queue);
		msq.cleanQueue(scheduler_manager_queue);
	}

	// wysłać info, że sie uruchomiło
	msq.sendMessage(scheduler_manager_queue, scheduler_manager_msgt,
														"scheduler started");

	msq.num_received = 0;


	cout << "main loop..." << "\n";
	//============================== MAIN LOOP: ==============================
	while (1)
	{
		/**
		 * ### Main Loop:
		 * -# imports services from database. If no computers are available,
		 *  sleeps for 1 sec
		 * -# updates status of sent jobs from Crankshaft
		 * -# manages Finished jobs
		 * -# manages jobs with errors
		 * -# reads queue from mutators and ads new jobs
		 * -# executes waiting jobs
		 * -# executes new jobs
		 * -# manages time-outed jobs
		 * -# if in verbose mode, prints stats about computers
		 * -# sleeps for 0.2 sec
		 */

		importServices(computers);
		if (computers.size() == 0)
		{
			cout << "no computers available\n";
			sleep(1);
			continue;
		}


		if (jobs.size() > 0 )
		{
			// przeczytaj kolejke Crankshafta i ustal odpowiednie statusy
			updateStatusFromCrankshaft(jobs);

			// zakończone zadania zdejmij z kolejki i zwolnij zasoby komputerów
			manageFinishedJobs(jobs, computers);
			manageErrorJobs(jobs, computers);
		}

		pushBackJobsFromMessages(jobs,
				msq.readQueue(mutator_scheduler_queue, mutator_scheduler_msgt));



		if (jobs.size() > 0 )
		{
			executeWaitingJobs(jobs, computers);
			executeNewJobs(jobs, computers);
		}

		if (options.verbose)
			printStats(jobs, computers);

		clearEmtyItems(jobs);

		manageTimeoutedJobs(jobs);


		usleep(213932);			// in MICROSECONDS
	}

	services.disconnect();


	return 0;
}


/**
 * @brief Prints info on computers' load and number of tasks per priority.
 *
 * @param[in] jobs needed for stats
 * @param[in] computers needed for stats
 */
void printStats(map<int, vector<Job> > &jobs, vector<Computer> computers)
{


	int num_sent = 0, num_waiting = 0, num_error = 0;

	cout << "p:num(p)" << "\t";
	map<int, vector<Job> >::iterator it;
	for (it = jobs.begin(); it != jobs.end(); it++)
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

	sort(computers.begin(), computers.end(),
			[](const Computer& lhs, const Computer& rhs)
			{ return lhs.ip < rhs.ip; });

	for (auto c: computers)
	{
		cout << c.ip<<"\t" << c.resources<<"\t"<<c.num_jobs << "\t";
		if (c.blocked)
			cout << "blocked: "<< c.blocking_job_id;
		cout << "\n";
	}
}


/** @brief Looks for jobs executing longer than JOB_TIMEOUT_SECONDS = 120
 *
 * For such a job a **NEW** status is set (meaning it will be scheduled again)
 * and computer that was executing this job is removed from services table in
 * DB. A message is printed on screen (regardles of --verbose option) and also
 * to the file __computers.log__.
 */
void manageTimeoutedJobs(map<int, vector<Job> > &jobs)
{
	for (auto it = jobs.begin(); it != jobs.end(); it++)
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

/** @brief Executes jobs with  Job::Status::WAITING.
 *
 *	Job with **WAITING** Status has a computer blocked for it. Computer object
 *	has two members for that (`bool blocked` and `int blocking_job_id`). This
 *	function checks if computer reserved for this job has enough resources and
 *	if yes, send the job to Crankshaft and unblocks the computer.
 */
void executeWaitingJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers)
{
	map<int, vector<Job> >::iterator it;

	for (it = jobs.begin(); it != jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
		if (it->second[i].isStatus(Job::Status::WAITING) )
		{
			// 		Job *job = &jobs[i];
			Job *job = &it->second[i];

			Computer *computer =
			   	findBlockedComputer(computers, job->getUniqueId(), job->getResources());

			if (computer != NULL)
			{
				job->setComputerIp(computer->ip);
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

					computer->unblock();
					computer->addJob(job->getResources());
				}
				else
					job->setStatus(Job::Status::NEW);
			}
		}
}


/** @brief Executes jobs with  Job::Status::NEW.
 *
 *  Prioritizing of the jobs is performed here ``'online'``; e.g. jobs with
 *  priority 10 will be executed 10 times more often than ones with priority 1.
 *  If no computer is available (`NULL` pointer returned by findFreeComputer)
 *  function returns. If a job is of Job::Type::ESTIMATE_RESOURCES a computer is
 *  blocked (this job needs an empty computer to estimate resources on GPU).
 *
 *  If there is a free computer, a job is sent to it.
 */
void executeNewJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers)
{

	if (jobs.size() == 0 ) // MUSI BYĆ, BO current_iterator musi byc
	{
// 		current_iterator = jobs.rbegin();
		return;						 // dobrze zdefiniowany, a jak puste jest
	}
                                     // to niestety nie jest i seg fault

	// znajdz iterator do current priority
	map<int, vector<Job> >::iterator it;
	it = jobs.find(current_priority);
	if (it == jobs.end())
		it = jobs.begin();

	// Priorytetowanie na bierząco
	map<int, int> priority_counter;
	priority_counter[it->first] = current_priority_counter;

// 	for (auto it = current_iterator; it != jobs.rend(); ++it)
	for ( ; it != jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
		if (it->second[i].isStatus(Job::Status::NEW) )
		{
			if (priority_counter[it->first] >= it->first &&
					jobs.size() > 1)
				break;

			Job *job = &it->second[i];

			job->estimateResources(MAX_COMPUTER_RESOURCES);

			Computer *computer =
				findFreeComputer(computers, (int)job->getService(), job->getResources());

			if (computer == NULL) // wszystkie zablokowane lub wszystkie pełne
			{
				current_priority = it->first;
// 				current_priority = job->getPriority();
				current_priority_counter = priority_counter[it->first];

				if (job->isType(Job::Type::ESTIMATE_RESOURCES) )
				{
					Computer *blocked_computer =
					   	findLeastOccupiedComputer(computers, (int)job->getService());
					if (blocked_computer != NULL)
					{
						blocked_computer->block(job->getUniqueId());
						job->setStatus(Job::Status::WAITING);
					}
				}
				return; // current priority and p. counter will not be reseted
						// at the end of loops
			}
			else
			{
				job->setComputerIp(computer->ip);

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
					computer->addJob(job->getResources());
				}
				else
					job->setStatus(Job::Status::NEW);

				priority_counter[it->first]++;
			}

			long id = 0;
			for (auto c: computers)
				if (c.blocked)
				{
					id = c.blocking_job_id;
				}


		}

	current_priority = 1;
	current_priority_counter = 0;
}

/** @brief Reads message queue from Crankshaft (finished jobs)
 *
 * ...and updates the Job::Status accordingly.
 */
void updateStatusFromCrankshaft(map<int, vector<Job> > &jobs)
{
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

		for (auto it = jobs.begin(); it != jobs.end(); it++)
			for (unsigned int i = 0; i < it->second.size(); i++)
				if (it->second[i].getUniqueId() == id)
					if (it->second[i].isStatus(Job::Status::SENT) )
						it->second[i].setStatus(new_status);
	}
}

/** @brief If a job has status Job::Status::ERROR, the status is set to
 * Job::Status::NEW and the job is scheduled again.
 *
 */
void manageErrorJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers)
{
	for (auto it = jobs.begin(); it != jobs.end(); it++)
	for (unsigned int i = 0; i < it->second.size(); i++)
	{
		Job *job = &it->second[i];

		if (job->isStatus(Job::Status::ERROR))
		{
			job->setStatus(Job::Status::NEW);
			releaseComputer(computers, job);
		}
	}
}

/** @brief Manages jobs with status Job::Status::FINISHED (correctly calculated
 * jobs).
 *
 * The computer is freed, job's clock is stopped and the job is removed from
 * jobs list. A message to Mutator is sent.
 */
void manageFinishedJobs(map<int, vector<Job> > &jobs, vector<Computer> &computers)
{

	for (auto it = jobs.begin(); it != jobs.end(); it++)
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
				job->stopClock();

				releaseComputer(computers, job);
				// remove from the list
// 				jobs.erase(job->getPriority(), i);
				jobs[job->getPriority()].erase(
						jobs[job->getPriority()].begin() + i);
				i--;
			}
		}
	}
}

/** @brief Deletes computer form DataBase WHERE computer_ip == ip.
 *
 * New jobs will not be scheduled for this computer any more. Computer needs to
 * be added manually to the DB after making sure it is operational again.
 */
void deleteFromServices(string ip)
{
	char query[1000];
	sprintf(query,
			"DELETE FROM services WHERE ip = %s",
		   	ip.c_str());
	servicesTransaction.exec(query);
}

/** @brief Imports and updates computers based on DataBase
 *
 * It is performed in every iteration so computers list is always up to date.
 */
void importServices(vector<Computer> &computers)
{
	char query[1000];
	sprintf(query, "SELECT * FROM services;");
	result res = servicesTransaction.exec(query);

	for (auto &c: computers)
		c.services.clear();

	for (result::const_iterator item = res.begin(); item != res.end(); item++)
	{
		bool exists = false;
		for (auto &c: computers)
			if (c.ip == item["ip"].as<string>())
			{
				exists = true;
				c.services.push_back(item["service"].as<int>());
			}

		if (!exists)
		{
			computers.push_back(Computer());
			computers.back().ip = item["ip"].as<string>();
			computers.back().services.push_back(item["service"].as<int>());
		}
	}
}

/** @brief Returns Computer pointer form computers list with lowest number of
 * resources used.
 *
 * @return
 * * @c NULL  If all computers are blocked, or
 * `computer[i].resources + job_resources > MAX_COMPUTER_RESOURCES`
 * (i.e. the job cannot be scheduled to any of the
 * machines) <br>
 * * Computer* otherwise.
 */
Computer* findFreeComputer(vector<Computer> &computers,
										   	int job_service, int job_resources)
{
	sort(computers.begin(), computers.end()); // sortowanie po resources

	for (auto &c: computers)
		if (!c.blocked && c.hasService(job_service) &&
			c.resources + job_resources <= MAX_COMPUTER_RESOURCES )
			return &c;
	return NULL;
}


/** @brief Searches the computer witch executes the job and frees its resources.
 *
 */
void releaseComputer(vector<Computer> &computers, Job *job)
{
	for (auto &c: computers)
		if (c.ip == job->getComputerIp())
		{
// 			c.blocking_job_id = -1;
			c.resources -= job->getResources();
			c.num_jobs--;
		}

}

/** @brief Returns pointer to Computer with least amount of running jobs.
 *
 * @return
 * * @c NULL if all computers with appropriate service are blocked
 * * @c Computer* otherwise
 */
Computer* findLeastOccupiedComputer(vector<Computer> &computers, int job_service)
{
	sort(computers.begin(), computers.end(),
			[](const Computer& lhs, const Computer& rhs)
			{ return lhs.num_jobs < rhs.num_jobs; });

	for (auto &c: computers)
		if (!c.blocked && c.hasService(job_service) )
			return &c;
	return NULL;
}


/** @brief Searches for a computer reserved for the job
 *
 * @return
 * * @c NULL if there is no computer reserved for the job (which shouldn't
 * happen) or a blocked computer has no available resources to run the job
 * * @c Computer* otherwise
 */
Computer* findBlockedComputer(vector<Computer> &computers,
											long unique_id, int job_resources)
{
	for (auto &c: computers)
		if (c.blocking_job_id == unique_id &&
			c.resources + job_resources <= MAX_COMPUTER_RESOURCES)
			return &c;
	return NULL;
}

inline const char * const boolToString(bool b)
{
	  return b ? "t" : "f";
}


/** @brief For each message in messages creates a job (message parsing in
 * constructor) and appends to jobs
 *
 */
void pushBackJobsFromMessages(map<int, vector<Job> > &jobs,
	   													vector<string> messages)
{
	for (auto m: messages)
	{
		Job j(m);

		jobs[j.getPriority()].push_back( j );
	}
}


/** @brief Clears items with empty vectors
 *
 */
void clearEmtyItems(map<int, vector<Job> > &jobs)
{
	for (auto it = jobs.cbegin(); it != jobs.cend(); )
		if (it->second.empty())
			jobs.erase(it++);    // or "it = m.erase(it)" since C++11
		else
			++it;
}

