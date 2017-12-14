#include "MessageQueue.h"

void MessageQueue::die(const char *s)
{
	perror(s);
	exit(1);
}

MessageQueue::MessageQueue()
{
// 	logfile = fopen("messageQueue.log", "a");
// 	fprintf(logfile, "########## START ########## \n");

	num_received = 0;
}

MessageQueue::~MessageQueue()
{
// 	fclose(logfile);
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
	vector<string> messages;
	struct msqid_ds	queue_stats;

	logfile = fopen("messageQueue.log", "a");
	key = k;

	if ((msqid = msgget(key, 0666)) < 0)
		die("readQueue");

	if (msgctl(msqid, IPC_STAT, &queue_stats) < 0)
		return messages;

	if (queue_stats.__msg_cbytes >= (unsigned long) queue_stats.msg_qbytes)
		fprintf(logfile, "BYTES: using:%ld max:%ld \n",
				queue_stats.__msg_cbytes,
				(unsigned long) queue_stats.msg_qbytes);


	for (int i = 0; i < (int)queue_stats.msg_qnum; i++)
	{
		struct MessageBuffer rcvbuffer;
		rcvbuffer.mtype = t;
		memset(rcvbuffer.mtext, '\0', sizeof rcvbuffer.mtext);

		int status;
		status = msgrcv(msqid, &rcvbuffer, MAXSIZE, t, 0);
		if (status < 0)
		{
			if (errno == ENOMSG)
			{
				// 	printf("!!! no messages, continuing...\n");
				fprintf(logfile, "no messages, continuing\n");
				break;
			}
			else
				die("msgrcv");
		}
		else if (status == 0)
			cout << "message status = 0" << "\n";
		else
		{
			num_received++;
			string buff = rcvbuffer.mtext;

			messages.push_back(buff);
			fprintf(logfile, "READ_PCKG key=%d, t=%ld |%s|l=%d,n_r=%d\n",
				   	key, t, rcvbuffer.mtext, status, num_received);
		}
	}
	fclose(logfile);
	return messages;
}

string MessageQueue::readMessageLock(int k, long t)
{
	struct MessageBuffer rcvbuffer;
// 	for (int i = 0; i < MAXSIZE; i++)
// 		rcvbuffer.mtext[i] = '\0';
	memset(rcvbuffer.mtext, '\0', sizeof rcvbuffer.mtext);

	key = k;
	rcvbuffer.mtype = t;

	string message;
	message.clear();

	if ((msqid = msgget(key, 0666)) < 0)
		die("readQueue");

	if (msgrcv(msqid, &rcvbuffer, MAXSIZE, t, 0) < 0)
		die("msgrcv");
	else
		message = rcvbuffer.mtext;

// 	cout << "MESSAGE_LOCK key="<<k<<" type="<<t<<" text="<< rcvbuffer.mtext << "\n";
// 	fprintf(logfile, "READ_LOCK key=%d, t=%ld |%s\n", key, t, message.c_str());

// 	num_received++;
	return message;
}


int MessageQueue::sendMessage(int k, long t, string message)
{
// 	cout << k<< "\t"<<  t << "\n";
	struct MessageBuffer rcvbuffer;
// 	for (int i = 0; i < MAXSIZE; i++)
// 		rcvbuffer.mtext[i] = '\0';
	memset(rcvbuffer.mtext, '\0', sizeof rcvbuffer.mtext);

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

// 	fprintf(logfile, "SENDING   key=%d, t=%ld |%s\n", key, t, message.c_str());
// 	cout << "SENDING key="<<k<<" type="<<t<<" : " << rcvbuffer.mtext << "\n";

	num_sent++;

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

void MessageQueue::cleanQueue(int k)
{
	readQueue(k, 0);
}
