/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_student1_student2.hpp"

static int pick_highest_priority_index(const std::vector<PCB> &ready_queue){
    if(ready_queue.empty()){
        return -1;
    }
    int best_idx = 0;
    for(size_t i = 1; i < ready_queue.size(); i++){
        if(ready_queue[i].priority > ready_queue[best_idx].priority){
            best_idx = (int)i;
        } else if(ready_queue[i].priority == ready_queue[best_idx].priority){
            if(ready_queue[i].arrival_time < ready_queue[best_idx].arrival_time){ //if tie earlier arrival gets priority
                best_idx = (int)i;
            }
        }
    }
    return best_idx;
}

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
    std::vector<unsigned int> wait_remaining; //remaining time for each wait_queue entry in ms
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    std::vector<PCB> pending = list_processes; //processes that havnt been accepted into job list yet

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(true) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.

        //process arrivals at current time
        for(auto it = pending.begin(); it != pending.end();) 
        {
            if(it->arrival_time <= current_time){
                bool ok = assign_memory(*it); //try to allocate mem
                if (ok){
                    //accepted int mem and ready
                    it->state = READY;
                    it->start_time = -1;
                    it->cpu_since_last_io = 0;
                    ready_queue.push_back(*it);
                    job_list.push_back(*it);
                    execution_status += print_exec_status(current_time, it->PID, NEW, READY);

                    it = pending.erase(it);
                } else{
                    //mem not available
                    ++it; //leave process pending and try again later
                }
            } else{
                ++it;
            }
        }

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

        //if cpu idle, choose highest priority ready process
        if (running.PID == -1 || running.state != RUNNING){
            if(!ready_queue.empty()){
                //pick highest priority by index
                int idx = pick_highest_priority_index(ready_queue);
                if(idx >= 0){
                    PCB proc = ready_queue[idx];//move to running
                    ready_queue.erase(ready_queue.begin() + idx); //remove from ready queue

                    states old_state = proc.state;
                    proc.state = RUNNING;
                    if(proc.start_time == -1){
                        proc.start_time = current_time;
                    }
                    sync_queue(job_list, proc);
                    
                    //log state 
                    execution_status += print_exec_status(current_time, proc.PID, old_state, RUNNING);

                    // Print memory snapshot only when the process *actually* starts running this cycle
                    if (proc.start_time == current_time) {
                        std::stringstream ss;
                        ss << "Memory snapshot at start of PID " << proc.PID << ":\n";

                        unsigned int total_free = 0, total_used = 0, total_usable = 0;
                        for (int p = 0; p < 6; ++p) {
                            ss << "Partition " << memory_paritions[p].partition_number << " (" 
                            << memory_paritions[p].size << "MB): ";
                            if (memory_paritions[p].occupied == -1) {
                                ss << "FREE\n";
                                total_free += memory_paritions[p].size;
                                total_usable += memory_paritions[p].size;
                            } else {
                                ss << "PID " << memory_paritions[p].occupied << "\n";
                                total_used += memory_paritions[p].size;
                            }
                        }
                        ss << "Total used: " << total_used << " MB\n";
                        ss << "Total free: " << total_free << " MB\n";
                        ss << "Total usable free: " << total_usable << " MB\n";

                        execution_status += ss.str();
                    }
                    
                    //copy into running
                    running = proc;
                }
            }
        }

        //advance running process
        if (running.PID != -1 && running.state == RUNNING){
            if (running.remaining_time > 0){
                --running.remaining_time;
                ++running.cpu_since_last_io;
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
                //else continue running
            }
        }

        //termination conditions, no pedning, ready wait or running
        bool all_done = true;
        if(!pending.empty()){
            all_done = false;
        }
        if(!ready_queue.empty()){
            all_done = false;
        }
        if(!wait_queue.empty()){
            all_done = false;
        }
        if(running.PID != -1 && running.state != TERMINATED){
            all_done = false;
        }
        if(!job_list.empty()){
            for (const auto &p : job_list){
                if(p.state != TERMINATED){
                    all_done = false;
                    break;
                }
            }
        }

        if(all_done){
            break;
        }

        //prevent infinte sim
        if (current_time > 60u * 1000U * 5u){ //five min as saftey
            std::cerr << "Simulation time exceeded saftey limit, aborting.\n";
            break;
        }

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
