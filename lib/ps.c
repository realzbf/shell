#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/sysinfo.h>

#include <locale.h>

#include<errno.h>

#include "ps.h"


static char* columns[] = {
    "USER", // 用户
    "PID",// 进程id
    "\%CPU",// CPU占用比
    "%MEM",// 内存占用比
    "VSZ",// 虚拟的内存量 Kbytes
    "RSS",// 固定的内存量 Kbytes
    "TTY",// 终端号，与终端无关则为？
    "STAT",//状态 R、S、T、Z
    "COMMAND"// 运行指令
};

#define USER_COL    0
#define PID_COL     1
#define CPU_COL   2
#define MEM_COL     3
#define VSZ_COL     4
#define RSS_COL     5
#define TTY_COL     6
#define STAT_COL    7
#define COMMAND_COL 8

#define MAX_COL     9

#define COL_WIDTH   12
#define COMM_COL_WIDTH 40
/* 可选参数 */
char *args_available[] = {
    "--user",
    "--pid",
    "-u",
    "-p",
};

#define MAX_ARGS  4

#define USER_OPTION 0
#define U_OPTION 2
#define PID_OPTION 1
#define P_OPTION 3

#define print_error() fprintf(stderr, "bash: ps: %s\n", strerror(errno))

char *user = NULL;
int pid = -1;

int main(int argc,char **argv)
{
    DIR	*dirp;
    struct dirent *direntp;
    int selected[MAX_COL] = {0};
    int i, j;

    proc_t *p = NULL;

    for(i=0;i<MAX_COL+1;i++){
        selected[i]=i;
    }


    if(argc > 1){
        for(i=1;i<argc;i++){
            /* 寻找用户输入的选项 */
            for(j=0;j<MAX_ARGS;j++){
                if (strcmp(args_available[j], argv[i])==0)
                break;
            }
            if(j>MAX_ARGS) {
                printf("no such argument!\n");
                return SUCCESS;
            }

            switch(j){
                case USER_OPTION:
                case U_OPTION:
                user = argv[i + 1];
                break;

                case PID_OPTION:
                case P_OPTION:
                pid = atoi(argv[i + 1]);
                break;

                default:
                if ((i % 2) ==1) {
                    printf("error: garbage option\n");
                    return 0;
                }
            }
        }
    }

    /* 打印表头 */
    for(i=0;i<sizeof(selected)/sizeof(selected[0]);i++){
        if (i==USER_COL) printf("%-20s",columns[selected[i]]);
        else printf("%-12s", columns[selected[i]]);
    }
    printf("\n");

    /* 打开/proc/目录 */
    if((dirp = opendir(PROCESS_INFO_PATH)) == NULL){
        fprintf(stderr,"dir error %s\n", PROCESS_INFO_PATH);
    } else {    
        while((direntp=readdir(dirp)) != NULL) {
            /* 文件名是数字则该文件是属于进程的 */
            if((is_num(direntp->d_name))==0) {
                if(pid>0 && (pid - atoi(direntp->d_name)!=0)) continue;
                p = pr_info(direntp->d_name);
                if(p){
                    if (user && strcmp(p->ruser,user)!=0) continue;
                    pr_show(p, selected, sizeof(selected)/sizeof(selected[0]));

                }
            }
        }
    }
return 0;
}


void pr_show(proc_t *p, int *selected, int count)
{
    char outbuf[COL_WIDTH];
    char cmdline[COMM_COL_WIDTH];
    int i;
    unsigned long long total_time;


    for(i=0;i<count;i++){
        switch(*(selected+i)){
            case USER_COL:
            printf("%-20s", p->ruser);break;

            case PID_COL:
            printf("%-12d", p->pid);
            break;

            case CPU_COL:   
            if (pr_pcpu(p, outbuf) > 0) 
            printf("%-12s", outbuf);
            break;

            case MEM_COL:  
            if (pr_pmem(p, outbuf) > 0) printf("%-12s ", outbuf);
            break;

            case VSZ_COL:     
            if (pr_vsz(p, outbuf) > 0) printf("%-12s", outbuf);
            break;

            case RSS_COL:
            printf("%-12ld", p->rss*4);
            break;

            case TTY_COL:
            if (!p->tty) printf("%-12c", '?');
            else printf("%-12d", p->tty);
            break;

            case STAT_COL:    
            printf("%-12c", p->state);
            break;

            case COMMAND_COL: 
            if (pr_command(p, cmdline) > 0) printf("%-40s", cmdline);
            break;

        }

    }
    printf("\n");
}

int is_num(char *p_name)
{
    int i,len;
    len = strlen(p_name);
    if (len == 0) return -1;
    for(i = 0; i < len; i++)
    {
        if(p_name[i]<'0' || p_name[i]>'9')
        return -1;
    }
    return SUCCESS;
}


