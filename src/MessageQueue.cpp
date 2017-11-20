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
// 	printf("reading queue\n");
	key = k;
	rcvbuffer.mtype = t;

	vector<string> messages;

	if ((msqid = msgget(key, 0666)) < 0)
		die("readQueue");

	//Receive an answer of message type 1. BEZ ZATRZASKU -> IPC_NOWAIT
	while (1)
	{
		for (int i = 0; i < MAXSIZE; i++)
			rcvbuffer.mtext[i] = 0;


		if (msgrcv(msqid, &rcvbuffer, MAXSIZE, t, IPC_NOWAIT) < 0)
// 		if (msgrcv(msqid, &rcvbuffer, MAXSIZE, t, 0) < 0)
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

string MessageQueue::readMessageNowait(int k, long t)
{
	key = k;
	rcvbuffer.mtype = t;

	string message;

	if ((msqid = msgget(key, 0666)) < 0)
		die("readQueue");

	if (msgrcv(msqid, &rcvbuffer, MAXSIZE, t, 0) < 0)
		die("msgrcv");
	else
		message = rcvbuffer.mtext;

	return message;
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
		die("sendMessage failed");
	}


	return 0;
}

void MessageQueue::recreate(int k)
{
	int msgflg = IPC_CREAT | 0666;
	key = k;

	if ((msqid = msgget(key, 0666 )) < 0)
	{
		//       die("clearing error ");
		printf("no queue to destroy\n");
// 		return;
	}
	else if (msgctl(msqid, IPC_RMID, NULL) == -1)
		printf("Message queue could not be deleted\n");

	printf("creating new queue\n");
	msqid = msgget(key, msgflg);
}
