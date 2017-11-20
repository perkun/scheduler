# Message Queues

## only scheduler can create and destroy MQs



`x -> y`: x sends message for y to read

## MessageQueue Mutator -> Scheduler

* `int` task_id
* `int` mutator_id
* `int` priority
* `string` path
* `int` service			// foto, radar, occ, ...
* `int` resources

## MessageQueue Scheduler -> Crankshaft

* `string` computer_ip
* `int` task_id
* `int` mutator_id 	?????
* `string` path
* `string` program_name

## MessageQueue Crankshaft -> Scheduler

* `int` task_id
* `int` status

## MessageQueue Scheduler -> Mutator

* `int` task_id
* ???

## Message queues keys and types

| From | To   | key | type |
| --------------------|----:|-----|
| Mutator    | Scheduler  |1234|1|
| Crankshaft | Scheduler  |1234|2|
| Scheduler  | Crankshaft |1234|3|
| Scheduler  | Manager    |1234|4|
| Scheduler  | Mutator    |1234|5|




# Database

## Table jobs
CREATE TABLE jobs (
	jobid int,
	ip inet,
	service int,
	resources int,
	starttime timestamp DEFAULT CURRENT_TIMESTAMP,
	mutatorid int,
	priority int,
	path varchar
);

CREATE TABLE services (
	ip inet,
	service int
);

CREATE TABLE blocked (
	ip inet,
	jobid int
);


