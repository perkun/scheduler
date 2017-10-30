#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>

#include "MessageQueue.h"
#include "Job.h"

#define MAXSIZE 128

using namespace std;



int main() {


	MessageQueue msq(1234);

	vector<Job> jobs;
	while (1)
	{
		vector<string> messages = msq.readQueue();
		for (auto m: messages)
			jobs.push_back( Job(m) );




		char c = getchar();

		if (c == 'q')
			break;
		else if (c == 'p')
			for (auto j: jobs)
				j.printInfo();

	}



	return 0;
}

