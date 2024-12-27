#include "wsclock_kernel.h"

/* 内部函数声明 */
static void log_msg(WSClockEnvironment* env, const char* msg);
static Page* find_victim_page(Process* proc);

void wsclock_init(WSClockEnvironment* env, 
                  Process* processes,
                  int process_count,
                  WSClockLogCallback logger)
{
    if (!env || !processes) return;
    
    env->processes = processes;
    env->process_count = process_count;
    env->logger = logger;
}

/*
 * 访问某个页面：如果不在工作集，则进行置换；
 * 如果在工作集，则更新引用位、时间戳等
 */
void wsclock_access_page(WSClockEnvironment* env, 
                         int process_index, 
                         int page_to_access)
{
    if (!env || process_index < 0 || process_index >= env->process_count) {
        return;
    }

    Process* proc = &env->processes[process_index];
    if (!proc->active) {
        return; /* 如果进程不活跃，忽略访问 */
    }

    if (!proc->page_table || page_to_access < 0 || page_to_access >= proc->page_count) {
        return;
    }

    proc->clock++; /* 模拟进程时钟递增 */

    Page* page = &proc->page_table[page_to_access];
    if (page->in_working_set) {
        /* 已在工作集中：更新引用位、时间戳 */
        page->referenced = 1;
        page->age = proc->clock;
    } else {
        /* 缺页，记录 */
        log_msg(env, "Page fault occurred. Replacing a page if WS is full.");

        /* 检查工作集是否已满 */
        int i;
        int count_in_ws = 0;
        for (i = 0; i < proc->page_count; i++) {
            if (proc->page_table[i].in_working_set) {
                count_in_ws++;
            }
        }

        /* 工作集已满，需要置换 */
        if (count_in_ws >= proc->working_set_size) {
            Page* victim = find_victim_page(proc);
            if (victim) {
                victim->in_working_set = 0;
                victim->referenced = 0;
                victim->age = 0;
                victim->modified = 0;
            }
        }

        /* 将目标页加入工作集 */
        page->in_working_set = 1;
        page->referenced = 1;
        page->modified = 0; /* 本示例中不做写回处理 */
        page->age = proc->clock;
    }
}

/*
 * 简化的WSClock扫描：找一个可替换的页面(未被引用或最老)
 */
static Page* find_victim_page(Process* proc)
{
    Page* victim = 0;
    unsigned long min_age = (unsigned long)-1;

    /* 第一轮找 reference=0 中 age 最小的 */
    for (int i = 0; i < proc->page_count; i++) {
        if (!proc->page_table[i].in_working_set) 
            continue;
        if (proc->page_table[i].referenced == 0 && proc->page_table[i].age < min_age) {
            victim = &proc->page_table[i];
            min_age = proc->page_table[i].age;
        }
    }

    /* 如果找不到则找 age 最小的(即最先被访问的页面) */
    if (!victim) {
        for (int i = 0; i < proc->page_count; i++) {
            if (!proc->page_table[i].in_working_set) 
                continue;
            if (proc->page_table[i].age < min_age) {
                victim = &proc->page_table[i];
                min_age = proc->page_table[i].age;
            }
        }
    }

    return victim;
}

/*
 * 周期性清理函数：可以在调度循环中调用
 * 将被引用位(reference)清零，以模拟操作系统在一定时间间隔内“衰减”访问位
 */
void wsclock_periodic_scan(WSClockEnvironment* env, int process_index)
{
    if (!env || process_index < 0 || process_index >= env->process_count) {
        return;
    }
    Process* proc = &env->processes[process_index];
    if (!proc->active) {
        return;
    }
    /* 清理引用位 */
    for (int i = 0; i < proc->page_count; i++) {
        if (proc->page_table[i].in_working_set) {
            proc->page_table[i].referenced = 0;
        }
    }
}

/*
 * 简单日志输出
 */
static void log_msg(WSClockEnvironment* env, const char* msg)
{
    if (env && env->logger) {
        env->logger(msg);
    }
}

/*
 * 清理函数(示例中仅空实现)
 */
void wsclock_cleanup(WSClockEnvironment* env)
{
    
}