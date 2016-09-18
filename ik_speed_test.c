#include "ik_speed_test.h"
#define  IK_APP_DEBUG
#include "ik_app_debug.h"
  
static CThread_pool *pool = NULL; 
int bind_fd = 0;
long long sum_lens = 0;
long long sum_send_lens = 0;
int pthread_number = 10;

static void wakeup_help(FILE * fp, const char *pname)
{
	fprintf(fp, "Usage: %s <command>\n"
		"  -n  --name, --ip\tinterface for speed\n"
		"  -j, --pthread_n\tnumber of pthread\n"
		"  -d, --distance\tdistance between two point\n"
		"  -h, --help\tprint wakeup help\n", pname);
}

static double rad(double d)
{
        return d * M_PI/180.0;
}

static double distance(double lat1,double lon1,double lat2,double lon2)
{
	double dlat = fabs(rad(lat1) - rad(lat2));
	double dlon = fabs(rad(lon1) - rad(lon2));
	double a,c,d;
	//printf("===%f,%f\n",dlat,dlon);
	a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
	//printf("a===%f\n",a);
	a=fabs(a);
	c = 2 * atan2(sqrt(a), sqrt(fabs(1 - a)));
	//printf("c===%f\n",c);
	d = EARTH * c;
	printf("%.6f\n",d);
    return d;
}


void pool_init (int max_thread_num) 
{ 
	pool = (CThread_pool *) malloc (sizeof (CThread_pool)); 
	if(pool == NULL)
		exit(1);
	pthread_mutex_init (&(pool->queue_lock), NULL); 
	pthread_cond_init (&(pool->queue_ready), NULL); 
	pool->queue_head = NULL; 
	pool->max_thread_num = max_thread_num; 
	pool->work_thread_num = 0; 
	pool->cur_queue_size = 0; 
	pool->shutdown = 0; 
	pool->threadid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t)); 
	if(pool->threadid == NULL)
		exit(1);
	int i = 0; 
	for (i = 0; i < max_thread_num; i++) 
	{  
		pthread_create (&(pool->threadid[i]), NULL, thread_routine,NULL); 
	} 
}
 
int pool_add_worker (void *(*process) (void *arg), void *arg) 
{ 
	CThread_worker *newworker = (CThread_worker *) malloc (sizeof (CThread_worker)); 
	if(newworker == NULL){
		exit(1);
	}
	newworker->process = process; 
	newworker->arg = arg; 
	newworker->next = NULL; 

	pthread_mutex_lock (&(pool->queue_lock)); 
	/*add the mission into the mission queue*/ 
	CThread_worker *member = pool->queue_head; 
	if (member != NULL) 
	{ 
		while (member->next != NULL) 
			member = member->next; 
		
		member->next = newworker; 
	} 
	else 
	{ 
		pool->queue_head = newworker; 
	} 

	assert (pool->queue_head != NULL); 

	pool->cur_queue_size++;
	pthread_mutex_unlock (&(pool->queue_lock)); 
	/* wake one pthread to get work */  
	pthread_cond_signal (&(pool->queue_ready)); 
	return 0; 
} 

/*销毁线程池，等待队列中的任务不会再被执行，但是正在运行的线程会一直把任务运行完后再退出*/
int pool_destroy () 
{ 
	/*not again*/
	if (pool->shutdown) 
		return -1;  
	pool->shutdown = 1; 

	/*wake all sleep pthread ,it is over*/  
	pthread_cond_broadcast(&(pool->queue_ready)); 

	int i; 
	for (i = 0; i < pool->max_thread_num; i++) 
		pthread_join (pool->threadid[i], NULL); 
	
	free (pool->threadid); 

	/*destroy the mission queue */
	CThread_worker *head = NULL; 
	while (pool->queue_head != NULL) 
	{ 
		head = pool->queue_head; 
		pool->queue_head = pool->queue_head->next; 
		free (head); 
	} 

	pthread_mutex_destroy(&(pool->queue_lock)); 
	pthread_cond_destroy(&(pool->queue_ready)); 

	free (pool); 
	pool=NULL;

	return 0; 
} 
 
