/*
  This program reads /proc dir and returns process id and command
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>


int check_proc(const char* proc_id,int lower_bound){
    int flag = 0;
    char sfile[255];
    char sbuffer[50000];
    sprintf(sfile,"/proc/%s/cmdline",proc_id );
    int f=open(sfile,O_RDONLY);
    if(f==-1){
        sprintf(sfile,"Can't open proc %s", proc_id );
        perror(sfile);
        return 0;
    }
    sbuffer[0]='\0';
    char buffer;
    char space[2];
    space[0]=' ';
    space[1]='\0';
    while(read(f,&buffer,1)!=0){
        if(buffer=='\0') strncat(sbuffer,space,1);
        strncat(sbuffer,&buffer,1);
    }
    close(f);
    int count = 0;
    char num[50];
    num[0] = '\0';
    int k = 0;
    for(int i=0;i<strlen(sbuffer);i++){
        if(count == 4){
            if(sbuffer[i] >= '0' && sbuffer[i] <= '9'){
                num[k] = sbuffer[i];
                k++;
            }
            else{
                if(k == 0)
                    num[0] = '\0';
                break;
            }
                
        }
        else{
            if(' ' == sbuffer[i]){
                count++;
            }
        }   
    }
    int number=0;
    if(num[0] != '\0'){
        number = atoi(num);
    }
    if(number == lower_bound)
        flag = atoi(proc_id);
    if(flag == 0)
        return 1;
    else
        return flag;
}

int my_getpid(int low){
    DIR *dirp;
    struct dirent *dp;
    int control = 0;
    if ((dirp = opendir("/proc")) == NULL) {
        perror("couldn't open '/proc'");
    }
    while( (dp = readdir(dirp)) != NULL){
        if( dp->d_type == DT_DIR && isdigit( dp->d_name[0] ) ){
            control = check_proc( dp->d_name,low);
        }
    }
    closedir(dirp);
    return control;
}


int  main( int argc, char** argv ){
    int lowerbound;
    int c;
    char *lower;
    while ((c = getopt(argc, argv, "c:d:r:p:")) != -1){
        switch (c){
            case 'c':
                lower = optarg;
                break;
        }
    }
    lowerbound = atoi(lower);
    printf(" my = %d \n",my_getpid(lowerbound));
    printf("%d \n",getpid());
    return 0;

}