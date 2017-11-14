
# Message Queues

## only scheduler can create and destroy MQs



`x -> y`: x sends message for y to read

## MessageQueue Mutator -> Scheduler

* `int` task_id
* `int` mutator_id
* `int` priority
* `string` path
* `int` service			// foto, radar, occ, ...

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

| Mutator			  | key | type |
| --------------------|----:|-----|
| Mutator MessageQueue |1234|1|
| Scheduler MessageQueue |1234|2|
| Crankshaft MessageQueue |1234|3|
| Manager MessageQueue |1234|4|




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








			"SELECT j.ip, (1-j.sum) AS free_res "
			"FROM "
		   	"(SELECT jobs.ip, sum(jobs.weight) FROM jobs "
			"GROUP BY  ip order by sum) AS j "
			"INNER JOIN "
			"(SELECT services.ip FROM services WHERE services.service=%d) AS k "
			"ON j.ip=k.ip order by free_res DESC;",