void *  thread_routine (void *arg) 
{ 
    //printf ("starting thread 0x%lu\n", pthread_self ()); 
    while (1) 
    { 
	/*如果任务队列为0并且不销毁线程池，则处于阻塞状态*/ 
        pthread_mutex_lock (&(pool->queue_lock)); 
         
        while (pool->cur_queue_size == 0 && !pool->shutdown) 
        { 
            //printf ("thread 0x%lu is waiting\n", pthread_self ()); 
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock)); 
        } 
 
         
        if (pool->shutdown) 
        { 
            /* unlock  befor exit */
            //printf ("thread 0x%lu will exit\n", pthread_self ()); 
            pthread_mutex_unlock (&(pool->queue_lock)); 
            pthread_exit (NULL); 
        } 
 
        //printf ("thread 0x%lu is starting to work\n", pthread_self ()); 
 
         
        assert (pool->cur_queue_size != 0); 
        assert (pool->queue_head != NULL); 
         
        /*get head of mission queue*/
        pool->cur_queue_size--;
	
        CThread_worker *worker = pool->queue_head; 
        pool->queue_head = worker->next; 
        pthread_mutex_unlock (&(pool->queue_lock)); 
 
        /*excute mission */ 
        (*(worker->process)) (worker->arg); 
        free (worker); 
        worker = NULL;
    } 
     
    pthread_exit (NULL); 
} 

void *  myprocess (void *arg) 
{ 
	//struct download_post_info *client;
	//client = (struct download_post_info *)arg;
	//printf ("threadid : 0x%lu, task %d \n", pthread_self (),*(int *) arg); 
	//printf("interface: %s\nserver ip: %s\npost string \n%s",client->interface,client->server_ip,client->post_string); 
	return NULL; 
} 

void* file_upload(void *arg)
{
	struct download_post_info *info;
	info = (struct download_post_info *)arg;
	int i = 0,seek;
	int size = 250*100*100;
	int loop_num = size / (36*30);
	char * chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char * first_one = "content1=0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int  first_len = strlen(first_one);
	char post_string[SEND_BUF+1];
	long long send_sum = 0;
	
	bzero(post_string,(SEND_BUF+1));
	
	for(i=0;i<30;i++){
		seek = i*36;
		snprintf((post_string+seek),SEND_BUF,"%s",chars);
	}
	int post_string_len = strlen(post_string);
	/* socket init */
	int sockfd;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(HTTP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(info->server_ip);
	
	/* create socket*/
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		IK_APP_ERR_LOG("ik_speed upload socket() %s\n",strerror(errno));
		goto psend;
	}
	
	/* bind choice */
	if(bind_fd == 1){
		if(bind_interface(info->interface,sockfd)==1){
			goto psend;
		}
	}
	
	/*connect timeout  set time */
	struct timeval timeo = {TIME_OUT_MIN,0};
	socklen_t len = sizeof(timeo);
	
	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len) == -1){
		IK_APP_ERR_LOG("ik_speed upload setsockopt() %s \n",strerror(errno));
		goto psend;
	}
	
		/* http connect to server ip */		
	int connect_res = connect(sockfd,(struct sockaddr *)&dest_addr,sizeof(dest_addr));
	if(connect_res == -1){
		IK_APP_ERR_LOG("ik_speed upload connect() %s\n",strerror(errno));
		goto psend;;
	}
	
	/*set recv  time out time */
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeo, len) == -1){
		IK_APP_ERR_LOG("ik_speed upload setsockopt() %s\n",strerror(errno));
		write_result(info->interface,"线程 设置接受超时时间失败");
		goto psend;
	}
	
	/*get upload delay*/
	int nSend,nRecv;	
	int head_sendlen = strlen(info->post_string);
	nSend = send(sockfd,info->post_string,head_sendlen,0);	
	if(nSend<0){
		write_result(info->interface," upload 线程发送数据包失败");
		goto psend;
	}
	send_sum += nSend;
	nSend = send(sockfd,first_one,first_len,0);
	if(nSend<0){
		write_result(info->interface," upload 线程发送数据包失败");
		goto psend;
	}
	
	//send_sum = head_sendlen + first_len;
	send_sum += nSend;
	/*send post string */
	for(i=1;i<loop_num;i++){
		nSend = send(sockfd,post_string,post_string_len,0);
		if(nSend<0){
			IK_APP_ERR_LOG("ik_speed upload send() %s\n",strerror(errno));
			goto psend;
		}
		//send_sum += post_string_len;
		send_sum += nSend;
	}
	char read_buf[20];
	nRecv = recv(sockfd,read_buf,11,0);			
	if(nRecv<0){
		IK_APP_ERR_LOG("ik_speed upload recv %s\n",strerror(errno));	
		goto psend;
	}
	
