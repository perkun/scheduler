#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#define MAXSIZE     128

void die(char *s)
{
  perror(s);
  exit(1);
}

struct msgbuf
{
    long    mtype;
    char    mtext[MAXSIZE];
} ;


int main()
{
    int msqid;
    key_t key;
    struct msgbuf rcvbuffer;
	char c='q';

    key = 1234;

    if ((msqid = msgget(key, 0666)) < 0)
      die("msgget()");


	while (1)
	{
		printf("=== reading loop:\n");
		while (1)
		{

			//Receive an answer of message type 1. BEZ ZATRZASKU -> IPC_NOWAIT
			if (msgrcv(msqid, &rcvbuffer, MAXSIZE, 1, IPC_NOWAIT) < 0)
			{
				if (errno == ENOMSG)
				{
					printf("!!! no messages, continuing...\n");
					break;
				}
				else
					die("msgrcv");
			}
			else
				printf("%s\n", rcvbuffer.mtext);


		}
		printf("=== end of reading loop, getchar() \n");
		getchar();
	}



    exit(0);
}
