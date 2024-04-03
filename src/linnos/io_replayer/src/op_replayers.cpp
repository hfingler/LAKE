/*
 * Part of LAKE: Towards a Machine Learning-Assisted Kernel with LAKE
 * Copyright (C) 2022-2024 Henrique Fingler
 * Copyright (C) 2022-2024 Isha Tarte
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <ctime>
#include <iostream>
#include <cinttypes>
#include "op_replayers.hpp"
#include <errno.h>
#include <random>
#include <cstdlib>

#define MAX_FAIL 2

std::random_device rd;
std::mt19937 gen(rd());
std::mt19937 gen2(rd());
std::uniform_int_distribution<> distribution2(1, 2);
std::uniform_int_distribution<> distribution(1, 100); // Example range from 1 to 100

static int sleep_until(uint64_t next) {
    uint64_t now = get_ns_ts();
    int64_t diff = next - now;

    //if 0 or negative, we need to issue
    if(diff <= 0) {
        //we're late by at least 2 us
        if (diff <= -2000) return 1;
        return 0; //late but not that much
    }
    else 
        std::this_thread::sleep_for(std::chrono::nanoseconds(diff));
    return 0;
}

void baseline_execute_op(TraceOp &trace_op, Trace *trace, uint32_t device, char* buf) {
    int ret;
    int *fds = trace->get_fds();
    //read
    if(trace_op.op == 0) {
        trace->add_io_count(device);
        ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
    } 
    else if(trace_op.op == 1) {
        trace->add_io_count(device);
        ret = pwrite(fds[device], buf, trace_op.size, trace_op.offset);
    } else {
        printf("Wrong OP code! %d\n", trace_op.op);
    }

    if (ret < 0){
        printf("FAILURE ret: %d\n", ret);
        printf("IO error during pread/pwrite: %s\n", strerror(errno));
        printf("err %d\n", errno);
        printf("offset in B : %lu\n", trace_op.offset );
        printf("size in B : %lu\n", trace_op.size);
    }
}

/*
    This method randomly cancels the request for LAKE, as specified by the rand_int generated. After selecting
    whether or not to cancel, the device that the request will be assigned to will also be randomly generated/selected.
    This gives us data on the accuracy of the policy when collecting different percentages of cancellation.
*/
void baseline_random_cancel_execute_op(TraceOp &trace_op, Trace *trace, uint32_t device, char* buf) {
    int ret;
    int *fds = trace->get_fds();
    //read
    if(trace_op.op == 0) {
        baseline_total_count++;
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        int rand_int = distribution(gen); // Generate a random number
        if (1 <= rand_int && rand_int <= 5) { // Decides whether ot not to cancel, change the 5 to 10 or 50 to edit percentages
            baseline_random_cancel_count++;
            int random = distribution2(gen2); // Randomly picks a device to send request to
            if (random == 1) {
                device = (--device + 3) % 3;
            } else {
                device = ++device % 3;
            }
        }
        trace->add_io_count(device);
        ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
    } 
    else if(trace_op.op == 1) {
        trace->add_io_count(device);
        ret = pwrite(fds[device], buf, trace_op.size, trace_op.offset);
    } else {
        printf("Wrong OP code! %d\n", trace_op.op);
    }

    if (ret < 0){
        printf("err %d\n", errno);
        printf("offset in B : %lu\n", trace_op.offset );
        printf("size in B : %lu\n", trace_op.size);
    }
}

void failover_execute_op(TraceOp &trace_op, Trace *trace, uint32_t device, char* buf) {
    int ret, i;
    int *fds = trace->get_fds();
    bool success = false;
    //read
    if(trace_op.op == 0) {
        trace->add_io_count(device);
        ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
        if (ret < 0) {
            trace->add_fail(device);
            for (i = 0 ; i < MAX_FAIL ; i++) {
                device = ++device % 3;
                trace->add_io_count(device);
                ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
                if (ret > 0) {
                    success = true;
                    break;
                }
                trace->add_fail(device);
            }
            if (!success) {
                trace->add_unique_fail(device);
                pread(fds[device], buf, trace_op.size, 0); //this is what linnos does
            }
        }
    } else if(trace_op.op == 1) {
        trace->add_io_count(device);
        ret = pwrite(fds[device], buf, trace_op.size, trace_op.offset);
    } else {
        printf("Wrong OP code! %d\n", trace_op.op);
    }
}