psend:
	//printf("thread send : %lld\n",send_sum);
	pthread_mutex_lock (&(pool->queue_lock)); 
	sum_send_lens += send_sum;
	pthread_mutex_unlock (&(pool->queue_lock));
	close(sockfd);
	return NULL;
}

void upload_speed(char* dest_server_ip,char* interface_name,char* vist_domain)
{
	int i;
	int loop = HTTP_SEND_NUM;
	struct download_post_info info[HTTP_SEND_NUM];
	/*delay time */
	struct timeval starttime,endtime;
	
	/*fill struct for post */
	for(i=0;i<loop;i++){
		fill_upload_request(info[i].post_string,vist_domain);
		strncpy(info[i].server_ip,dest_server_ip,IPV4_SIZE);
		strncpy(info[i].interface,interface_name,MAX_NAME_LEN);
	}
	
	gettimeofday(&starttime,0);
	for(i=0;i<loop;i++){
		pool_add_worker(file_upload,&info[i]); 
	}
	while(1){ 
		usleep(1000);
		//IK_APP_DEBUG_LOG("pool->cur_queue_size == %d\n",pool->cur_queue_size);
		if(pool->cur_queue_size == 0){
			pool_destroy();
			gettimeofday(&endtime,0);
			double timeuse = (1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec)/1000000;
			double result = (sum_send_lens / timeuse / (1024 * 1024));
			printf("sum_lens: %lld byte | time_use: %f s | speed : %.2fMB\n",sum_send_lens,timeuse,result);
			break;
		}
	}
}

void download_speed(char* dest_server_ip,char* interface_name,char* vist_domain)
{
	/*https check ?*/
	/*create  url like http://speedtest.bmcc.com.cn/speedtest/random350x350.jpg */
	int size[] = {4000,4000,4000,3500,3500,3500,3000,3000,3000,2500,2500,2500,2000,2000,2000,1500,1500,1500,1000,1000,1000,750,750,750,500,500,500,350,350,350};
	int size_num = sizeof(size)/4;
	int i;
	struct download_post_info info[size_num];
	/*delay time */
	struct timeval starttime,endtime;

	
	for(i=0;i<size_num;i++){
		fill_down_request(info[i].post_string,size[i],vist_domain);
		strncpy(info[i].server_ip,dest_server_ip,IPV4_SIZE);
		strncpy(info[i].interface,interface_name,MAX_NAME_LEN);
	}
	gettimeofday(&starttime,0);
	for(i=0;i<size_num;i++){
		pool_add_worker(file_download,&info[i]); 
	}
	while(1){ 
		usleep(1000);
		//IK_APP_DEBUG_LOG("cur_queue_size = %d\n",pool->cur_queue_size);
		if(pool->cur_queue_size == 0){
			pool_destroy();
			/*when the pool been destroy , mission had been done */
			gettimeofday(&endtime,0);
			double timeuse = (1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec)/1000000;
			double result = (sum_lens / timeuse / (1024 * 1024));
			printf("sum_lens: %lld byte | time_use: %f s | speed : %.2fMB\n",sum_lens,timeuse,result);
			break;
		}
	}
}

