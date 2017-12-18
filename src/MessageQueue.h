#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAXSIZE 128

using namespace std;


struct MessageBuffer
{
    long    mtype;
    char    mtext[MAXSIZE];
};



class MessageQueue
{
public:
	int num_sent;
	int num_received;
 	MessageQueue();
 	~MessageQueue();
// 	MessageQueue(int);
	vector<string> readQueue(int k, long t);
	string readMessageLock(int k, long t);
	int sendMessage(int k, long t, string message);
	void recreate(int k);
	void die(const char *s);
	void cleanQueue(int k);

// 	struct MessageBuffer rcvbuffer;

	bool log_messages;
	string log_messages_filename;

    int msqid;
    key_t key;
	char c='q';

	FILE *logfile;

};

// struct ipc_perm {
// 	key_t          __key;       /* Key supplied to msgget(2) */
// 	uid_t          uid;         /* Effective UID of owner */
// 	gid_t          gid;         /* Effective GID of owner */
// 	uid_t          cuid;        /* Effective UID of creator */
// 	gid_t          cgid;        /* Effective GID of creator */
// 	unsigned short mode;        /* Permissions */
// 	unsigned short __seq;       /* Sequence number */
// };

// struct msqid_ds {
// 	struct ipc_perm msg_perm;     /* Ownership and permissions */
// 	time_t          msg_stime;    /* Time of last msgsnd(2) */
// 	time_t          msg_rtime;    /* Time of last msgrcv(2) */
// 	time_t          msg_ctime;    /* Time of last change */
// 	unsigned long   __msg_cbytes; /* Current number of bytes in
// 									 queue (nonstandard) */
// 	msgqnum_t       msg_qnum;     /* Current number of messages
// 									 in queue */
// 	msglen_t        msg_qbytes;   /* Maximum number of bytes
// 									 allowed in queue */
// 	pid_t           msg_lspid;    /* PID of last msgsnd(2) */
// 	pid_t           msg_lrpid;    /* PID of last msgrcv(2) */
// };
#endif /* MESSAGEQUEUE_H_ */
