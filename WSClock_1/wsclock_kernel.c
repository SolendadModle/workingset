#include "wsclock_kernel.h"

/*
 * 新增：在一次扫描中允许写回的最大页面数
 * 这个限制可以放在环境或进程级别，根据实际需求设计
 */
static const int maxWritesPerScan = 2;
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

    proc->clock++; /* 模拟进程时钟 */

    Page* page = &proc->page_table[page_to_access];
    if (page->in_working_set) {
        /* 已在工作集中：更新引用位、时间戳 */
        page->referenced = 1;
        page->age = proc->clock;
    } else {
        /* 缺页 */
        log_msg(env, "Page fault occurred; checking for victim page...");

        /* 工作集是否已满 */
        int inWS = 0;
        for (int i = 0; i < proc->page_count; i++) {
            if (proc->page_table[i].in_working_set) {
                inWS++;
            }
        }
        if (inWS >= proc->working_set_size) {
            Page* victim = find_victim_page(proc);
            if (victim) {
                /* 释放被替换页面 */
                victim->in_working_set = 0;
                victim->referenced = 0;
                victim->age = 0;
                victim->modified = 0;
            }
        }

        /* 将新页面加入工作集 */
        page->in_working_set = 1;
        page->referenced = 1;
        page->modified = 0;
        page->age = proc->clock;
    }
}

/*
 * 改进后的 victim 选择逻辑
 *  - 采用时钟指针扫描
 *  - 如果 R=1 => 置 R=0 => pointer++ => 跳过
 *  - 如果 R=0 => 检查“页面是否足够老”且“是否干净”
 *       干净 => 立即回收
 *       脏 => 如果没有超过写回限制，则安排写回，否则跳过
 */
static Page* find_victim_page(Process* proc)
{
    /* 需要一个指针记录当前扫描位置；可以在Process结构中加字段 clockHand */
    static int clockHand = 0;  /* 为简化，这里使用静态变量模拟单个进程指针 */
    int scanCount = 0;         /* 防止无限循环 */
    int writesThisRound = 0;   /* 跟踪本轮写回的次数 */

    int totalPages = proc->page_count;
    if (clockHand >= totalPages) {
        clockHand = 0;
    }

    while (scanCount < totalPages) {
        Page* currentPage = &proc->page_table[clockHand];
        if (currentPage->in_working_set) {
            if (currentPage->referenced == 1) {
                /* 最近使用过 => R=1 => 清零并跳过 */
                currentPage->referenced = 0;
                clockHand = (clockHand + 1) % totalPages;
                scanCount++;
                continue;
            } else {
                /* R=0 => 判断页面是否足够老 */
                unsigned long ageGap = proc->clock - currentPage->age;
                /* 这里可用自定义阈值, 例如 WSClock 论文中的 tau */
                unsigned long oldThreshold = 5; 
                if (ageGap >= oldThreshold) {
                    /* 页面老化 */
                    if (currentPage->modified == 0) {
                        /* 干净 => 可回收 */
                        return currentPage;
                    } else {
                        /* 脏 => 判断是否还有写回配额 */
                        if (writesThisRound < maxWritesPerScan) {
                            /*
                             * 安排写回，这里仅简单演示，实际需要异步IO或队列
                             * 写回后可立即回收，也可能需要短暂时间
                             */
                            writesThisRound++;
                            /* 这里将modified清0表示已写回 */
                            currentPage->modified = 0;
                            /* 回收该页面 */
                            return currentPage;
                        } else {
                            /* 达到写回上限 => 暂不回收, 指针继续前移 */
                            clockHand = (clockHand + 1) % totalPages;
                            scanCount++;
                            continue;
                        }
                    }
                } else {
                    /* 没到老化时间 => 继续 */
                    clockHand = (clockHand + 1) % totalPages;
                    scanCount++;
                    continue;
                }
            }
        } else {
            /* 不在工作集 => 跳过 */
            clockHand = (clockHand + 1) % totalPages;
            scanCount++;
            continue;
        }
    }

    /* 如果扫描一圈都没找到可替换页面, 返回NULL */
    /* 这在实际中很特殊，说明工作集中页面都在被频繁访问或等待写回 */
    return 0;
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

void wsclock_periodic_scan(WSClockEnvironment* env, int process_index)
{
    /* 原有的引用位清理逻辑，可保留或根据需要更新 */
    if (!env || process_index < 0 || process_index >= env->process_count) {
        return;
    }
    Process* proc = &env->processes[process_index];
    if (!proc->active) {
        return;
    }
    for (int i = 0; i < proc->page_count; i++) {
        if (proc->page_table[i].in_working_set) {
            proc->page_table[i].referenced = 0;
        }
    }
}

/*
 * 释放WSClock环境(可根据项目实际需要做处理)
 */
void wsclock_cleanup(WSClockEnvironment* env)
{
    /* 不做额外处理 */
}