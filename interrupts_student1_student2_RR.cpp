/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_student1_student2.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<unsigned int> wait_remaining; //The remaining wait time of each entry in the wait queue
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    unsigned int quantia = 100;
    unsigned int running_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the wait queue

        //manage wait queue and decrement remaining io times, move to ready when 0
        for(size_t i =0; i < wait_queue.size();){
            if (wait_remaining[i] == 0){
                //io completed move from waiting to ready
                PCB proc = wait_queue[i]; //copy
                states old_state = WAITING;
                proc.state = READY;
                proc.cpu_since_last_io = 0; //reset counter after io
                
                sync_queue(job_list, proc); //update job list entry

                //push back to ready queue
                ready_queue.push_back(proc); 
                execution_status += print_exec_status(current_time, proc.PID, old_state, READY);

                //remove from wait queue
                wait_queue.erase(wait_queue.begin() + i);
                wait_remaining.erase(wait_remaining.begin() +i);
            } else{
                --wait_remaining[i]; //decrement after check
                ++i;
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////

        FCFS(ready_queue); //Sort ready queue by FCFS

        //if cpu idle, choose highest priority ready process
        if (running.PID == -1 || running.state != RUNNING){
            if(!ready_queue.empty()){

                PCB proc = ready_queue[ready_queue.size() - 1];                  // move to running
                ready_queue.pop_back();                                          // remove from ready queue

                states old_state = proc.state;
                proc.state = RUNNING;
                if (proc.start_time == -1)
                {
                    proc.start_time = current_time;
                }
                sync_queue(job_list, proc);

                // log state
                execution_status += print_exec_status(current_time, proc.PID, old_state, RUNNING);

                //reset running_time
                running_time = 0;

                // copy into running
                running = proc;

                //log mem snapshot
                {
                    std::stringstream ss;
                    ss << "\nMemory snapshot after starting process " << running.PID << ":\n";
                    unsigned int total_free = 0, total_usable = 0, total_used = 0;
                    for(int i = 0; i <6; ++i){
                        ss << "Partition " << memory_paritions[i].partition_number << " (" << memory_paritions[i].size << "MB): ";
                        if(memory_paritions[i].occupied == -1){
                            ss << "FREE\n";
                            total_free += memory_paritions[i].size;
                            total_usable += memory_paritions[i].size;
                        }else{
                            ss << "PID" << memory_paritions[i].occupied << "\n";
                            total_used += memory_paritions[i].size;
                        }
                    }
                    ss << "Total used: " << total_used << " MB\n";
                    ss << "Total free: " << total_free << " MB\n";
                    ss << "Total usable free: " << total_usable << " MB\n";
                    execution_status += ss.str();
                    execution_status += "\n";
                }
            }
        }

        //advance running process
        if (running.PID != -1 && running.state == RUNNING){
            if (running.remaining_time > 0){
                --running.remaining_time;
                ++running.cpu_since_last_io;
                ++running_time;
            }

            //check for termination
            if(running.remaining_time == 0){
                states old_state = RUNNING;
                running.state = TERMINATED;
                sync_queue(job_list, running);

                //free mem
                free_memory(running);
                execution_status += print_exec_status(current_time + 1, running.PID, old_state, TERMINATED);

                //mark running as idle
                idle_CPU(running);
            } else{
                //check if running should perform io
                if (running.io_freq > 0 && running.cpu_since_last_io >= running.io_freq){
                    //move running to waiting
                    states old_state = RUNNING;
                    running.state = WAITING;
                    sync_queue(job_list, running);
                    execution_status += print_exec_status(current_time+ 1, running.PID, old_state, WAITING);

                    //push to wait queue with remaining dur
                    wait_queue.push_back(running);
                    wait_remaining.push_back(running.io_duration);

                    //running becomes idle
                    idle_CPU(running);
                } 
                //check if running out of allocated time
                else if (running_time == quantia) {
                    //move running to ready
                    states old_state = RUNNING;
                    running.state = READY;
                    sync_queue(job_list, running);
                    execution_status += print_exec_status(current_time + 1, running.PID, old_state, READY);

                    //push to ready queue
                    ready_queue.push_back(running);

                    //running becomes idle
                    idle_CPU(running);
                }
                //else continue running
            }
        }
        /////////////////////////////////////////////////////////////////

        ++current_time; //advance time by 1 ms

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}