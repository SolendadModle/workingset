#ifndef KERNEL_MODULE_H
#define KERNEL_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * 数据结构及接口声明：
 *  - 不使用任何标准输入/输出函数
 *  - 不依赖STL容器或其他高层库
 *  - 主要提供“工作集”部分的内部实现
 */

#define MAX_PROCESSES 10   /* 最大支持的进程数 */
#define MAX_PAGES     256  /* 单个进程可用最大页数 */

/*
 * 描述单个页面的信息
 *  - pageId: 页编号
 *  - inWorkingSet: 是否在工作集中
 */
typedef struct {
    int pageId;
    int inWorkingSet;
} PageInfo;

/*
 * 工作集数据结构
 *  - processId: 进程ID
 *  - pageCount: 当前在使用的页总数
 *  - workingSetSize: 工作集可容纳的最大页数(可根据算法动态调整或设为固定)
 *  - pages[]: 存储此进程所有页面的在工作集中的状态
 */
typedef struct {
    int processId;
    int pageCount;
    int workingSetSize;
    PageInfo pages[MAX_PAGES];
} WorkingSet;

/*
 * 进程控制块(PCB)示例结构，仅包含工作集信息，用于模拟多进程执行
 *  - ws: 工作集信息
 */
typedef struct {
    WorkingSet ws;
    /* 这里可以根据需要扩展PCB信息，如调度信息、寄存器状态等 */
} ProcessControlBlock;

/* 
 * kernel_module 初始化接口：
 *   - 初始化内核数据结构，如进程列表。
 */
void Kernel_Init(void);

/*
 * 创建一个新进程，分配进程控制块并初始化其工作集。
 * 参数:
 *   - processId: 进程ID
 *   - maxPages: 此进程使用的最大页数
 *   - workingSetSize: 该进程的工作集大小
 * 返回值:
 *   - 0: 成功
 *   - -1: 失败(例如进程过多)
 */
int Kernel_CreateProcess(int processId, int maxPages, int workingSetSize);

/*
 * 根据“页面引用”更新工作集。
 * 参数:
 *   - processId: 引用页面的进程
 *   - pageId: 引用的页面
 * 返回值:
 *   - 0: 成功
 *   - -1: 对应进程或页面不存在
 */
int Kernel_ReferencePage(int processId, int pageId);

/*
 * 计算各进程工作集的状态（可根据一定策略进行调整）。
 * 这里以固定工作集大小方式为例，仅根据是否被引用而在/不在工作集中切换。
 */
void Kernel_UpdateWorkingSets(void);

/*
 * 获取内核中的进程控制块，用于演示读取状态。
 * 返回值:
 *   - ProcessControlBlock* 数组指针
 */
ProcessControlBlock* Kernel_GetProcessTable(void);

/*
 * 获取当前有效进程数量
 */
int Kernel_GetProcessCount(void);

#ifdef __cplusplus
}
#endif

#endif /* KERNEL_MODULE_H */