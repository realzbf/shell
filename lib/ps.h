/*************************************************************************
	> File Name: ps.h
	> Author: 
	> Mail: 
	> Created Time: 2019年05月25日 星期六 12时36分24秒
 ************************************************************************/

#ifndef _PS_H
#define _PS_H

#define SUCCESS 0
#define PROCESS_INFO_PATH "/proc/"


typedef struct proc_t {

    int
        pid,		/* 进程id */
    	ppid;		/* 父进程id */
    unsigned
        pcpu;           /* cpu使用率 */
    char
    	state,		/* 进程状态 */
    	pad_1,		/* padding */
    	pad_2,		/* padding */
    	pad_3;		/* padding */

    unsigned long long
	utime,		/* 用户态时间 */
	stime,		/* 内核态时间 */
// and so on...
	cutime,		/* 子进程用户态时间 */
	cstime,		/* 子进程内核态时间 */
	starttime;	/* 开始时间 */
    long
	priority,	/* kernel scheduling priority */
	timeout,	/* ? */
	nice,		/* standard unix nice level of process */
	rss,		/* resident set size from /proc/#/stat (pages) */
	it_real_value,	/* ? */
    /* the next 7 members come from /proc/#/statm */
	size,		/* total # of pages of memory */
	resident,	/* number of resident set (non-swapped) pages (4k) */
	share,		/* number of pages of shared (mmap'd) memory */
	trs,		/* text resident set size */
	lrs,		/* shared-lib resident set size */
	drs,		/* data resident set size */
	dt;		/* dirty pages */
    unsigned long
	vm_size,        /* same as vsize in kb */
	vm_lock,        /* locked pages in kb */
	vm_rss,         /* same as rss in kb */
	vm_data,        /* data size */
	vm_stack,       /* stack size */
	vm_exe,         /* executable size */
	vm_lib,         /* library size (all pages, not just used ones) */
	rtprio,		/* real-time priority */
	sched,		/* scheduling class */
	vsize,		/* number of pages of virtual memory ... */
	rss_rlim,	/* resident set size limit? */
	flags,		/* kernel flags for the process */
	min_flt,	/* number of minor page faults since process start */
	maj_flt,	/* number of major page faults since process start */
	cmin_flt,	/* cumulative min_flt of process and child processes */
	cmaj_flt,	/* cumulative maj_flt of process and child processes */
	nswap,		/* ? */
	cnswap,		/* cumulative nswap ? */
	start_code,	/* address of beginning of code segment */
	end_code,	/* address of end of code segment */
	start_stack,	/* address of the bottom of stack for the process */
	kstk_esp,	/* kernel stack pointer */
	kstk_eip,	/* kernel instruction pointer */
	wchan;/* address of kernel wait channel proc is sleeping in */
    char
	**environ,	/* environment string vector (/proc/#/environ) */
	**cmdline;	/* command line string vector (/proc/#/cmdline) */
    char
	/* Be compatible: Digital allows 16 and NT allows 14 ??? */
        comm[32],
    	ruser[16],	/* real user name */
    	euser[16],	/* effective user name */
    	suser[16],	/* saved user name */
    	fuser[16],	/* filesystem user name */
    	rgroup[16],	/* real group name */
    	egroup[16],	/* effective group name */
    	sgroup[16],	/* saved group name */
    	fgroup[16],	/* filesystem group name */
    	cmd[16];	/* basename of executable file in call to exec(2) */
    int
        ruid, rgid,     /* real      */
        euid, egid,     /* effective */
        suid, sgid,     /* saved     */
        fuid, fgid,     /* fs (used for file access only) */
	pgrp,		/* process group id */
	session,	/* session id */
	tty,		/* full device number of controlling terminal */
	tpgid,		/* terminal process group id */
	exit_signal,	/* might not be SIGCHLD */
	processor;      /* current (or most recent?) CPU */
#ifdef FLASK_LINUX
    security_id_t sid;
#endif
} proc_t;


int pr_pcpu(proc_t *, char *);
int pr_pmem(proc_t *, char *);
int pr_command(proc_t *, char *);
int pr_vsz(proc_t *, char *);


int is_num(char *);
int pr_stat(proc_t *);
proc_t *pr_info(char *);
void output();
int uptime(double *restrict, double *restrict);

void pr_show(proc_t *, int *, int);
#endif
