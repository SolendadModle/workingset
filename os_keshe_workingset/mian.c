#include <stdio.h>
#include <stdlib.h>
#include "kernel_module.h"

/* 
 * 示例演示代码：
 *  - 可使用标准I/O进行文件读写
 *  - 调用 kernel_module 提供的API 执行工作集相关操作
 */

int main(int argc, char* argv[])
{
    FILE *fp = NULL;
    int processId, pageId;
    int i, j;

    /* 1) 初始化内核 */
    Kernel_Init();

    /* 2) 创建演示进程，假设创建3个进程，每个进程可使用的最大页数不同，工作集大小也不同 */
    Kernel_CreateProcess(0, 10, 3);
    Kernel_CreateProcess(1, 12, 4);
    Kernel_CreateProcess(2, 8,  2);

    /*
     * 3) 从文件中读取引用序列，示例文件格式为:
     *    processId pageId
     *    processId pageId
     *    ...
     *   假设文件名为 "references.txt"
     */
    if (argc > 1) {
        fp = fopen(argv[1], "r");
    } else {
        fp = fopen("references.txt", "r");
    }

    if (fp == NULL) {
        printf("无法打开引用文件，请检查文件路径！\n");
        return 1;
    }

    printf("开始读取引用序列...\n");
    while (fscanf(fp, "%d %d", &processId, &pageId) == 2) {
        /* 引用进程的页面 */
        if (Kernel_ReferencePage(processId, pageId) == 0) {
            /* 每次成功引用后，调用更新接口更新工作集(也可批量调用) */
            Kernel_UpdateWorkingSets();
        }
    }
    fclose(fp);

    /* 
     * 4) 演示数据：打印每个进程的页面在工作集中的状态 
     */
    {
        ProcessControlBlock *table = Kernel_GetProcessTable();
        int processCount = Kernel_GetProcessCount();

        for (i = 0; i < processCount; i++) {
            if (table[i].ws.processId == -1) {
                continue; /* 无效进程 */
            }
            printf("进程 %d：\n", table[i].ws.processId);
            printf("  最大页数: %d\n", table[i].ws.pageCount);
            printf("  工作集大小: %d\n", table[i].ws.workingSetSize);
            printf("  页在工作集中的情况:\n    ");
            for (j = 0; j < table[i].ws.pageCount; j++) {
                if (table[i].ws.pages[j].inWorkingSet) {
                    printf("[P%d] ", j);
                }
            }
            printf("\n\n");
        }
    }

    printf("演示结束。\n");
    return 0;
}