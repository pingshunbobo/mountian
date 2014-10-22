#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *file = "conf/mountian.conf";
int get_config(char* ip,int* port, int* thread_num)
{
	int confd;
	char *buf;
	char *index;
	struct stat f_stat;

	confd = open(file,O_RDONLY);
	if(confd <= -1){
		perror("open");
		exit(1);
	}
	fstat(confd,&f_stat);
	buf = mmap(NULL,f_stat.st_size,PROT_READ|PROT_WRITE,MAP_PRIVATE,confd,0);
	if(*(int *)buf <= -1)
	{
		perror("write");
		exit(1);
	}
	if((index = strstr(buf,"port:")) != NULL){
		index += 5;
		*port = atoi(index);
	};	
	if((index = strstr(buf,"threadnumber:")) != NULL){
		index += 13;
                *thread_num = atoi(index);
	}
	if((index = strstr(buf,"address:")) != NULL){
		index += 9;
		char * point = index;
		point += strcspn(index," \r\n");
		*point = '\0';
		strncpy(ip,index,strlen(index)+1);
	}
	return 0;
}
