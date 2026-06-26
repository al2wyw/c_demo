//
// Created by 李扬 on 2026/6/26.
//
/**
 * 多线程版 loop_unroll：验证多核能否突破单核 DRAM 带宽瓶颈。
 * 运行：
 *   ./loop_mt_o3 [total_size] [num_threads] [test_id]
 *     total_size  : 数组总长度（默认 100000000，即 800MB long）
 *     num_threads : 线程数（默认 4）
 *     test_id     : -1=全部, 0=base, 1=test0, 2=test1, 3=test2, 4=test3（默认 -1）
 *
 * 设计：
 *   - 数组按线程数等分，每个线程处理 [tid*chunk, (tid+1)*chunk)
 *   - 每个线程内部独立计时 + printf，不汇总结果
 *   - 主线程在所有 join 完成后额外打印总耗时（墙钟）和总吞吐，这是单核 vs 多核加速比的核心对比指标
 *   - Linux 下绑核（pthread_setaffinity_np），macOS 下退化为不绑核
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <sched.h>
#endif

long long get_timestamp_ns();

#define OP +
#define data_type long
#define MAX_THREADS 64

/* ============================================================
 *  5 个测试函数：内层算法与 loop_unroll.c 完全一致，
 *  仅在签名上加 tid，并把 printf 改为带线程编号的格式。
 * ============================================================ */

static int base_mt(int tid, data_type* arr, int loop) {
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int i = 0; i < loop; i++) {
        x = x OP arr[i];
    }
    long long elapsed = get_timestamp_ns() - start;
    double gbps = (double)loop * sizeof(data_type) / (double)elapsed; /* ns→GB/s 自动消元 */
    printf("[T%d] base   ret:%ld, time: %lld ns, %.2f GB/s\n",
           tid, (long)x, elapsed, gbps);
    return 0;
}

static int test0_mt(int tid, data_type* arr, int loop) {
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int i = 0; i < loop; i += 4) {
        x = x OP arr[i];
        x = x OP arr[i + 1];
        x = x OP arr[i + 2];
        x = x OP arr[i + 3];
    }
    long long elapsed = get_timestamp_ns() - start;
    double gbps = (double)loop * sizeof(data_type) / (double)elapsed;
    printf("[T%d] test0  ret:%ld, time: %lld ns, %.2f GB/s\n",
           tid, (long)x, elapsed, gbps);
    return 0;
}

static int test1_mt(int tid, data_type* arr, int loop) { /* 累积变量变换 */
    long long start = get_timestamp_ns();
    data_type x = 0x1, y = 0x0, n = 0x0, m = 0x0;
    for (int i = 0; i < loop; i += 4) {
        x = x OP arr[i];
        y = y OP arr[i + 1];
        n = n OP arr[i + 2];
        m = m OP arr[i + 3];
    }
    x = x OP y;
    n = n OP m;
    x = x OP n;
    long long elapsed = get_timestamp_ns() - start;
    double gbps = (double)loop * sizeof(data_type) / (double)elapsed;
    printf("[T%d] test1  ret:%ld, time: %lld ns, %.2f GB/s\n",
           tid, (long)x, elapsed, gbps);
    return 0;
}

static int test2_mt(int tid, data_type* arr, int loop) {
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int i = 0; i < loop; i += 4) {
        x = x OP arr[i] OP arr[i + 1] OP arr[i + 2] OP arr[i + 3];
    }
    long long elapsed = get_timestamp_ns() - start;
    double gbps = (double)loop * sizeof(data_type) / (double)elapsed;
    printf("[T%d] test2  ret:%ld, time: %lld ns, %.2f GB/s\n",
           tid, (long)x, elapsed, gbps);
    return 0;
}

static int test3_mt(int tid, data_type* arr, int loop) { /* 重新结合变换 */
    long long start = get_timestamp_ns();
    data_type x = 0x1;
    for (int i = 0; i < loop; i += 4) {
        x = x OP ((arr[i] OP arr[i + 1]) OP (arr[i + 2] OP arr[i + 3]));
    }
    long long elapsed = get_timestamp_ns() - start;
    double gbps = (double)loop * sizeof(data_type) / (double)elapsed;
    printf("[T%d] test3  ret:%ld, time: %lld ns, %.2f GB/s\n",
           tid, (long)x, elapsed, gbps);
    return 0;
}