void* file_download(void *arg)
{
	struct download_post_info *info;
	info = (struct download_post_info *)arg;
	long long recv_lens_sum=0;
	
	/* socket init */
	int sockfd;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(HTTP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(info->server_ip);	
	
	/* create socket*/
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		IK_APP_ERR_LOG("ik_speed failed  create socket in download speed \n");
		goto pend;
	}

	/* bind choice */
	if(bind_fd == 1){
		if(bind_interface(info->interface,sockfd)==1){
			goto pend;
		}
	}
		
	/*connect timeout  set time */
	struct timeval timeo = {TIME_OUT_MIN,0};
	socklen_t len = sizeof(timeo);
	
	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len) == -1){
		IK_APP_ERR_LOG("ik_speed setsockopt() set timeout %s\n",strerror(errno));
		goto pend;
	}
	
	/* http connect to server ip */		
	int connect_res = connect(sockfd,(struct sockaddr *)&dest_addr,sizeof(dest_addr));
	if(connect_res == -1){
		IK_APP_ERR_LOG("ik_speed download connect() %s\n",strerror(errno));
		goto pend;;
	}
	
	/*set recv  time out time */
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeo, len) == -1){
		IK_APP_ERR_LOG("ik_speed download recv %s\n",strerror(errno));
		goto pend;
	}
	
	
	/*get delay*/
	int nSend,nRecv;	
	int sendlen = strlen(info->post_string);
	nSend = send(sockfd,info->post_string,sendlen,0);
	if(nSend<0){
		IK_APP_ERR_LOG("download send() %s",strerror(errno));
		goto pend;
	}
	char read_buf[READ_BUF];
	while(1){
		nRecv = recv(sockfd,read_buf,READ_BUF,0);			
		if(nRecv<0){
			if(errno==EAGAIN){
				IK_APP_ERR_LOG("ik_speed recv() %s\n",strerror(errno));
			}
			goto pend;
		}
		if(0 == nRecv)
			break;
		recv_lens_sum += nRecv;
		/*make the last pthread end more faster*/
		if(nRecv<4096&&pool->cur_queue_size==0)
			break;
	}
pend:
	//printf("thread recv : %lld\n",recv_lens_sum);
	pthread_mutex_lock (&(pool->queue_lock)); 
	sum_lens += recv_lens_sum;
	pthread_mutex_unlock (&(pool->queue_lock));
	close(sockfd);
	return NULL;
}

void write_result(char* interface,char *result)
{
	char buffer[BUF_SIZE]={0};
	int file_fd;

	file_fd = open(RESULT_FILE,O_RDWR|O_CREAT|O_APPEND,0644);
	if(file_fd < 0){
		IK_APP_ERR_LOG("ik_speed_test  open result file failed\n");
		exit(1);
	} 
	snprintf(buffer,BUF_SIZE,"%s:%s\n",interface,result);
	write(file_fd,buffer,strlen(buffer)+1);
	close(file_fd);
}

void getBestServer(char* interface_name)
{
	FILE* file_fd;
	char buffer[BUF_SIZE];
	char vist_url[BUF_SIZE];
	char vist_ip[IPV4_SIZE];
	char result_ip[IPV4_SIZE];
	double min_delay = MIN_DELAY;
	/*choice best from 5 address */
	int test_num = 0;
	file_fd = fopen("/tmp/speed_best","r");
	if(file_fd == NULL){
		IK_APP_ERR_LOG("ik_speed_test open file best failed\n");
		exit(1);
	}
	
	while(!feof(file_fd)){
		bzero(buffer,BUF_SIZE);
		bzero(vist_url,BUF_SIZE);
		bzero(vist_ip,IPV4_SIZE);
		test_num++;
		if(fgets(buffer,1024,file_fd) == NULL){
			write_result(interface_name,"ik_speed getbestserver 读取失败");
			exit(1);
		}
		char* url = strstr(buffer,"url");
		if(url==NULL)
			continue;
		char* http = strstr(url,"http://");
		if(http == NULL){
			continue;
		}
		int len1 = strlen(http);
		char* http_end = strstr(http,"speedtest/upload.");
		if(http_end == NULL){
			continue;
		}
		int len2 = strlen(http_end);
		int diff = len1 - len2 - 7;
		snprintf(vist_url,diff,"%s",http+7);

		struct hostent *host;
		
		if((host = gethostbyname(vist_url)) == NULL){
			write_result(interface_name,"ik_speed getbestserver  dns 解析失败");
			exit(1);
		}
		if(inet_ntop(host->h_addrtype,*host->h_addr_list,vist_ip,IPV4_SIZE) == NULL){
			write_result(interface_name,"ik_speed getbestserver   转换ip失败");
			exit(1);
		}
		double delay=0.0;
		int delay_loop;
		for(delay_loop=0;delay_loop<3;delay_loop++){
			delay+=get_average_delay(vist_ip,vist_url,interface_name);
		}
		//printf("%s : %f\n",vist_url,delay);
		if(delay<min_delay){
			min_delay = delay;
			strncpy(result_ip,vist_ip,IPV4_SIZE);
		}	

		/* loop number limit*/
		if(5 == test_num)
			break;
	}
	
	fclose(file_fd);
	printf("min_delay : %f,last_ip : %s\n",min_delay,result_ip);
	printf("start test download speed! ......\n");
	download_speed(result_ip,interface_name,vist_url);
	sleep(1);
	printf("start test upload speed! ......\n");
	pool_init(pthread_number);
	upload_speed(result_ip,interface_name,vist_url);
}

