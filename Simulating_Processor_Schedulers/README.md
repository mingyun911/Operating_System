## Project #2: Simulating Processor Schedulers

### *** Due on 24:00, May 4 (Friday) ***


### Goal

We have learned various process scheduling policies and examined their properties in the class. To better understand them, you will implement the schedulers on an educational scheduler simulation framework which imitates the core of modern operating systems' process scheduler.


### Problem Specification

#### Basics

- The simulator maintains the time with `ticks` variable. It is increased by 1 when a scheduling takes place. You may read this variable but should not modify it by yourself.

- Firstly, we define a schedulable entity, which is a process. The simulator accepts a *process description file* as an argument which describes processes to simulate. Following example shows an example of the description file for two processes: process 1 and 2.

  ```
  process 1
    start 0
    lifespan 4
    prio 0
  end

  process 2
    start 5
    lifespan 10
    prio 10
  end
  ```

- In the example, the simulator will create the process 1 at tick 0 as `start 0` keyword specifies and keep it live until the process gets aged by 4 ticks (`lifespan 4`). The process is given a priority value 0 by default unless the priority is specified with `prio` keyword (as of `prio 0` and `prio 10`). The larger priority value implies the higher priority of the process. Likewise, process 2 will be forked at time of tick 5 and live for 10 ticks with priority 10. This information is also shown at the beginning of the program execution as follows.
  ```
  - Process 1: Forked at tick 0 and run for 4 ticks with initial priority 0
  - Process 2: Forked at tick 5 and run for 10 ticks with initial priority 10
  ```
- The process gets aged by 1 tick only if it is scheduled and runs for the entire tick. In other words, the process will not get aged when it is waiting for being scheduled in the `readyqueue` or blocked for some resources.

- The simulator will realize the processes described in the description file with `struct process` defined in `process.h`. See the file for the fields that describes processes in the system. It is prohibited to access the variables starting with two underbars.

- At any moment, `struct process *current` defined as a global variable points to the process that is currently running. You may use the variable whenever you need to access the currently running process.


#### Interacting with the framework

- The simulator implements these scheduling *mechanisms* (e.g., replacing the current, counting ticks, ... ), and it interacts with scheduling *policies* that are defined with `struct scheduler` in `sched.h`. `struct scheduler` is a collection of function pointers. The simulator will call the functions to ask the scheduling policy for making decisions. Have a look at `fcfs_scheduler` in `pa2.c` which implements the FCFS scheduler. You may also find other `scheduler` instances in `pa2.c` that are waiting for your implementation.

- `struct process *(*schedule)(void)` is the key function for the scheduling policy. The simulator invokes this function whenever it needs to schedule a process to run next. Accordingly, the function should return a process to run next, or return NULL to indicate the framework that there is no process to run. See `fcfs_schedule()` in `pa2.c`.

- The simulator has the ready queue `struct list_head readyqueue`. It is supposed to keep the list of processes that are ready to run. Note that *the current process should not be in the ready queue* as it is currently running not ready to run.

- When a process is created by the simulator, `forked()` callback function will be invoked. Similarly, when the process is done, `exiting()` callback function is called.


#### Simulating resources

- The system has 16 system resources that can be assigned to a process *exclusively*. `struct resource` abstracts the system resources in `resource.h`. The process may ask the simulator to acquire a resoruce and release it after use. Such a resource use is specified in the process description file using `acquire` keyword. For example, `acquire 1 4 2` means the process will require resource #1 when it is aged for 4 ticks and once it is acquired it will use the resource for 2 ticks. Have a look at `testcases/resources` for an example.

- When the simulator gets the resource acquisition request, it calls `acquire()` function of the scheduler. Similarly, the simulator calls `release()` function when the process finishes using the resource. You may find default FCFS acquire/release functions in `pa2.c` which are used by the FCFS scheduler.

- Non-priority-based scheduling policies should handle resource acquision requests in the first-come-first-served way. On the other hand, priority-based scheduling policies should dispatch the released resource to the process with the highest priority. To this end, you may define your own acquire/release functions and associate them to your scheduler implementation to make a correct scheduling decision. If two processes with the same priority are requesting the same resource, the one came earlier receives the resource.

- The resources are managed in a non-preemptive manner; once a process successfully acquired a resource, it is guaranteed that the resource is assigned to the process until the process explicitly releases it.