/* ============================================================
 *  线程入口
 * ============================================================ */
typedef struct {
    int tid;
    data_type* arr;
    int loop;
    int test_id;
} thread_arg_t;

static void* worker(void* p) {
    thread_arg_t* a = (thread_arg_t*)p;

#ifdef __linux__
    /* 把当前线程绑到第 tid 个 CPU 上，消除调度抖动 */
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(a->tid, &set);
    pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
#endif

    switch (a->test_id) {
        case 0: base_mt(a->tid, a->arr, a->loop);  break;
        case 1: test0_mt(a->tid, a->arr, a->loop); break;
        case 2: test1_mt(a->tid, a->arr, a->loop); break;
        case 3: test2_mt(a->tid, a->arr, a->loop); break;
        case 4: test3_mt(a->tid, a->arr, a->loop); break;
        default: break;
    }
    return NULL;
}

/* ============================================================
 *  跑一个 test：创建 N 线程 → join → 打印总耗时
 * ============================================================ */
static void run_one_test(const char* name, int test_id,
                         data_type* arr, int total, int nthreads) {
    pthread_t tids[MAX_THREADS];
    thread_arg_t args[MAX_THREADS];
    int chunk = total / nthreads;
    /* 4 路展开要求 chunk 是 4 的倍数 */
    chunk = (chunk / 4) * 4;

    printf("==================== %s (threads=%d, chunk=%d) ====================\n",
           name, nthreads, chunk);

    long long t0 = get_timestamp_ns();
    for (int i = 0; i < nthreads; i++) {
        args[i].tid     = i;
        args[i].arr     = arr + (size_t)i * chunk;
        args[i].loop    = chunk;
        args[i].test_id = test_id;
        if (pthread_create(&tids[i], NULL, worker, &args[i]) != 0) {
            printf("pthread_create failed at %d: %s\n", i, strerror(errno));
            exit(-1);
        }
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(tids[i], NULL);
    }
    long long total_ns = get_timestamp_ns() - t0;

    size_t total_bytes = (size_t)chunk * nthreads * sizeof(data_type);
    double agg_gbps = (double)total_bytes / (double)total_ns;
    printf("[TOTAL] %s wall: %lld ns, processed: %.1f MB, aggregate: %.2f GB/s\n\n",
           name, total_ns,
           (double)total_bytes / (1 << 20),
           agg_gbps);
}

/* ============================================================
 *  main
 * ============================================================ */
int main(int argc, char* argv[]) {
    int total    = (argc > 1) ? atoi(argv[1]) : 100000000;
    int nthreads = (argc > 2) ? atoi(argv[2]) : 4;
    int test_id  = (argc > 3) ? atoi(argv[3]) : -1;

    if (nthreads <= 0 || nthreads > MAX_THREADS) {
        printf("nthreads must be in [1, %d]\n", MAX_THREADS);
        return -1;
    }
    /* 让 total 能被 nthreads 整除，且分片是 4 的倍数 */
    int chunk = (total / nthreads / 4) * 4;
    total = chunk * nthreads;
    if (total <= 0) {
        printf("total too small\n");
        return -1;
    }

    data_type* arr = (data_type*)malloc((size_t)total * sizeof(data_type));
    if (arr == NULL) {
        printf("malloc failed: %s\n", strerror(errno));
        return -1;
    }
    for (int i = 0; i < total; i++) {
        arr[i] = rand() % total;
    }

    printf(">>> total=%d elements (%.1f MB), nthreads=%d, test_id=%d\n\n",
           total,
           (double)total * sizeof(data_type) / (1 << 20),
           nthreads, test_id);

    const char* names[] = {"base", "test0", "test1", "test2", "test3"};
    if (test_id == -1) {
        for (int t = 0; t < 5; t++) {
            run_one_test(names[t], t, arr, total, nthreads);
        }
    } else if (test_id >= 0 && test_id <= 4) {
        run_one_test(names[test_id], test_id, arr, total, nthreads);
    } else {
        printf("test_id must be -1 or in [0,4]\n");
    }

    free(arr);
    return 0;
}
