#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>

using namespace std;

class Options
{
public:
	Options(int argc, char *argv[]);
	void parse(int argc, char *argv[]);
	void printHelp();

	bool log_messages;
	string log_messages_filename;

	bool verbose;
	bool clear_queues;

};

#endif /* OPTIONS_H_ */
