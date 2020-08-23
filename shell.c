#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h>
#include<netdb.h>
#include<sys/types.h> 
#include<sys/wait.h>

#include<readline/readline.h> 
#include<readline/history.h>

#include<dirent.h>

#include<errno.h>

#include<sys/mman.h>

#define MAX_INPUT_STRING 1000 /* 能输入的最长字符串长度 */ 
#define MAX_ARGUMENT_NUM 100 /* 参数最多个数 */

#define COMMAND_BUILTIN_NUM 2 /* 内置命令个数 */
#define EXTERNAL_COMMAND_PATH "/home/hdubf/linux-learning/e3-1/bin/"/* 内置命令路径 */

#define SUCCESS 0

#define MAX_DIRECTORY_HISTORY 100
#define MAX_SHARED_MEM_SIZE 12800
static char *command_builtin[COMMAND_BUILTIN_NUM]={
    "exit",
    "cd"
};


#define NOT_BUILTIN_COMMAND -5
#define OPEN_DIR_ERROR      -2
#define EXIT_COMMAND         0
#define CD_COMMAND           1
#define ARGUMENTS_ERROR     -1

/******************************************
* 清屏 
* \033[H:将光标移至左上角
* \033[J:屏幕的一部分从光标清除到屏幕的末尾
******************************************/
#define clear() printf("\033[H\033[J") 
#define print_error(str) fprintf(stderr, "%s%s\n", str, strerror(errno))
#define clear_string(str) memset(str, 0, sizeof(str))

void* shared_memory = NULL;


/* 创建共享内存 */
void* create_shared_memory(size_t size) {

  /* 可读可写 */
  int protection = PROT_READ | PROT_WRITE;

  /***************************************************** 
   *  MAP_SHARED 与其它所有映射这个对象的进程共享映射空间 
   *  MAP_ANONYMOUS 匿名映射，映射区不与任何文件关联 
   ****************************************************/
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  /* void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset); */
  return mmap(NULL, size, protection, visibility, -1, 0);
}



/* 初始化shell */ 
void init_shell() 
{

    shared_memory = create_shared_memory(MAX_SHARED_MEM_SIZE);
    char parent_message[] = "Hello, I am your father!\n";
    memcpy(shared_memory, parent_message, sizeof(parent_message));

    clear();
}

char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}


/* 执行内置命令，如果执行失败，返回非0 */
int execute_command_builtin(int argc, char **argv)
{
    static char pre_dir[1024] = {0};/* 保存之前的目录 */
    char *username = getenv("USER");/* 用户名 */
    char path[1024]; /* 路径缓冲区 */
    int status = 0;/* 执行状态 */
    int switchPos = 0;/* 命令匹配位置*/
    int i;
    char *tmp;
    int n;

    if(argc <= 0 || argv == NULL){
        return ARGUMENTS_ERROR;
    }
    /* 寻找匹配的命令 */
    for(i = 0; i < COMMAND_BUILTIN_NUM; i++){
        if(strcmp(argv[0], command_builtin[i]) == 0){
            break;
        }
        switchPos++;
    }
    
    switch(switchPos){
        /* exit命令 */
        case EXIT_COMMAND:

        printf("Goodbye!\n");
        exit(SUCCESS);

        /* cd命令，只接受一个参数 */
        case CD_COMMAND:

        if (argc > 2) {
            printf("bash: cd: too many arguments\n");
            return SUCCESS;
        }

        /* 返回家目录 */
        if (argc == 1 || (strcmp(argv[1],"~") == 0)){
        
            n = snprintf(path, sizeof(path), "/home/%s/", username);
            if (n < 0){
                return SUCCESS;
            }
            clear_string(pre_dir);
            if (!getcwd(pre_dir, sizeof(pre_dir))){
                print_error("");
            }
            status = chdir(path);
            if (status == -1){
                print_error("bash: cd: ");
            }

        } else if (strcmp(argv[1], "-") == SUCCESS){
            
            if (strlen(pre_dir) == 0) {
                printf("bash: cd: OLDPWD not set\n");
                return SUCCESS;
            }

            clear_string(path);

            /* 返回上次所在目录 */
            if(!getcwd(path, sizeof(path))){
                print_error("");
            }

            if(chdir(pre_dir)==-1){
                print_error("bash: cd: ");
            }

            clear_string(pre_dir);

            if(!strcpy(pre_dir, path)){
                print_error("");
            }

        } else { /* 返回指定目录 */
            clear_string(pre_dir);

            if(!getcwd(pre_dir, sizeof(pre_dir))) {
                print_error("");
                return 0;
            }

            if (argv[1][0]=='~') {
                n = snprintf(path, sizeof(path), "/home/%s", username);
                if (n < 0) return n;
                if((tmp = str_replace(argv[1],"~",path)) == NULL ){
                    return -1;
                }
                if(chdir(tmp) == -1) {
                    print_error("bash: cd: ");
                } 
     
            } else if(chdir(argv[1]) == -1) {
                print_error("bash: cd: ");
            } else
                ;
            
        } 
        /* 错误处理 */
        if (status < 0){ 
            fprintf(stderr, "bash: cd: %s: %s\n", argv[1], strerror(errno));
            return 0;
        }
        
        break;

        /* 不是内置命令 */
        default:
            status = NOT_BUILTIN_COMMAND;
            break;
    }
    return status;
}

