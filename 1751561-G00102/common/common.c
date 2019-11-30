#include"common.h"

/*------------------------------------------牟------------------------------------------*/
/*设置文件锁*/
int set_lock(int fd, int type)
{
	struct flock old_lock, lock;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_type = type;
	lock.l_pid = -1;
	if ((fcntl(fd, F_SETLKW, &lock)) < 0)
	{
		printf("Lock failed");
		return 1;
	}
	return 0;
}

/*使k在[1 ~ MAX_SEQ-1]间循环增长*/
int inc_seq_num(int &seq_num)
{
	if (seq_num < 10000)
		seq_num++;
	else
		seq_num = 0;
	return seq_num;
}


int key_from_network_layer = 0;//信号阻塞
void key_from_network_layer_enable()//信号启用
{
	key_from_network_layer = 1;
}

/*发送方从网络层得到纯数据包*/
void from_network_layer(packet* p)
{
	while (key_from_physical_layer == 0)();//信号阻塞
	key_from_physical_layer = 0;

	static int seq_num = 1;
	int fd;
	char share_filename[MAX_FILE_LEN];
	sprintf(share_filename, "%s%04d", N_D_SHARE, seq_num);
	inc(seq_num);
	fd = open(share_filename, O_RDONLY);
	set_lock(fd, F_RDLCK);
	read(share_file, p->data, MAX_PKT);
	set_lock(fd, F_UNLCK);
	close(fd);
	int pid = FindPidByName("./sender_network");//获得网络层进程的pid
	kill(pid, 40);//发送40号信号
	printf("datalink receive from network successfully\n");
}

/*接收方向网络层发送纯数据包*/
void to_network_layer(packet* p)
{
	static int seq_num = 1;
	char share_filename[MAX_FILE_LEN];
	int d_nfd;
	sprintf(share_filename, "%s%04d", D_N_SHARE, seq_num);
	inc(seq_num);
	d_nfd = open(share_file_name, O_WRONLY | O_CREAT, 0644);
	write(d_nfd, p->data, MAX_PKT);
	close(d_nfd);
}

/*接收方从物理层取得帧，调用本函数前已验证过校验和，若发生错误,则发送cksum_err事件
因此只有帧正确的情况下会调用本函数*/
int key_from_physical_layer = 0;//信号阻塞
void key_from_physical_layer_enable()//信号启用
{
	key_from_physical_layer = 1;
}

void from_physical_layer(frame* s);
{
	while (key_from_physical_layer == 0)();//信号阻塞
	key_from_physical_layer = 0;

	static int seq_num = 1;
	int fd;
	char share_filename[seq_num];
	sprintf(share_filename, "%s%04d", P_D_SHARE, seq_num);
	inc(seq_num);//读取文件序号+1
	fd = open(share_filename, O_RDONLY);//打开文件
	set_lock(fd, F_RDLCK);//设置文件锁
	read(fd, *(s->kind), 4);//帧类型
	read(fd, *(s->seq), 4);//发送序号
	read(fd, *(s->ack), 4);//接收序号
	if (s->kind == frame_kind)read(fd, s->info->data, MAX_PKT);//数据包
	set_lock(fd, F_UNLCK);//解开文件锁
	close(fd);
	printf("数据链路层已从物理层读入帧数据\n");
}

/*发送方向物理层发送帧*/
void to_physical_layer(frame *s);
{
	static int seq_num = 1;
	char share_filename[MAX_FILE_LEN];//写入文件名
	int fd;
	sprintf(share_filename, "%s%04d", D_P_SHARE, s->);//文件名
	inc(seq_num);//文件序号+1
	fd = open(share_file_name, O_WRONLY | O_CREAT, 0644);//打开文件
	set_lock(fd, F_RDLCK);//设置文件锁
	write(fd, *(s->kind), 4);//帧类型
	write(fd, *(s->seq), 4);//发送序号
	write(fd, *(s->ack), 4);//接收序号
	if (s->kind == frame_kind)write(fd, s->info->data, MAX_PKT);//数据包写数据
	set_lock(fd, F_UNLCK);//解开文件锁
	close(fd);//关文件
	int pid = FindPidByName("./sender_physical");//获得网络层进程的pid
	kill(pid, 40);//发送信号让物理层读文件
	printf("数据链路层已向物理层写入文件\n");
}

