#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "MessageQueue.h"

#define MAXSIZE 128

using namespace std;


long scheduler_message_type = 2;
int scheduler_queue_id = 1234;

int main () {


	MessageQueue msq;

	while (1)
	{
		string m = msq.readMessageNowait(1234, 3);

		pid_t pid = fork();
		if (pid == 0)
		{
			// child process
			cout << m << "\n";

			// rozczytaj komunikat
			char computer_ip[100], path[100], program_name[100];
			int task_id, mutator_id;

			sscanf(m.c_str(), "%s %d %d %s %s", computer_ip, &task_id,
					&mutator_id, path, program_name);

			////////// RUN CORBA //////////
			sleep(20);
			///////////////////////////////

			char buf[100];
			sprintf(buf, "%d %d", task_id, 2);
			msq.sendMessage(scheduler_queue_id, scheduler_message_type,
					buf);

			return 1;
		}


	}

	return 0;
}

// 		else
// 		{
// 			cout << "========== received messages ==========" << "\n";
// 			for (string s: messages)
// 				cout << s << "\n";
// 			cout << "==============================" << "\n";
// 		}
//
// 		cout << "type <task_id> <status> to finish it:";
//  		int id = 0, status = 0;
//
// 		char buf[MAXSIZE];
//
// 		scanf("%d %d", &id, &status);
// 		cout << "\n";
//
// 		printf("id status: %d %d\n", id, status);
//
// 		sprintf(buf, "%d %d", id, status);
//
// 		if (id < 0)
// 			continue;
// 		else
// 			// wyslij komunikat z zakonczonym zadaniem
// 			msq.sendMessage(1234, 2, buf);
//
// 		getchar();
