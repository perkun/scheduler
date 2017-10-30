#include "MessageQueue.h"

void MessageQueue::die(const char *s)
{
	perror(s);
	exit(1);
}


MessageQueue::MessageQueue(int k)
{
	key = k;

	if ((msqid = msgget(key, 0666)) < 0)
		die("msgget()");

}


vector<string> MessageQueue::readQueue()
{
	vector<string> messages;

	//Receive an answer of message type 1. BEZ ZATRZASKU -> IPC_NOWAIT
	while (1)
	{
		if (msgrcv(msqid, &rcvbuffer, MAXSIZE, 1, IPC_NOWAIT) < 0)
		{
			if (errno == ENOMSG)
			{
				// 	printf("!!! no messages, continuing...\n");
				break;
			}
			else
				die("msgrcv");
		}
		else
		{
			string buff = rcvbuffer.mtext;
			messages.push_back(buff);

			// 	printf("%s\n", rcvbuffer.mtext);

		}
	}
	return messages;
}
