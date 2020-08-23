#include<stdio.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>

#include<errno.h>

#define MAX_OPTIONS_NUM 3
#define SUCCESS         0
#define IS_OPTION(i)  argv[i][0] == '-'

void do_ls(char []);
void get_stat(char*, int );
void show_file_info(char* ,struct stat*, int);
void mode_to_letters(int ,char[]);
char* uid_to_name(uid_t);
char* gid_to_name(gid_t);
void show_without_argument(char *);

static char* options[MAX_OPTIONS_NUM] = {
    "-a",
    "-l",
    "-lh"
};

int main(int argc, char **argv){
    DIR *dirp;
    struct dirent *dp = NULL;
    int i;
    char tmp[10];
    char path[256];
    int n;


    if (argc == 1){
        show_without_argument("./");
        return SUCCESS;
    } else {
        /* 带有参数 
        * 支持的命令格式
        * ls -arg path 
        * ls path -arg 
        * ls -arg 
        **/
        /* 第二个位置是选项 */
        if(IS_OPTION(1)){

            /* 匹配选项 */
            for (i = 0; i< MAX_OPTIONS_NUM; i++){
                if(strcmp(options[i], argv[1]) == 0)
                {
                    if (argc > 2) snprintf(path, sizeof(path), "%s",argv[2]);
                    else snprintf(path, sizeof(path), "./");
                    break;
                }
            }


            /* 第二个位置是路径 */
        } else {
            n = snprintf(path, sizeof(path), "%s", argv[1]);
            if(n<0) {
                fprintf(stderr, "%s\n", strerror(errno));
                return SUCCESS;
            }


            if(argc == 2) {
                show_without_argument(path);
                return SUCCESS;
            } else{
                for (i = 0; i< MAX_OPTIONS_NUM; i++){
                    if(strcmp(options[i], argv[2]) == 0){
                        break;
                    }
                }
            }
        }
        chdir(path);
        switch(i){
            case 0:
            // printf("show hidden!\n");
            dirp = opendir(path); 
            while ((dp=readdir(dirp))!=NULL) {
                printf("%s\n", dp->d_name);
            }
            closedir(dirp);
            break;

            case 1:
            // printf("show detail!\n");
            dirp = opendir(path);
            while ((dp=readdir(dirp))!=NULL){
                get_stat(dp->d_name, 0);
            }
            break;
            case 2:
            // printf("show readable message!\n");
            dirp = opendir(path);
            while ((dp=readdir(dirp))!=NULL){
                get_stat(dp->d_name, 1);
            }
            break;

            default:
            strncpy(tmp, argv[1]+1, sizeof(tmp));
            printf("ls: invalid option -- '%s'\n", tmp);
            printf("Try 'ls --help' for more information.\n");
            break;

        }
}
return 0;
}

/* 只打印文件名且不打印隐藏文件 */
void show_without_argument(char *path){
    DIR *dirp;
    struct dirent *dp = NULL;

    /* 打印当前目录的所有文件名 */
    dirp = opendir(path);

    if(!dirp) {
        fprintf(stderr, "%s\n", strerror(errno));
        return;
    }
    while ((dp=readdir(dirp))!=NULL) {
        if (dp->d_name[0] != '.'){
            /* 默认不打印隐藏文件 */
            printf("%s\n", dp->d_name);
        }
    }
    closedir(dirp);

}


void get_stat(char* filename, int readable)
{
    struct stat info;
    if((stat(filename,&info)) == -1)
        fprintf(stderr, "%s\n", strerror(errno));
    else
        show_file_info(filename,&info,readable);
}

void show_file_info(char* filename,struct stat * info_p, int readable)
{
    double readable_digit;
    char modestr[11];
    /* 将文件模式转为字符形式 */
    mode_to_letters(info_p->st_mode,modestr);
    /* 打印信息 */
    printf("%s",modestr);
    printf("%4d ",(int)info_p->st_nlink);
    printf("%-8s ",uid_to_name(info_p->st_uid));
    printf("%-8s ",gid_to_name(info_p->st_gid));
    if(!readable){
        printf("%8ld ",(long)info_p->st_size);
    } else {
        /* 将文件大小转为容易读的形式 */
        readable_digit = (double)info_p->st_size/1024.0;
        if(readable_digit < 1024){
            printf("%7.1fk ", readable_digit);
        } else if (readable_digit<1024.0*1024.0){
            readable_digit = readable_digit/1024.0;
            printf("%7.1fM ", readable_digit);
        } else {
            readable_digit = readable_digit/(1024.0* 1024.0);
            printf("%7.1fG ", readable_digit);
        }
    }
    printf("%.12s ",ctime(&info_p->st_mtime)+4);
    printf("%s\n",filename);
}

void mode_to_letters(int mode,char str[])
{
    strcpy(str,"----------");
    if(S_ISDIR(mode)) str[0] = 'd';  //"directory ?"
    if(S_ISCHR(mode)) str[0] = 'c';  //"char decices"?
    if(S_ISBLK(mode)) str[0] = 'b';  //block device?


    //3 bits for user
    if(mode&S_IRUSR) str[1] = 'r';
    if(mode&S_IWUSR) str[2] = 'w';
    if(mode&S_IXUSR) str[3] = 'x';

    //3 bits for group
    if(mode&S_IRGRP) str[4] = 'r';
    if(mode&S_IWGRP) str[5] = 'w';
    if(mode&S_IXGRP) str[6] = 'x';

    //3 bits for other
    if(mode&S_IROTH) str[7] = 'r';
    if(mode&S_IWOTH) str[8] = 'w';
    if(mode&S_IXOTH) str[9] = 'x';
}

char* uid_to_name(uid_t uid)
{
    struct passwd* pw_ptr;
    static char numstr[10];
    if((pw_ptr =getpwuid(uid)) == NULL)
    {
        sprintf(numstr,"%d",uid);
        printf("world");
        return numstr;
    }
    return pw_ptr->pw_name;
}

char* gid_to_name(gid_t gid)
{
    /*returns pointer to group number gid, used getgrgid*/
    struct group* grp_ptr;
    static char numstr[10];
    if((grp_ptr =getgrgid(gid)) == NULL)
    {
        printf("hello wofjl");
        sprintf(numstr,"%d",gid);
        return numstr;
    }
    else
    return grp_ptr->gr_name;
}
