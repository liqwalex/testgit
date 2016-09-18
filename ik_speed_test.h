#include <stdio.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>  
#include <assert.h> 
#include <sys/socket.h>
#include <math.h>


#define HTTP_METHOD  "GET"
#define HTTP_UP  "POST"
#define BUF_SIZE 1024
#define READ_BUF 10240
#define SEND_BUF 2048
#define TIME_OUT_TIME 10
#define TIME_OUT_MIN  2
#define MAX_NAME_LEN  16
#define IPV4_SIZE  16
#define  HTTP_PORT 80
#define  RESULT_FILE "/tmp/ik_speed_result"
#define  AGENT "User-Agent: Mozilla/5.0 (Linux; U; 64bit; en-us) Python/2.7.6 (KHTML, like Gecko) speedtest-cli/0.3.4\r\n\r\n"
#define  HTTP_SEND_LEN 249993
#define  HTTP_SEND_NUM 40
#define  EARTH 6371
#define  MIN_DELAY 1800000


typedef struct worker 
{ 
    /*回调函数*/ 
    void *(*process) (void *arg); 
    void *arg;
    struct worker *next; 
 
} CThread_worker; 

typedef struct 
{ 
	pthread_mutex_t queue_lock; 
	pthread_cond_t queue_ready; 
	/*任务链表,队列模式*/  
	CThread_worker *queue_head; 
	int cur_queue_size; 

	/* destroy button */  
	int shutdown; 
	pthread_t *threadid; 

	int max_thread_num; 
	int work_thread_num;
    
} CThread_pool;

struct download_post_info{
	char interface[MAX_NAME_LEN];
	char server_ip[IPV4_SIZE];
	char post_string[BUF_SIZE];
};

int pool_add_worker (void *(*process) (void *arg), void *arg); 
void *thread_routine (void *arg);  
void write_result(char* interface,char *result);
double get_average_delay(char* server_ip,char* host_domain,char* interface_name);
void fill_down_request(char* buffer,int jpg_size,char* vist_domain);
void* file_download(void *arg);
int bind_interface(char* interface_name,int sockfd);
void fill_upload_request(char* buffer,char* vist_domain);
static double rad(double d);
static double distance(double lat1,double lon1,double lat2,double lon2);