#### Scheduling Policies
- The simulator is waiting for your implementation of shortest-job first (SJF) scheduler, shortest time-to-complete first (STCF) scheduler, round-robin scheduler, base priority-based scheduler, priority-based scheduler with aging (PA), priority-based scheduler with priority ceiling protocol (PCP), and priority-based scheduler with priority inheritance protocol (PIP). You can select a scheduler to run with a starting option, and the simulator will be set to use the corresponding scheduler automatically. Check the options by running the program (`sched`) without any option.

- FCFS and SJF are supposed to be non-preemptive; even the simulator may ask the scheduler to select next process to run at every tick, the schedulers should not change currently running process unless it is completed. STCF scheduler can preempt the currently running process when a process with a higher priority arrives, but should keep the current process otherwise.

- For round-robin scheduler, the time quantum coincides with the tick; when the framework calls `schedule()`, it implies the time quantum is expired. You may ignore the priority while implementing the RR scheduler.

- The priority-based schedulers should handle processes with the same priority in the round-robin way; If two or more processes are with the same priority, they should be switched on each tick.

- For the scheduler with aging, at every scheduling moment, the priority of the current process is reset to its original priority, and all processes in the readyqueue receive the priority boost by 1. The priority can be boosted up to `MAX_PRIO` defined in `process.h`. The scheduler should pick the process with the highest adjusted priority at this point. Note that the processes with the same priority should be handled in a round-robin manner just like the original priority scheduler.

- To boost the priority of processes in PCP scheduler, use `MAX_PRIO`.

- When you implement PIP, make sure that the priority of a process is set properly when it releases a resource. There are complicated cases to implement PIP.
  - More than one processes with different priority values can wait for the releasing resource. Suppose one process is holding one resource type, and other process is to acquire the same resource type. And then, another process with higher (or lower) priority is to acquire the resource type again, and then ...
  - Many processes with different priority values are waiting for different resources held by a process.
  You will get the full points for PIP *if and only if* these cases are all handled properly. Hint: calculate the *current* priority of the releasing process by checking resource acquisition status.
  - See [this](https://www.embedded.com/how-to-use-priority-inheritance/) for a comprehensive exposition.


### Tips and Restriction

- The grading system only examines the messages printed out to `stderr`. Thus, you can use `printf` as you want.
- Use `dump_status()` function to see the situation of processes and resources.
- Recall PA0 and PA1 for manipulating the `list_head` queues.
- Make sure you are using `list_for_*_safe` variants if an entry is removed from the list during the iteration, and `list_del_init` to remove an entry from the list. (Do some Internet search for their differences)


### Submission / Grading

- Use [PASS](https://sslab.ajou.ac.kr/pass) for submission
  - 550 pts + 20 pts
  - You can submit up to **20** times.
  - The details of some testcase results are hidden and you can only check the final decision (i.e., pass/fail);

- Code: ***pa2.c*** (500 pts)
  - SJF scheduler: 20pts (tested using `multi`)
  - STCF scheduler: 50pts (`multi`);
  - RR scheduler:  50pts (`multi` and `prio`)
  - Priority scheduler: 60pts (`prio` and `resources-prio`)
  - Priority scheduler + aging: 100pts (`prio`)
  - Priority scheduler + PCP: 70pts (`resources-basic`)
  - Priority scheduler + PIP: 150pts (`resources-adv1` and `resources-adv2`)

- Document: One PDF document (50 pts) including;
  - Sketch your idea for implementing **each** scheduling policy.
    - Do not explain the code itself. Instead, focus on explaining your key idea and approach.
  - Explain how the priority of the processes are changed over time for PIP using `resources-adv2`.
    - Explain **when, why, and how much** the priority inheritance takes place in the example.
    - DO NOT just copy-paste the output of a program.
  - Lesson learned
    - No need to recite what is explained in the class.
  - No more than **five** pages
  - Do not put screenshots of your code.

- Git repository (20 pts)
  - Register http URL and with a deploy token and password.
  - Start the repository by cloning this repository.
  - Make sure the token is valid through May 8 (due + 3 slip days + 1 extra day)

- *WILL NOT ANSWER THE QUESTIONS ABOUT THOSE ALREADY SPECIFIED ON THE HANDOUT.*
- *QUESTIONS OVER EMAIL WILL BE IGNORED UNLESS IT CONCERNS YOUR PRIVACY.*
