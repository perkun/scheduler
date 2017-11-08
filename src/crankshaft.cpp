
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

#include "MessageQueue.h"

#define MAXSIZE 128

using namespace std;

int main () {


	MessageQueue msq;

	while (1)
	{
		vector<string> messages = msq.readQueue(1234, 3);

		if (messages.empty())
			printf("msqueue empty\n");
		else
		{
			cout << "========== received messages ==========" << "\n";
			for (string s: messages)
				cout << s << "\n";
			cout << "==============================" << "\n";
		}

		cout << "type <task_id> <status> to finish it:";
 		int id = 0, status = 0;

		char buf[MAXSIZE];

		scanf("%d %d", &id, &status);
		cout << "\n";

		printf("id status: %d %d\n", id, status);

		sprintf(buf, "%d %d", id, status);

		if (id < 0)
			continue;
		else
			// wyslij komunikat z zakonczonym zadaniem
			msq.sendMessage(1234, 2, buf);

		getchar();
	}

	return 0;
}
