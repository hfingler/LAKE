#ifdef __KERNEL__
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/vmalloc.h>
#include <asm/fpu/api.h>
#include "cuda.h"
#include "lake_shm.h"
//uspace
#else
#define kava_free(X) free(X)
#define kava_alloc(X) malloc(X)
#define vfree(X) free(X)
#define vmalloc(X) malloc(X)
#include <stdint.h>
#define u64 uint64_t
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#define usleep_range(X,Y) sleep(X/1000)
#include <sys/time.h>
#include <sys/random.h>
u64 get_tsns() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return current_time.tv_sec*1000000000 + current_time.tv_usec*1000;
}
#define ktime_get_ns() get_tsns()
#include <stdbool.h>
#endif

#define FEAT_31
#include "weights.h"
#include "helpers.h"
#include "predictors.h"
#include "variables.h"

//#include <asm/fpu/api.h>
#define LEN_INPUT 31
#define LEN_LAYER_0 256
#define LEN_LAYER_0_HALF 128
#define LEN_LAYER_1 2

#define RUNS 3
bool printResults = true; 

static char *cubin_path = "linnos.cubin";
#ifdef __KERNEL__
module_param(cubin_path, charp, 0444);
MODULE_PARM_DESC(cubin_path, "The path to linnos.cubin, default ./linnos.cubin");
#endif

long *test_weights[4] = { weight_0_T, weight_1_T, bias_0, bias_1};

static int run_cpu(void) {
    return 0;
}

// static inline void check_malloc(void *p, const char* error_str, int line) {
//     #ifdef __KERNEL__
// 	if (p == NULL) printk(KERN_ERR "ERROR: Failed to allocate %s (line %d)\n", error_str, line);
//     #else
//     if (p == NULL) printf("ERROR: Failed to allocate %s (line %d)\n", error_str, line);
//     #endif
// }

//CUdeviceptr d_weight_0_T_ent, d_weight_1_T_ent, d_bias_0_ent, d_bias_1_ent, d_input_vec_i, d_mid_res_i, d_final_res_i;
static long *final_res_i;

static void setup_gpu_memory(int batch_size) {
    static long *weight_0_T_ent, * bias_0_ent, *weight_1_T_ent, * bias_1_ent; 
    weight_0_T_ent = weight_0_T;
    weight_1_T_ent = weight_1_T;
    bias_0_ent = bias_0;
    bias_1_ent = bias_1;

    long *kbuf_weight_0_T_ent = (long*) kava_alloc(256*31*sizeof(long));
    memcpy(kbuf_weight_0_T_ent, weight_0_T_ent, 256*31*sizeof(long));

    long *kbuf_weight_1_T_ent = (long*) kava_alloc(256*2*sizeof(long));
    memcpy(kbuf_weight_1_T_ent, weight_1_T_ent, 256*2*sizeof(long));

    long *kbuf_bias_0_ent = (long*) kava_alloc(256*sizeof(long));
    memcpy(kbuf_bias_0_ent, bias_0_ent, 256*sizeof(long));

    long *kbuf_bias_1_ent = (long*) kava_alloc(2*sizeof(long));
    memcpy(kbuf_bias_1_ent, bias_1_ent, 2*sizeof(long));
	
	check_error(cuMemAlloc((CUdeviceptr*) &d_weight_0_T_ent, sizeof(long) * 256*31), "cuMemAlloc ", __LINE__);
    check_error(cuMemAlloc((CUdeviceptr*) &d_weight_1_T_ent, sizeof(long) * 256*2), "cuMemAlloc ", __LINE__);
    check_error(cuMemAlloc((CUdeviceptr*) &d_bias_0_ent, sizeof(long) * 256), "cuMemAlloc ", __LINE__);
    check_error(cuMemAlloc((CUdeviceptr*) &d_bias_1_ent, sizeof(long) * 2), "cuMemAlloc ", __LINE__);
    
    check_error(cuMemAlloc((CUdeviceptr*) &d_input_vec_i, sizeof(long) * 31 * batch_size), "cuMemAlloc ", __LINE__);

    check_error(cuMemAlloc((CUdeviceptr*) &d_mid_res_i, sizeof(long) *LEN_LAYER_0 * batch_size), "cuMemAlloc ", __LINE__);
    check_error(cuMemAlloc((CUdeviceptr*) &d_final_res_i, sizeof(long) *LEN_LAYER_1 * batch_size *32), "cuMemAlloc ", __LINE__);

    check_error(cuMemcpyHtoD(d_weight_0_T_ent, kbuf_weight_0_T_ent, sizeof(long) * 256*31), "cuMemcpyHtoD", __LINE__);
	check_error(cuMemcpyHtoD(d_weight_1_T_ent, kbuf_weight_1_T_ent, sizeof(long) * 256*2), "cuMemcpyHtoD", __LINE__);
	check_error(cuMemcpyHtoD(d_bias_0_ent, kbuf_bias_0_ent, sizeof(long) * 256), "cuMemcpyHtoD", __LINE__);
	check_error(cuMemcpyHtoD(d_bias_1_ent, kbuf_bias_1_ent, sizeof(long) * 2), "cuMemcpyHtoD", __LINE__);

    kava_free(kbuf_weight_0_T_ent);
    kava_free(kbuf_weight_1_T_ent);
    kava_free(kbuf_bias_0_ent);
    kava_free(kbuf_bias_1_ent);

    // final_res_i = (long*) kava_alloc(batch_size*64*sizeof(long));
    // check_malloc(final_res_i, "check_malloc", __LINE__);
}

