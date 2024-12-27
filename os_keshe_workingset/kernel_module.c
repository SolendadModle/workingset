#include "kernel_module.h"

/* 静态的全局数组，用于存储系统中的进程信息 */
static ProcessControlBlock g_processTable[MAX_PROCESSES];
static int g_processCount = 0;

/* 
 * 内核初始化：
 *   - 清空进程表
 */
void Kernel_Init(void)
{
    int i, j;
    g_processCount = 0;
    for (i = 0; i < MAX_PROCESSES; i++) {
        g_processTable[i].ws.processId = -1;  /* 表示无效进程 */
        g_processTable[i].ws.pageCount = 0;
        g_processTable[i].ws.workingSetSize = 0;
        for (j = 0; j < MAX_PAGES; j++) {
            g_processTable[i].ws.pages[j].pageId = j;
            g_processTable[i].ws.pages[j].inWorkingSet = 0;
        }
    }
}

/* 
 * 创建一个进程并初始化其工作集
 */
int Kernel_CreateProcess(int processId, int maxPages, int workingSetSize)
{
    int i, j;

    if (g_processCount >= MAX_PROCESSES) {
        /* 达到最大进程数，无法再创建 */
        return -1;
    }

    /* 找到一个空闲的进程槽 */
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (g_processTable[i].ws.processId == -1) {
            g_processTable[i].ws.processId = processId;
            g_processTable[i].ws.pageCount = (maxPages > MAX_PAGES) ? MAX_PAGES : maxPages;
            g_processTable[i].ws.workingSetSize = workingSetSize > 0 ? workingSetSize : 1;
            
            /* 将所有页初始设置为不在工作集里 */
            for (j = 0; j < g_processTable[i].ws.pageCount; j++) {
                g_processTable[i].ws.pages[j].pageId = j;
                g_processTable[i].ws.pages[j].inWorkingSet = 0;
            }

            g_processCount++;
            return 0;
        }
    }

    return -1; /* 未找到空闲槽 */
}

/*
 * 引用某个进程的某个页面，更新其在工作集中的标记
 */
int Kernel_ReferencePage(int processId, int pageId)
{
    int i;
    ProcessControlBlock *pcb = 0;

    /* 查找目标进程 */
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (g_processTable[i].ws.processId == processId) {
            pcb = &g_processTable[i];
            break;
        }
    }

    if (!pcb) {
        /* 未找到该进程 */
        return -1;
    }

    if (pageId < 0 || pageId >= pcb->ws.pageCount) {
        /* 数据非法 */
        return -1;
    }

    /* 简单标记为引用，后续 Kernel_UpdateWorkingSets 决定其是否停留在工作集中 */
    pcb->ws.pages[pageId].inWorkingSet = 1;

    return 0;
}

/*
 * 简单地更新工作集：若页面被引用则置1，若超过工作集大小则置0。
 * 这是演示性的算法，以固定大小的窗口 (workingSetSize) 来控制工作集中能保留多少“最新”引用。
 */
void Kernel_UpdateWorkingSets(void)
{
    int i, j, count;
    ProcessControlBlock *pcb;

    for (i = 0; i < MAX_PROCESSES; i++) {
        pcb = &g_processTable[i];
        if (pcb->ws.processId == -1) {
            continue; /* 无效进程 */
        }

        /* 计数在工作集中的页面个数 */
        count = 0;
        for (j = 0; j < pcb->ws.pageCount; j++) {
            if (pcb->ws.pages[j].inWorkingSet) {
                count++;
            }
        }

        /* 若页面数超过了工作集大小，则随机或按一定策略移除一些页面 */
        /* 这里仅展示一个示例策略：从头开始清除多余的页 */
        if (count > pcb->ws.workingSetSize) {
            int toRemove = count - pcb->ws.workingSetSize;
            for (j = 0; j < pcb->ws.pageCount && toRemove > 0; j++) {
                if (pcb->ws.pages[j].inWorkingSet) {
                    pcb->ws.pages[j].inWorkingSet = 0;
                    toRemove--;
                }
            }
        }
    }
}

/* 获取进程表首地址 */
ProcessControlBlock* Kernel_GetProcessTable(void)
{
    return g_processTable;
}

/* 获取当前进程数量 */
int Kernel_GetProcessCount(void)
{
    return g_processCount;
}