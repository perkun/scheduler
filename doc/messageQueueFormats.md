# Message Queues

## only scheduler can create and destroy MQs



`x -> y`: x sends message for y to read

## MessageQueue Mutator -> Scheduler

* `int` mutator_id
* `int` task_id
* `int` priority
* `string` path
* `int` service			// foto, radar, occ, ...
* `int` resources

## MessageQueue Scheduler -> Crankshaft

* `string` computer_ip
* `int` task_id
* `string` path

## MessageQueue Crankshaft -> Scheduler

* `int` job_id
* `int` status

## MessageQueue Scheduler -> Mutator

* `int` job_id
* `int` mutator_id
* `int` task_id
* `int` status

## Message queues keys and types

| From | To   | key | type |
| ------------|--------|----:|-----|
| Mutator    | Scheduler  |1000|1|
| Scheduler  | Mutator    |1001|1|
| Scheduler  | Crankshaft |2000|1|
| Crankshaft | Scheduler  |2001|1|
| Scheduler  | Manager    |1000|4|
| corba error| Sheduler   |6000|job_id|




# Database

## Table jobs
CREATE TABLE jobs (
	jobid bigint,
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