static long *parallel_input;
static bool *res;

static void flatten_input(int batch_size, long* input_vec_i) {
    int b, j;
	for(b = 0 ; b < batch_size; b++) {
		for(j = 0; j < 31; j++)
			parallel_input[ b*31 + j ] =  input_vec_i[j];
	}
}

static void flatten_input_test_output(int batch_size, long* input_vec_i) {
    int j;
    //long input_1[31] = {1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,0,0,0,0,9,0,0,0,9,0,0,0,9};
    long input_1[31] = {9,9,9,0,9,1,1,9,9,9,9,0,9,9,1,9,9,1,9,9,0,9,1,9,1,9,0,9,9,0,9};
    long input_2[31] = {9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9};
    long input_3[31] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	for(j = 0; j < 31; j++)
		parallel_input[ j ] = input_vec_i[j];
    for(j = 0; j < 31; j++)
		parallel_input[ 1*31 + j ] = input_1[j];
    for(j = 0; j < 31; j++)
		parallel_input[ 2*31 + j ] = input_2[j];
    for(j = 0; j < 31; j++)
		parallel_input[ 3*31 + j ] = input_3[j];
}

static void copy_batch_inputs(int batch_size) {
    check_error(cuMemcpyHtoDAsync(d_input_vec_i, parallel_input, sizeof(long) * 31 * batch_size, 0), "cuMemcpyHtoD", __LINE__);
}

static void cleanup(void) {
    kava_free(parallel_input);
    kava_free(res);
}

void clean_batch(void) {
	cuMemFree(d_input_vec_i);
	cuMemFree(d_weight_0_T_ent);
	cuMemFree(d_weight_1_T_ent);
	cuMemFree(d_bias_0_ent);
	cuMemFree(d_bias_1_ent);
	cuMemFree(d_mid_res_i);
	cuMemFree(d_final_res_i);
	kava_free(final_res_i);
}

