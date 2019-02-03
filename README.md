sheduler
------

# Scheduler for OA Cluster

## Program arguments:

	--help | -h                    print this help
	--logm <filename>              output all received and sent messages to file
	--verbose | -v                 print stuff on da screen
	--noclear                      do NOT clear message queues

## Description

This program reads IPC messages queues from Mutators (programs handling
computational tasks), then schedules the tasks to computers and send message to
Crankshaft program, which distributes tasks to Work Stations using CORBA.