void from_datalink_layer(frame *s)
{
	static int seq_num = 1;
	char share_file_name[MAX_FILE_LEN];
	int fd = -1;
	sprintf(share_file_name, "%s%04d", D_P_SHARE, seq_num);
	inc(seq_num);
	while (fd == -1)
	{
		fd = open(share_file_name, O_RDONLY);
	}
	/*加锁，读取文件*/
	set_lock(fd, F_RDLCK);
	read(fd, s, sizeof(frame));
	set_lock(fd, F_UNLCK);//读完开锁

	close(fd);
}

/*------------------------------------------冕------------------------------------------*/
/*物理层之间传输*/
void send_to_phy(frame *s,int sockfd)
{
	if (s->kind == nak || s->kind == ack)
		write(sockfd, s, FRAMEHEAD);
	else
		write(sockfd, s, sizeof(frame));
}

static event_type e;

static void TIMEOUT()
{
	e = timeout;
}

static void CksumErr()
{
	e = cksum_err;
}

static void FrameArrival()
{
	e = frame_arrival;
}

static void NLReady()
{
	e = network_layer_ready;
}

static void wait_for_event(event_type* event)
{
	signal(SIGALRM, TIMEOUT);
	signal(35, CksumErr);
	signal(36, FrameArrival);
	signal(37, NLReady);
	pause();
	*event = e;
	return;
}

void enable_network_layer()
{
	char buf[128] = { 0 };
	FILE *fp = NULL;
	fp = popen("ps -ef|grep _network|awk '{print $2}'", "r");//找到网络层进程的pid号，发送信号使用
	fgets(buf, 127, fp);
	pid_t pid;
	pid = atoi(buf);
	kill(pid, 38);
}

void disable_network_layer()
{
	char buf[128] = { 0 };
	FILE *fp = NULL;
	fp = popen("ps -ef|grep _network|awk '{print $2}'", "r");//找到网络层进程的pid号，发送信号使用
	fgets(buf, 127, fp);
	pid_t pid;
	pid = atoi(buf);
	kill(pid, 39);
}

/*------------------------------------------舟------------------------------------------*/
//-----------------------------计时器-------------------------------
LinkList timeL;//使用的链表
int nowtime, nowusec;//现在是第几帧、第几毫秒
struct timeval tv;//用于获取当前时间

/*启动第k帧的定时器*/
void start_timer(seq_nr k)
{
	if (k < nowtime)return;
	Listadd(timeL, k - nowtime);
}
/*停止第k帧的定时器*/
void stop_timer(seq_nr k)
{
	int pos = List_find(timeL, k), e;
	Listdelete(timeL, pos, e);
}
/*启动确认包定时器*/
void start_ack_timer(void)
{

}
/*停止确认包定时器*/
void stop_ack_timer(void)
{

}

/*对定时器进行维护*/
void timer_keep(void)
{
	gettimeofday(&tv);//获取当前时间
	if (tv.tv_usec == nowusec)return;//触发毫秒时间中断
	int n;
	n = L->next->data;//获取当前第一个定时器的时间
	n--;//时间减一
	if (n == 0)//计时器超时
	{
		Listdelete(timeL, 1, e);//删除第一个计时器
		//发送timeout事件，等待填入
	}
}

/*定时器初始化，请在开始时使用*/
void timer_create(void)
{
	nowtime = 0;//初始化当前帧

	gettimeofday(&tv);//获取当前时间
	nowusec = tv.tv_usec;//初始化当前时间
}

