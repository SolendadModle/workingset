#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wsclock_kernel.h"

/* 简单日志回调，用于演示打印 */
static void demo_log(const char* msg)
{
    if (msg) {
        printf("[WSClock LOG]: %s\n", msg);
    }
}

/* 读取某进程的页面访问序列(模拟或从文件加载) */
static int* load_page_sequence(const char* filename, int* out_length)
{
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("无法打开文件: %s\n", filename);
        *out_length = 0;
        return NULL;
    }
    int capacity = 128;
    int count = 0;
    int* arr = (int*)malloc(sizeof(int) * capacity);
    if (!arr) {
        fclose(fp);
        *out_length = 0;
        return NULL;
    }
    while (!feof(fp)) {
        int page_id = 0;
        if (fscanf(fp, "%d", &page_id) == 1) {
            if (count >= capacity) {
                capacity *= 2;
                arr = (int*)realloc(arr, sizeof(int)*capacity);
                if (!arr) {
                    fclose(fp);
                    *out_length = 0;
                    return NULL;
                }
            }
            arr[count++] = page_id;
        }
    }
    fclose(fp);
    *out_length = count;
    return arr;
}

int main()
{
    /* 假设系统中有3个进程 */
    int process_count = 3;
    Process* allProcs = (Process*)malloc(sizeof(Process)*process_count);

    /* 初始化各进程的页表与工作集大小 */
    for(int i=0; i<process_count; i++){
        allProcs[i].process_id = i;
        allProcs[i].page_count = 20;          /* 每个进程6页 */
        allProcs[i].working_set_size = 4;    /* 工作集容量3 */
        allProcs[i].clock = 0;
        allProcs[i].active = 1;             /* 激活状态 */
        allProcs[i].page_table = (Page*)malloc(sizeof(Page)*allProcs[i].page_count);

        for(int j=0; j<allProcs[i].page_count; j++){
            allProcs[i].page_table[j].page_id = j;
            allProcs[i].page_table[j].referenced = 0;
            allProcs[i].page_table[j].modified = 0;
            allProcs[i].page_table[j].age = 0;
            allProcs[i].page_table[j].in_working_set = 0;
        }
    }

    /* 初始化WSClock环境 */
    WSClockEnvironment env;
    memset(&env, 0, sizeof(WSClockEnvironment));
    wsclock_init(&env, allProcs, process_count, demo_log);

    /* 读取某个访问序列(可自定义多个文件对应多个进程) 
       或者统一使用一份序列，在调度循环中交替让不同进程访问 */
    int seq_length = 0;
    int* sequence = load_page_sequence("page_refs.txt", &seq_length);
    if(seq_length <= 0){
        printf("page_refs.txt 读取失败或无内容，使用内置模拟\n");
        seq_length = 6;
        sequence = (int*)malloc(sizeof(int)*seq_length);
        int dummy[6] = {0,1,2,4,3,5};
        memcpy(sequence, dummy, sizeof(dummy));
    }

    printf("开始调度，共有 %d 个进程，每个进程的工作集大小都为5。\n", process_count);

    /* 简单的轮转调度示例 */
    int current_proc = 0;
    for(int i=0; i<seq_length; i++){
        int page_id = sequence[i];
        printf("\n[调度] 让进程 %d 访问页面 %d\n", current_proc, page_id);
        wsclock_access_page(&env, current_proc, page_id);

        /* 显示工作集当前状况 */
        Process* p = &allProcs[current_proc];
        printf("  工作集：");
        for(int j=0; j<p->page_count; j++){
            if(p->page_table[j].in_working_set){
                printf("%d ", p->page_table[j].page_id);
            }
        }
        printf("\n");

        /* 每次访问后轮换到下一个进程 */
        current_proc = (current_proc + 1) % process_count;

        /* 每若干次(如5次访问)之后可以触发一次周期性扫描，模拟对引用位清零 */
        if ((i+1) % 5 == 0) {
            printf("[调度] 执行 periodic_scan...\n");
            for(int pi=0; pi<process_count; pi++){
                wsclock_periodic_scan(&env, pi);
            }
        }
    }

    /* 演示结束，打印所有进程的最终工作集情况 */
    printf("\n=== 所有进程的最终工作集情况 ===\n");
    for(int i=0; i<process_count; i++){
        Process* p = &allProcs[i];
        printf("进程 %d:\n  工作集：", i);
        for(int j=0; j<p->page_count; j++){
            if(p->page_table[j].in_working_set){
                printf("%d ", p->page_table[j].page_id);
            }
        }
        printf("\n");
    }

    /* 释放资源 */
    wsclock_cleanup(&env);

    for(int i=0; i<process_count; i++){
        if(allProcs[i].page_table){
            free(allProcs[i].page_table);
        }
    }
    free(allProcs);
    free(sequence);

    return 0;
}