int gpu_inference(CUfunction* cufunc1, CUfunction* cufunc2, int batch_size, int sync) {
    void *args[] = {
		&d_weight_0_T_ent, &d_bias_0_ent, &d_input_vec_i, &d_mid_res_i
	};

    check_error(cuLaunchKernel(*cufunc1, 
				batch_size, 1, 1,          //blocks
				256, 1, 1,   //threads per block
				0,   //shared mem
                NULL, args, NULL),
			"cuLaunchKernel", __LINE__);

    void *args1[] = {
		&d_weight_1_T_ent, &d_bias_1_ent, &d_mid_res_i, &d_final_res_i
	};

    int zg = sync == 0 ? 1 : 69; 

    check_error(cuLaunchKernel(*cufunc2, 
				batch_size, 1, zg,          //blocks
				64, 1, 1,   //threads per block
				0,   //shared mem
                NULL, args1, NULL),
			"cuLaunchKernel", __LINE__);

    return 0;
}

void get_result_batch(int batch_size) {
    int i;
    check_error(cuMemcpyDtoHAsync(final_res_i, d_final_res_i, sizeof(long) * 64 * batch_size, 0), "cuMemcpyDtoH", __LINE__);
	for(i = 0; i < batch_size; i++) {
		res[i] = final_res_i[i*64]>=(final_res_i[i *64 + 32])? false: true;
	}
}

void print_results(int batch_size) {
    int i;
    #ifdef __KERNEL__
        PRINT(V_INFO, "GPU batch_%d results,\n", batch_size);
        for(i = 0; i < batch_size; i++) {
            PRINT(V_INFO, "%d\n", res[i]);
        }
    #else
        printf("GPU batch_%d results\n", batch_size);
        for(i = 0; i < batch_size; i++) {
            printf("%d\n", res[i]);
        }
    #endif  
}

