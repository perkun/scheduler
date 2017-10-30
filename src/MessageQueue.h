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

	MessageQueue(int);
	vector<string> readQueue();
	void die(const char *s);

	struct MessageBuffer rcvbuffer;

    int msqid;
    key_t key;
	char c='q';

};

#endif /* MESSAGEQUEUE_H_ */
