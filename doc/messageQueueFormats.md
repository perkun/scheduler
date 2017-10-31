
`x -> y`: x sends message for y to read


# MessageQueue Mutator -> Scheduler

* `int` task_id
* `int` mutator_id
* `int` priority
* `string` path
* `string` program_name

# MessageQueue Scheduler -> Corbowod

* `string` computer_ip
* `int` task_id
* `int` mutator_id 	?????
* `string` path
* `string` program_name


# Mutator MessageQueue key = 1234
# Crankshaft MessageQueue key = 1234
# Mutator Message type = 1
# Scheduler Message type = 2
# Crankshaft Message type = 3
