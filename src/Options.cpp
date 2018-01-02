#include "Options.h"

Options::Options(int argc, char *argv[])
{
	log_messages = false;
	log_messages_filename = "messages.log";

	verbose = false;
	clear_queues = true;

	parse(argc, argv);
}

void Options::parse(int argc, char *argv[])
{
	if (argc == 1)
		return;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--logm") == 0 )
		{
			log_messages = true;
			log_messages_filename = argv[i+1];
		}

		if (strcmp(argv[i], "--help") == 0 ||  strcmp(argv[i], "-h") == 0)
		{
			printHelp();
			exit(1);
		}

		if (strcmp(argv[i], "--verbose") == 0 ||  strcmp(argv[i], "-v") == 0)
			verbose = true;

		if (strcmp(argv[i], "--noclean") == 0 )
			clear_queues = false;
	}

}

void Options::printHelp()
{
	char text[] =
"\n"
"                                                                                \n"
"Options:                                                                        \n"
"                                                                                \n"
"--help | -h                    print this help                                  \n"
"--logm <filename>              output all received and sent messages to file    \n"
"                                                                                \n"
"--verbose | -v                 print stuff on da screen                         \n"
"--noclear                      do NOT clear message queues                      \n"
"                                                                                \n"
"                                                                                \n"
"                                                                                \n"
"                                                                                \n";
	cout << text << "\n";

}
