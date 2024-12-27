#ifndef WSCLOCK_KERNEL_H
#define WSCLOCK_KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 回调函数指针，用于在不使用标准I/O时，向外部输出日志或调试信息
 */
typedef void (*WSClockLogCallback)(const char* msg);

/*
 * 页面结构：用最基本的字段表示引用位、修改位、时间戳等
 */
typedef struct Page {
    int page_id;
    int referenced;       /* 引用位(模拟) */
    int modified;         /* 模拟修改位 */
    unsigned long age;    /* 访问时间戳 */
    int in_working_set;   /* 是否为工作集成员 */
} Page;

/*
 * 进程结构：包含页表、工作集大小等信息
 */
typedef struct Process {
    int process_id;
    Page* page_table;
    int page_count;       /* 进程总页数 */
    int working_set_size; /* 工作集容量限制 */
    unsigned long clock;  /* 模拟进程内(或全局)时钟 */
    int active;           /* 是否处于可调度状态(模拟多进程管理) */
} Process;

/*
 * 整个WSClock环境：管理多个进程以及输出回调
 */
typedef struct WSClockEnvironment {
    Process* processes;
    int process_count;
    WSClockLogCallback logger;
} WSClockEnvironment;

/*
 * 初始化WSClock环境
 */
void wsclock_init(WSClockEnvironment* env, 
                  Process* processes,
                  int process_count,
                  WSClockLogCallback logger);

/*
 * 对指定进程访问page_to_access页，并根据需要触发WSClock置换
 */
void wsclock_access_page(WSClockEnvironment* env, 
                         int process_index, 
                         int page_to_access);

/*
 * 执行对目标进程的“周期性扫描/清理”操作，可与调度循环结合
 * 用于模拟WSClock中对工作集中页的周期检查
 */
void wsclock_periodic_scan(WSClockEnvironment* env, int process_index);

/*
 * 释放WSClock环境(示例中可简单处理)
 */
void wsclock_cleanup(WSClockEnvironment* env);

#ifdef __cplusplus
}
#endif

#endif /* WSCLOCK_KERNEL_H */