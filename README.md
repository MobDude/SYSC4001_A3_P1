Schedulers to implement:
  1. External Priorities (no preemption). Modify your PCB as needed [0.5 mark].
  2. Round Robin with a 100 ms timeout [0.5 mark].
  3. A combination of both: external Priorities including preemption and a 100ms timeout, in
Round-Robin fashion [0.5 mark].

Implement the 3 schedulers as their separate files so that it is easier to test and analyse.

Test Scenarios

Simulation Execution [1 mark]
Run at least 20 different simulation scenarios for each of the different schedulers you defined.
Analyze the results obtained in the simulation and write a report (2 pages minimum – can be
longer if you find interesting results and you want to elaborate) discussing the results of the
simulation execution.

To do that, you should first compute different metrics based on your simulation results:
Throughput, Average Wait Time, Average Turnaround time, Average Response Time (i.e.,
the time between 2 I/O operations). Use the metrics to compare how the algorithms perform with
mostly I/O bound, mostly CPU-bound processes, or processes with similar I/O and CPU bursts.
The metrics can be calculated externally (python script, C program) or be a part of your simulator
(but this will increase the simulator complexity).

You must run the different simulation scenarios, collect simulation results, and analyze the
metrics above for the different simulation scenarios. Discuss the results you obtained and
compare the algorithms based on your simulations. You should analyze the metrics and the
simulation scenarios and discuss the results obtained, thinking why each of the scheduling
algorithms favor the different kinds of processes.

[1 bonus mark] Record the use of memory in your simulator and analyze the results obtained.
Discuss the results in the report (highlight it as bonus).