int pr_pcpu(proc_t *p, char* outbuf)
{
    FILE *fp;
    unsigned long uptime_used, uptime_free;
    unsigned long long total_time;
    unsigned long long hz;
    unsigned pcpu = 0;
    unsigned long long seconds;
    unsigned long long seconds_since_boot;

    
    fp = fopen("/proc/uptime", "r");
    fscanf(fp, "%lu %lu", &uptime_used, &uptime_free);

    /* cpu使用时间 */
    seconds_since_boot  = uptime_used;

    /* 进程使用cpu的时间 */
    total_time = p->utime + p->stime;


    /* 每秒的节拍数 */
    hz = sysconf(_SC_CLK_TCK);

    
    /* 程序启动以来经过的时间/秒 */
    seconds = (seconds_since_boot >= (p->starttime / hz)) ? (seconds_since_boot - (p->starttime / hz)) : 0;
    if(seconds) pcpu = (total_time * 1000ULL / hz) / seconds;
    return snprintf(outbuf, COL_WIDTH, "%u.%u", pcpu/10U, pcpu%10U);
}
int pr_pmem(proc_t *p, char* outbuf)
{
    FILE *fp;
    long page_size;
    unsigned long pmem = 0;
    unsigned long kb_main_total;

    fp = fopen("/proc/meminfo", "r");
    fscanf(fp, "MemTotal: %lu\n",&kb_main_total);
    pmem = (p->vm_rss * 1000UL) / kb_main_total;
    if (pmem > 999) pmem = 999;
    return snprintf(outbuf, COL_WIDTH, "%2u.%u", (unsigned)(pmem/10), (unsigned)(pmem%10));

}
int pr_stat(proc_t *p)
{
    FILE *fp = NULL;
    char path[32];
    int status;

    int processor,exit_code;
    unsigned int rt_priority, policy;
    unsigned long signal, blocked, sigignore, sigcatch;
    unsigned long guest_time, start_data, end_data,start_brk, arg_start,arg_end, env_start, env_end;
    long num_threads, cguest_time;
    unsigned long long delayacct_blkio_ticks;

    snprintf(path, sizeof(path), "%s%d/stat", PROCESS_INFO_PATH, p->pid);
    if (status<0){
        return status;
    }
    fp = fopen(path, "r");
    if (!fp){
        printf("path=%s\n", path);
        return -1;
    }
    status = fscanf(fp, "%d %s %c %d %d %d %d %d %lu %lu\
           %lu %lu %lu %llu %llu %lld %llu %ld %ld %ld\
           %ld %llu %lu %ld %lu %lu %lu %lu %lu %lu\
           %lu %lu %lu %lu %lu %lu %lu %d %d %u\
           %u %llu %lu %ld %lu %lu %lu %lu %lu %lu\
           %lu %d", 
           &p->pid, p->comm,&p->state, &p->ppid, &p->pgrp, &p->session, &p->tty, &p->tpgid, &p->flags, &p->min_flt,
           &p->cmin_flt, &p->maj_flt, &p->cmaj_flt, &p->utime, &p->stime, &p->cutime, &p->cstime, &p->priority, &p->nice, &num_threads,
           &p->it_real_value, &p->starttime, &p->vsize, &p->rss, &p->rss_rlim, &p->start_code, &p->end_code, &p->start_stack, &p->kstk_esp, &p->kstk_eip,
           &signal, &blocked, &sigignore, &sigcatch, &p->wchan, &p->nswap, &p->cnswap,&p->exit_signal, &p->processor, &rt_priority,
           &policy, &delayacct_blkio_ticks, &guest_time, &cguest_time, &start_data, &end_data,&start_brk,
           &arg_start,&arg_end, &env_start, &env_end, &exit_code); 
    
    fclose(fp);
    if (status == EOF){
        return 1;
    }

    p->vm_rss = p->rss * 4;
    return SUCCESS;
}

proc_t *pr_info(char *d_name)
{
    FILE *fp = NULL;
    char path[256];
    struct stat statbuf;
    struct passwd *psw;
    proc_t *p = NULL;
    int status = 0;

    p = (proc_t*)malloc(sizeof(proc_t));
    if (!p){
        print_error();
        return NULL;
    }

    p->pid = atoi(d_name);
    psw = (struct passwd*)malloc(sizeof(struct passwd));
    if (!psw){
        print_error();
        return NULL;
    }

    status = snprintf(path, sizeof(path), "%s%s", PROCESS_INFO_PATH, d_name);


    if (status < 0){
        print_error();
        return NULL;
    }


    chdir(PROCESS_INFO_PATH);

    /* 读取文件状态，获取进程的用户id、组id(这里暂时没用到)等信息 */
    if(stat(path, &statbuf) == -1) {
        print_error();
    } else {
        psw = getpwuid(statbuf.st_uid);
        /* 用户名过长将不输出用户名，而用uid代替 */
        if (psw&&(strlen(psw->pw_name)<=16)){
            if (!strcpy(p->ruser, psw->pw_name)){
                print_error();
                return NULL;
            }
        }else {
            status = snprintf(p->ruser, sizeof(p->ruser), "%d", statbuf.st_uid);
            if (status < 0){
                print_error();
                return NULL;
            }
        }
    }
    /* 读取/proc/[pid]/stat文件， 大部分进程信息来自于该文件 */
    if (pr_stat(p)) return NULL;
    return p;
}

int pr_vsz(proc_t *p, char *outbuf){
    return snprintf(outbuf, COL_WIDTH, "%lu", p->vsize/(unsigned long)1024);
}

int pr_command(proc_t *p, char *outbuf){
    int fd;
    char path[32];
    int n;
    size_t size;
    
    n = snprintf(path, sizeof(path), "%s%d/cmdline", PROCESS_INFO_PATH, p->pid);
    if (n < 0) {
        print_error();
        return n;
    }
    
    if ((fd = open(path, O_RDONLY)) == -1){
        print_error();
        return -1;
    }

    if ((size = read(fd, outbuf, COMM_COL_WIDTH)) < 0){
        print_error();
        return size;
    }

    if (size > 0) {
        outbuf[size] = '\0';
        return size;
    } else {
        return snprintf(outbuf, COMM_COL_WIDTH, "%s", p->comm);
    }
}

