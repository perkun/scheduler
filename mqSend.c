#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAXSIZE     128
// #define MAXSIZE     99999

void die(char *s)
{
  perror(s);
  exit(1);
}

struct msgbuf
{
    long    mtype;
    char    mtext[MAXSIZE];
};

int main()
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    struct msgbuf sbuf;
    size_t buflen;

    key = 1234;


	// niszczy kolejke
	msgctl(msqid, IPC_RMID, NULL);

    if ((msqid = msgget(key, msgflg )) < 0)   //Get the message queue ID for the given key
      die("msgget");

    //Message Type
    sbuf.mtype = 1;

	int i;
	for(i=0;i<100000 ;i=i+1)
	{
		bzero(sbuf.mtext,sizeof(sbuf.mtext));
		if(msgrcv(msqid, &sbuf, sizeof(sbuf.mtext),0,IPC_NOWAIT)==-1)
		{
			i=10000000;
		}
	}

    printf("MQ started\n");

	int counter = 1;
	while (1)
	{
 		char bufor[20];

		scanf("%[^\n]",bufor);
// 		scanf("%[^\n]",sbuf.mtext);
		getchar();

// 		scanf("%s", bufor);
// 		scanf("%s", sbuf.mtext);

		sprintf(sbuf.mtext, "%d 0 %s /home jasnosc.cpp", counter, bufor);

		buflen = strlen(sbuf.mtext) + 1 ;

		if (msgsnd(msqid, &sbuf, buflen, IPC_NOWAIT) < 0)
		{
			printf ("%d, %ld, %s, %lu\n", msqid, sbuf.mtype, sbuf.mtext, buflen);
			die("msgsnd");
		}
		counter++;
	}

    exit(0);
}
