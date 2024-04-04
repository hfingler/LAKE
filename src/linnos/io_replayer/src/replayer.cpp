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


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <inttypes.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <pthread.h>

// added
#include <atomic>
#include <cinttypes>

#include "replayer.hpp"
#include "op_replayers.hpp"

uint8_t N_THREADS = 64;

int main (int argc, char **argv)
{
    if (argc < 4) {
        printf("Usage: ./replayer <baseline|failover> logfile <# of devices to trace (1,2,3)> /dev/tgt0-/dev/tgt1-/dev/tgt2 <n of devices traces>\n");
        exit(1);
    } 
    
    std::string metrics_file(argv[2]);
    std::string metrics_fname(argv[2]);
    std::string type(argv[1]);
    std::string devices_to_trace(argv[3]);
    int n_devices_to_trace = std::stoi(devices_to_trace);
    Trace trace(argv[4]);
    printf("Number devices %d\n", n_devices_to_trace);
    for (int i=0; i < n_devices_to_trace ; i++) {
        printf("parsing trace %d\n", i);
        trace.parse_file(i, argv[5+i]);
    }
    
    pthread_barrier_t sync_barrier;
    int err = pthread_barrier_init(&sync_barrier, NULL, n_devices_to_trace*N_THREADS+1);
    if (err != 0) {
        printf("Error creating barrier\n");
        exit(1);
    }

    pthread_t threads[n_devices_to_trace][N_THREADS];
    Thread_arg targs[n_devices_to_trace][N_THREADS];
    for (int dev=0; dev < n_devices_to_trace ; dev++) {
        for (int j = 0; j < N_THREADS; j++) {
            targs[dev][j].trace = &trace;
            targs[dev][j].device = dev;
            targs[dev][j].sync_barrier = &sync_barrier;
            if(type == "baseline") {
                targs[dev][j].executor = baseline_execute_op;
            } else if (type == "baseline_random_cancel") {
                targs[dev][j].executor = baseline_random_cancel_execute_op;
            } else if (type == "strawman") {
                targs[dev][j].executor = strawman_execute_op;
            } else if (type == "failover") {
                targs[dev][j].executor = failover_execute_op;
            } else if (type == "failover_random_cancel") {
                targs[dev][j].executor = failover_random_cancel_execute_op;
            } else if (type == "strawman2") {
                targs[dev][j].executor = strawman_2ssds_execute_op;
            } else {
                printf("I dont recognize type %s (second parameter)\n", type.c_str());
            }
            pthread_create(&threads[dev][j], NULL, replayer_fn, (void*)&targs[dev][j]);
        }
    }

    trace.set_output_file(metrics_file+"_"+type+".data");

    usleep(20); //wait until everyone hits barrier
    uint64_t now = get_ns_ts();
    //give threads most up do date starting time
    for (int dev=0; dev < n_devices_to_trace ; dev++) 
        for (int j = 0; j < N_THREADS; j++)
            targs[dev][j].start_ts = now;

    auto begin = std::chrono::steady_clock::now();
    //start workers
    pthread_barrier_wait(&sync_barrier);
    //wait for workers
    for (int dev=0; dev < n_devices_to_trace ; dev++) {
        for (int j = 0; j < N_THREADS; j++)
            pthread_join(threads[dev][j], 0);
    }
    auto end = std::chrono::steady_clock::now();
    uint64_t elaps =  std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();

    printf("Trace took %ld seconds to finish.\n", elaps);
    // printf("Total number of read operations in failover: %" PRIu64 "\n", total_count);
    // printf("Total number of randomly cancelled operations in failover: %" PRIu64 "\n", random_cancel_count);

    trace.print_stats();
    // printf("Trace has %ld requests\n", trace->get_io_count(device));

    return 0;
}