/* 提取输入 */ 
int get_user_input(char* str) 
{ 
    char *buf;
    char outputbuf[1500]; 
    char* username = getenv("USER");
    char hostbuff[256];
    int hostname;
    char cwd[1024];
    int n;

    
    if (!getcwd(cwd, sizeof(cwd))){
        print_error("");
        return 1;
    }
    
    if (gethostname(hostbuff, sizeof(hostbuff)) < 0){
        print_error("");
        return 1;
    }


    
    if (strcmp("root", username) == 0) {
        n = sprintf(outputbuf, "\e[1;32m%s@%s\e[0m:\e[1;34m%s\e[0m# ", username, hostbuff, cwd);
    } else {
        n = sprintf(outputbuf, "\e[1;32m%s@%s\e[0m:\e[1;34m%s\e[0m$ ", username, hostbuff, cwd);
    }
    
    if(n < 0){
        print_error("");
        return 1;
    }

    /* 读取一行 */
    buf = readline(outputbuf);

    if (strlen(buf) != 0) {
        add_history(buf); /* 存入命令行列表中 */
        if(!strcpy(str, buf))  {
            print_error("");
            return 1;
        } 
        else return 0; 
    } else { 
        return 1; 
    } 
} 


/* 寻找管道符号 */ 
int parse_pipe(char* str, char** strpiped) 
{ 
    int i; 
    for (i = 0; i < 2; i++) { 
        strpiped[i] = strsep(&str, "|"); 
        if (strpiped[i] == NULL) 
        break; 
    } 

    if (strpiped[1] == NULL) return 0; 
    else return 1; 
} 

/* 删除多余的空格符 */
int parse_space(char* str, char** parsed) 
{ 
    int i; 

    for (i = 0; i < MAX_ARGUMENT_NUM; i++) { 
        parsed[i] = strsep(&str, " "); 

        /* is the end of the line */
        if (parsed[i] == NULL) 
        break; 

        /* is space */
        if (strlen(parsed[i]) == 0) 
        i--; 
    }
    return i;
} 


int process_string(char* str, char** parsed, char** parsedpipe, int argc[2]) 
{ 

    char* strpiped[2]; 
    int piped = 0; 

    piped = parse_pipe(str, strpiped); 

    /* 如果有管道符号则分两个命令 */
    if (piped) { 
        argc[0] = parse_space(strpiped[0], parsed); 
        argc[1] = parse_space(strpiped[1], parsedpipe); 

    } else { 

        argc[0] = parse_space(str, parsed); 
    } 

    /* 如果是内置命令则执行并返回0 */
    if (execute_command_builtin(argc[0], parsed) == SUCCESS) 
        return SUCCESS; 
    else
        return 1 + piped; 
} 

int execute_external_command(int argc, char **argv)
{
    int status;
    char command_path[256];
    DIR *dir = opendir(EXTERNAL_COMMAND_PATH);
    struct dirent *dirent;
    pid_t pid = 0;

    if (dir == NULL){
        print_error("");
        return OPEN_DIR_ERROR; 
    }

    /* 1. 寻找对应的可执行文件 */
    while ((dirent = readdir(dir)) != NULL){
        if (strcmp(argv[0], dirent->d_name) == 0){
            status = snprintf(command_path, sizeof(command_path), "%s%s", 
                     EXTERNAL_COMMAND_PATH,argv[0]);
            if (status < 0 ){
                print_error("");
                return status;
            }
            break;
        }
    }

    
    if(dirent){ 
        pid = fork();  

        if (pid == -1) { 
            printf("Failed forking child..\n"); 
            return -1; 
        } else if (pid == 0) {
            /*********************************************
             * 此处可调试共享内存:
             * printf("Child read: %s\n", shared_memory);
             *********************************************/
            /* 2. 执行命令 */
            execvp(command_path, argv);
        } else { 
            /* 3. 等待子进程结束 */
            wait(NULL);  
        } 
    } else {
        printf("%s: command not found\n", argv[0]); 
    }
}

int main() 
{ 
    char inputString[MAX_INPUT_STRING]; 
    char *parsedArgs[MAX_ARGUMENT_NUM]; 
    char *parsedArgsPiped[MAX_ARGUMENT_NUM];
    int execFlag = 0;
    int argc[2] = {0};
    init_shell(); 

    while (1) {
        /* 如果返回值不为0说明有错误 */
        if (get_user_input(inputString)) 
            continue;  
        
        /***************************************************************************** 
         * 调用process_string函数读取用户输入，如果用户输入的是内置命令，
         * 则执行并返回0，如果不是内置命令，则返回一个大于等于1的整数，
         * 等于1说明没有管道符号，大于1则有管道符号; 暂不实现带有管道符号的命令
         *****************************************************************************/
        execFlag = process_string(inputString, parsedArgs, parsedArgsPiped, argc);  
        
        if (execFlag == 1) {
            execute_external_command(argc[0], parsedArgs);
        }
        if (execFlag == 2) 
               ;
    } 
    return 0; 
} 