static int run_gpu(void) {
    int i, j;
    PRINT(V_INFO, "Starting\n");
    int batch_sizes[] = {512, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    int n_batches = 12;
    // n needs to be at least as large as the largest batch size
    const int n = 1024;
    
    int batch_size;
    long input[31] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,9,0,0,0,9,0,0,0,9};
    u64 t_start, t_stop, c_start, c_stop;
    u64* comp_run_times;
    u64* total_run_times;
    u64 avg, avg_total;
    u64 best, best_total;
  
    initialize_gpu(cubin_path, test_weights, 1);

    //CUfunction batch_linnos_final_layer_kernel, batch_linnos_mid_layer_kernel;

    // comp_run_times = (u64*) vmalloc(RUNS*sizeof(u64));
    // total_run_times = (u64*) vmalloc(RUNS*sizeof(u64));

    //flatten n inputs, which is enough for all batches
    // parallel_input = (long*) kava_alloc(n*31*sizeof(long));
    // check_malloc(parallel_input, "check_malloc", __LINE__);

    // flatten_input(n, input);

    // res = (bool*) kava_alloc(n*sizeof(bool));
    // check_malloc(res, "check_malloc", __LINE__);

    // for (i = 0 ; i < n_batches ; i++) {
    //     batch_size = batch_sizes[i];
    //     // setup is only run once per batch size (cuda mallocs)
    //     setup_gpu(batch_size);    
    //     // copy inputs to GPU each time we run
    //     copy_batch_inputs(batch_size);

    //     //warmup
    //     gpu_inference(&batch_linnos_mid_layer_kernel, &batch_linnos_final_layer_kernel, batch_size, 1);
    //     get_result_batch(batch_size);
    //     cuCtxSynchronize();
    //     usleep_range(250, 1000);
    
    //     for (j = 0 ; j < RUNS ; j++) {
    //         //PRINT(V_INFO, "Runing batch %d/%d for batch size %d\n", k+1, n/batch_size, batch_size);
    //         t_start = ktime_get_ns();
    //         copy_batch_inputs(batch_size);
    //         gpu_inference(&batch_linnos_mid_layer_kernel, &batch_linnos_final_layer_kernel, batch_size, 0);
    //         get_result_batch(batch_size);
    //         t_stop = ktime_get_ns();
            
    //         usleep_range(500, 2000);

    //         c_start = ktime_get_ns();
    //         gpu_inference(&batch_linnos_mid_layer_kernel, &batch_linnos_final_layer_kernel, batch_size, 1);
    //         c_stop = ktime_get_ns();
            
    //         usleep_range(500, 2000);
    //         comp_run_times[j] = (c_stop - c_start);
    //         total_run_times[j] = (t_stop - t_start);
	//     }

	//     avg = 0; avg_total = 0;
    //     best = 0; best_total = 0;
    //     for (j = 0 ; j < RUNS ; j++) {
    //         avg += comp_run_times[j];
    //         avg_total += total_run_times[j];
    //         if (best == 0 || comp_run_times[j] < best) best = comp_run_times[j];
    //         if (best_total == 0 || total_run_times[j] < best_total) best_total = total_run_times[j];
    //     }
    //     avg = avg / (1000*RUNS); avg_total = avg_total / (1000*RUNS);
    //     best = best / 1000; best_total = best_total / 1000;
    //     #ifdef __KERNEL__
    //         PRINT(V_INFO, "GPU batch_%d, %lld, %lld, %lld, %lld\n", batch_size, avg, avg_total, best, best_total);
    //     #else
    //         printf("GPU batch_%d, %ld, %ld, %ld, %ld\n", batch_size, avg, avg_total, best, best_total);
    //     #endif
    //     clean_batch();
	// }

    if(printResults) {
        int batch_size = 1;
        char input[4][31] = {
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,9,0,0,0,9,0,0,0,9},
            {9,9,9,0,9,1,1,9,9,9,9,0,9,9,1,9,9,1,9,9,0,9,1,9,1,9,0,9,9,0,9},
            {9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
        };
        unsigned  char uuid[31];
        setup_gpu_memory(1);
        for(int k = 0; k < 5; k++) {
            //setup_gpu(batch_size);
            // for(j = 0; j < 31; j++)
		    //     parallel_input[j] = input[k][j];    
            // copy_batch_inputs(batch_size);
            // gpu_inference(&batch_linnos_mid_layer_kernel, &batch_linnos_final_layer_kernel, batch_size, 1);
            // get_result_batch(batch_size);
            // cuCtxSynchronize();

            #ifdef __KERNEL__ 
                get_random_bytes(uuid, sizeof(uuid));
            #else
                getrandom(uuid, sizeof (uuid), 0);
            #endif
            int cpu_result = cpu_prediction_model(uuid, 1, test_weights);
            PRINT(V_INFO, "CPU prediction done!\n");
            bool* gpu_result = gpu_prediction_model(uuid, 1, test_weights);
    
            #ifdef __KERNEL__
                PRINT(V_INFO, "GPU results %d,\n", (int)gpu_result[0]);
                PRINT(V_INFO, "CPU results %d,\n", cpu_result);
                if ((int)gpu_result[0] == cpu_result) {
                    PRINT(V_INFO, "Equal! \n");
                } else {
                    PRINT(V_INFO, " Not Equal! \n");
                }
            #else
                printf("GPU results %d\n", (int)gpu_result[0]);
                printf("CPU results %d\n", cpu_result);
                if ((int)gpu_result[0] == cpu_result) {
                    printf("Equal! \n");
                } else {
                    printf("Not Equal :( \n");
                }
            #endif
            kava_free(gpu_result);
        }
        clean_batch();
    }
    //cleanup();
    // vfree(comp_run_times);
    // vfree(total_run_times);
    return 0;
}

#ifdef __KERNEL__

/**
 * Program main
 */
static int __init linnos_init(void)
{
	return run_gpu();
}

static void __exit linnos_fini(void)
{

}

module_init(linnos_init);
module_exit(linnos_fini);

MODULE_AUTHOR("Isha Tarte");
MODULE_DESCRIPTION("Kernel module of a linnos program in kava");
MODULE_LICENSE("GPL");
MODULE_VERSION(__stringify(1) "."
               __stringify(0) "."
               __stringify(0) "."
               "0");

#else

int main() {
    run_gpu();
    return 0;
}

#endif