void fill_down_request(char* buffer,int jpg_size,char* vist_domain)
{
	char *agent  = AGENT;
	char *method = HTTP_METHOD;
	char size[BUF_SIZE];
	bzero(size,BUF_SIZE);
	snprintf(size,BUF_SIZE,"random%dx%d.jpg ",jpg_size,jpg_size);
	char *http_url = "/speedtest/";
	char *accept = "Accept-Encoding: identity";
	char *connect = "Connection: close";
	snprintf(buffer,BUF_SIZE,"%s %s%s HTTP/1.1\n%s\nHost: %s\n%s\n%s",method,http_url,size,accept,vist_domain,connect,agent);
	//printf("%s\n",buffer);
}

void fill_upload_request(char* buffer,char* vist_domain)
{
	char *agent  = AGENT;
	char *method = HTTP_UP;
	char *http_content = "Content-Length: ";
	char *http_url = "/speedtest/upload.php";
	char *accept = "Accept-Encoding: identity";
	char *connect = "Connection: close";
	char *content_type = "Content-Type: application/x-www-form-urlencoded";
	snprintf(buffer,BUF_SIZE,"%s %s HTTP/1.1\n%s%d\n%s\nHost: %s\n%s\n%s\n%s",method,http_url,http_content,HTTP_SEND_LEN,accept,vist_domain,content_type,connect,agent);
	//printf("%s\n",buffer);
}

void fill_latency_request(char* buffer,char* domain)
{
	//char *header = "User-Agent: Mozilla/5.0 (Linux; U; 64bit; en-us) Python/2.7.6 (KHTML, like Gecko) speedtest-cli/0.3.4\r\n\r\n";
	char *agent  = AGENT;
	char *method = HTTP_METHOD;
	char *http_url = "/speedtest/latency.txt";
	char *http_accept = "Accept-Encoding: identity";
	snprintf(buffer,BUF_SIZE,"%s %s HTTP/1.1\nHost: %s\n%s\n%s",method,http_url,domain,http_accept,agent);
	//printf("len = %d\n",(int)strlen(buffer));
}

int bind_interface(char* interface_name,int sockfd)
{
	struct ifreq interface;
	memset(&interface, 0, sizeof(interface));
	strncpy(interface.ifr_ifrn.ifrn_name, interface_name, strlen(interface_name));
	if(setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface, sizeof(interface)) < 0){
		IK_APP_ERR_LOG("ik_speed bind to device failed\n");
		write_result(interface_name," 绑定网卡失败 ");
		return 1;
	}
	return 0;
}