/*
    This method randomly cancels the request for LAKE, as specified by the rand_int generated. After selecting
    whether or not to cancel, the device that the request will be assigned to will also be randomly generated/selected.
    This gives us data on the accuracy of the policy when collecting different percentages of cancellation.
*/
void failover_random_cancel_execute_op(TraceOp &trace_op, Trace *trace, uint32_t device, char* buf) {
    int ret, i;
    int *fds = trace->get_fds();
    bool success = false;
    bool random_cancel;

    //read
    if(trace_op.op == 0) {
        total_count++;
        trace->add_io_count(device);
        ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
        if (ret < 0) {
            trace->add_fail(device);
            for (i = 0 ; i < MAX_FAIL ; i++) {
                std::srand(static_cast<unsigned int>(std::time(nullptr)));
                int rand_int = distribution(gen); // Generate a random number
                if (1 <= rand_int && rand_int <= 5) { // Decides whether ot not to cancel, change the 5 to 10 or 50 to edit percentages
                    random_cancel_count++;
                    random_cancel = true;
                    device = (--device + 3) % 3;
                } else {
                    random_cancel = false;
                    device = ++device % 3;
                }
                trace->add_io_count(device);
                ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
                if (random_cancel == true) {
                    if (ret < 0) {
                        success = false;
                    } else {
                        success = true;
                    }
                    break;
                }
                if (ret > 0) {
                    success = true;
                    break;
                }
                trace->add_fail(device);
            }
            if (!success) {
                trace->add_unique_fail(device);
                pread(fds[device], buf, trace_op.size, 0); //this is what linnos does
            }
        }
    } else if(trace_op.op == 1) {
        trace->add_io_count(device);
        ret = pwrite(fds[device], buf, trace_op.size, trace_op.offset);
    } else {
        printf("Wrong OP code! %d\n", trace_op.op);
    }
}

void strawman_execute_op(TraceOp &trace_op, Trace *trace, uint32_t device, char* buf) {
    int ret;
    int *fds = trace->get_fds();
    //read
    if(trace_op.op == 0) {
        trace->add_io_count(device);
        ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
        //rejected, go to next device (it should not have linnos enabled)
        if (ret < 0) {
            trace->add_fail(device);
            trace->add_unique_fail(device);
            trace->add_io_count(device+1);
            ret = pread(fds[1], buf, trace_op.size, trace_op.offset);
            if (ret < 0) {
                printf("Second IO failed, this shouldn't happen! err %d\n", errno);
                trace->add_never_finished(device);
            }
        }
    } else if(trace_op.op == 1) {
        trace->add_io_count(device);
        ret = pwrite(fds[device], buf, trace_op.size, trace_op.offset);
    } else {
        printf("Wrong OP code! %d\n", trace_op.op);
    }
}

void strawman_2ssds_execute_op(TraceOp &trace_op, Trace *trace, uint32_t device, char* buf) {
    int ret;
    int *fds = trace->get_fds();
    //read
    if(trace_op.op == 0) {
        trace->add_io_count(device);
        ret = pread(fds[device], buf, trace_op.size, trace_op.offset);
        //rejected, go to next device (it should not have linnos enabled)
        if (ret < 0) {
            trace->add_fail(device);
            trace->add_unique_fail(device);
            trace->add_io_count(2);
            ret = pread(fds[2], buf, trace_op.size, trace_op.offset);
            if (ret < 0) { 
                printf("Second IO failed, this shouldn't happen! err %d\n", ret);
                trace->add_never_finished(device);
            }
        }
    } else if(trace_op.op == 1) {
        trace->add_io_count(device);
        ret = pwrite(fds[device], buf, trace_op.size, trace_op.offset);
    } else {
        printf("Wrong OP code! %d\n", trace_op.op);
    }
}


void* replayer_fn(void* arg) {
    Thread_arg *targ = (Thread_arg*) arg;
    Trace *trace = targ->trace;
    uint32_t device = targ->device;
    TraceOp trace_op;
    char *buf;

    if (posix_memalign((void**)&buf, MEM_ALIGN, LARGEST_REQUEST_BYTES)) {
        fprintf(stderr, "memory allocation failed\n");
        exit(1);
    }

    //start together
    pthread_barrier_wait(targ->sync_barrier);
    int is_late;
    //add variable to see if ml_enabled has changed
    while (1) {
        trace_op = trace->get_line(device);
        if (trace_op.timestamp == -1) {
            break;
        }
        //timestamp in op is in microsecond float, so convert to nano
        uint64_t next = targ->start_ts + (uint64_t)(trace_op.timestamp*1000);
        if(sleep_until(next) == 1)
            trace->add_late_op(device);

        uint64_t submission = get_ns_ts();
        auto begin = std::chrono::steady_clock::now();
        //realize trace_op
        targ->executor(trace_op, trace, targ->device, buf);
        auto end = std::chrono::steady_clock::now();
        uint32_t elaps =  std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        uint64_t end_ts = get_ns_ts();

        //print out whether it's changed or not within the loop

        //store results
        trace->write_output_line(end_ts/1000, elaps, trace_op.op,
                trace_op.size, trace_op.offset, submission/1000,
                device);
    }
    free(buf);
    return 0;
}


