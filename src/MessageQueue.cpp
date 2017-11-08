#include "MessageQueue.h"

void MessageQueue::die(const char *s)
{
	perror(s);
	exit(1);
}


// MessageQueue::MessageQueue(int k)
// {
// 	key = k;
// //
// 	if ((msqid = msgget(key, 0666)) < 0)
// 		die("msgget()");
// //
// }


vector<string> MessageQueue::readQueue(int k, long t)
{
	key = k;
	rcvbuffer.mtype = t;

	vector<string> messages;

	if ((msqid = msgget(key, 0666)) < 0)
		die("msgget()");

	//Receive an answer of message type 1. BEZ ZATRZASKU -> IPC_NOWAIT
	while (1)
	{
		if (msgrcv(msqid, &rcvbuffer, MAXSIZE, t, IPC_NOWAIT) < 0)
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


int MessageQueue::sendMessage(int k, long t, string message)
{
	key = k;
	rcvbuffer.mtype = t;

	if ((msqid = msgget(key, 0666)) < 0)
		return -1;

	sprintf(rcvbuffer.mtext, "%s", message.c_str() );

	size_t buflen = strlen(rcvbuffer.mtext) + 1;

	if (msgsnd(msqid, &rcvbuffer, buflen, IPC_NOWAIT) < 0)
	{
		printf ("%d, %ld, %s, %lu\n", msqid, rcvbuffer.mtype, rcvbuffer.mtext, buflen);
		die("msgsnd");
	}


	return 0;
}