double get_average_delay(char* server_ip,char* host_domain,char* interface_name)
{
	/* socket init */
	int sockfd;
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(HTTP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(server_ip);	
	
	/* create socket*/
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		IK_APP_ERR_LOG("ik_speed delay socket() %s\n",strerror(errno));
		return MIN_DELAY;
	}	
	
	/* bind choice */
	if(bind_fd == 1){
		bind_interface(interface_name,sockfd);
	}
		
	/*connect timeout  set time */
	struct timeval timeo = {TIME_OUT_TIME-5,0};
	socklen_t len = sizeof(timeo);
	
	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len) == -1){
		IK_APP_ERR_LOG("ik_speed delay SO_SNDTIMEO setsockopt() %s\n",strerror(errno));
		return MIN_DELAY;
	}
	
	/* change recv buf */
	int rcvbuf_len=512*1024;
	socklen_t int_len = sizeof(int);
	if(getsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,(void*)&rcvbuf_len,&int_len) < 0 ){
		return -1;
	}
	printf("the recevice buf len: %d\n", rcvbuf_len );

	char buffer[BUF_SIZE];
	char recv_buffer[BUF_SIZE];
	fill_latency_request(buffer,host_domain);
		
	/* http connect to server ip */		
	int connect_res = connect(sockfd,(struct sockaddr *)&dest_addr,sizeof(dest_addr));
	if(connect_res == -1){
		IK_APP_ERR_LOG("ik_speed %s delay connect\n",strerror(errno));
		return MIN_DELAY;
	}
	
	/*set recv  time out time */
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeo, len) == -1){
		IK_APP_ERR_LOG("ik_speed delay SO_RCVTIMEO setsockopt() %s\n",strerror(errno));
		return MIN_DELAY;
	}
		
	/*delay time */
	struct timeval starttime,endtime; 
	double average_time = 0;
	
	/*get delay*/
	int sendlen = strlen(buffer);
	int i,nSend,nRecv;
	for(i=0;i<1;i++){		
		gettimeofday(&starttime,0);
		nSend = send(sockfd,buffer,sendlen,0);
		if(nSend<0){
			IK_APP_ERR_LOG("ik_speed delay test send() %s\n",strerror(errno));
			close(sockfd);
			return MIN_DELAY;
		}
		nRecv = recv(sockfd,recv_buffer,sendlen,0);
		
		if(nRecv<0){
			IK_APP_ERR_LOG("ik_speed delay test recv() %s\n",strerror(errno));
			close(sockfd);
			return MIN_DELAY;
		}
		
		gettimeofday(&endtime,0);
		double timeuse = 1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec;
		average_time += timeuse;
	}
	
	close(sockfd);
	
	if(average_time<=0){
		IK_APP_ERR_LOG("ik_speed failed to get delay time \n");
		return MIN_DELAY;
	}
	
	return  average_time;
}

int  main(int argc,char *argv[]){
	
	//get options
	char *const short_options = "n:j::d::h";
	char interface_name[MAX_NAME_LEN]={0};
	int opt;	
	
	struct option long_opts[] = {
		{ "name",	required_argument,	NULL, 'n' },
		{ "pthread_n", optional_argument, NULL, 'j' },
		{ "distance", optional_argument, 	NULL, 'd' },
		{ "help",	no_argument, 		NULL, 'h' },
		{ NULL, 0 , NULL, 0 }
	};	

	while((opt = getopt_long(argc,argv,short_options,long_opts,NULL))!=-1){
		int i=0;
		switch(opt){
			case 'n':
				if(strlen(optarg) + 1 > MAX_NAME_LEN)
					return 1;
				else
					stpcpy(interface_name,optarg);
				//printf("%s\n",interface_name);
				bind_fd = 1;
				break;
			case 'j':
				if(!isdigit(optarg[i])){
						//IK_APP_ERR_LOG("invalid interger :"" %s, checkout your input \n",optarg);
						exit(1);
				}
				pthread_number = atoi(optarg);
				if(pthread_number > 20){
					printf("pthread number too much\n");
					return 1;
				}
					
				break;
			case 'd':
				if(optarg==NULL){
					//IK_APP_ERR_LOG("need your input \n",optarg);
					exit(1);
				}
				char *result[4]; 
				for(i=0;i<4;i++){
					if(i==0)
						result[i] = strtok(optarg,",");
					else
						result[i] = strtok(NULL,",");
				}
				/**
				for(i=0;i<4;i++){
					printf("%f\n",atof(result[i]));
				}
				**/
				distance(atof(result[0]),atof(result[1]),atof(result[2]),atof(result[3]));
				exit(1);
				break;
			case 'h':
				wakeup_help(stdout, argv[0]);
				return 0;
			default:
				//IK_APP_DEBUG_LOG("error option !\n");
				exit(1);
			}
	}
	
	pool_init(pthread_number);
	getBestServer(interface_name);
	return 0;
}
