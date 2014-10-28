#ifndef __SOCKETIMPL_H__
#define __SOCKETIMPL_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef WIN32 //for windows nt/2000/xp

	#include <time.h>
	#include <signal.h>
	#ifndef _WINSOCKAPI_ 
		#pragma message("Including winsock2.h")
		#include <winsock2.h>
	#endif

	#pragma comment(lib,"ws2_32.lib")
#else //for unix
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/signal.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#if _AIX  //for ibm aix
		#include <sys/libcsys.h>
	#elif __HP_aCC //for hp_ux
	#else	//here for sun solaris
	#endif

	//仅用于类型转换时同Windows保持兼容
	typedef int SOCKET;
#endif

#ifdef WIN32
#define INIT_SOCKET_ENV static int volatile _g_socketFlag=SOCKET_OS::InitSocketEnv();
#define CLEAN_SOCKET_ENV
#else
#define INIT_SOCKET_ENV
#define CLEAN_SOCKET_ENV
#endif

#ifndef WIN32
	typedef struct sockaddr_in SOCKADDR_IN;
	typedef struct sockaddr    SOCKADDR;
#endif

typedef struct
{
	char LocalIP[32];
	char RemoteIP[32];
	int  LocalPort;
	int  RemotePort;
}SOCKET_INFO, *LPSOCKET_INFO;

class SOCKET_OS
{
public:
	static const char *PROTO_UDP;
	static const char *PROTO_TCP;

	/**
	 * 初始化Socket环境，只有在windows
	 * 系统下才用得到。
	 */
	static int InitSocketEnv();

	/**
	 * 初始化sockaddr_in数据库结构，根据参数host和service
	 * 填充sockaddr_in的sin_addr和sin_port项
	 */
	static int BuildSockAddr(SOCKADDR_IN *addr, 
							 const char *host,
							 int port);

	/**
	 * 根据服务名称查询端口号
	 * 即从系统文件/etc/services中查询服务对应的端口号
	 * 返回值：
	 *     <0：没有查询到
	 *     >0：端口号
	 */ 
	static int GetPortByName(const char *service, const char *proto = PROTO_TCP);

	/**
	 * 读取Socket的信息，包含本地和远端的信息
	 * 参数说明：
	 *     sockfd：Socket句柄
	 *     sinfo： Socket信息结构地址
	 * 
	 * 返回值：
	 *     =0：调用成功
	 *     !=0：失败
	 */
	static int GetSocketInfo(int sockfd, LPSOCKET_INFO sinfo);

	/**
	 *  发送dlen个字节到Socket，
	 *  适应场景：TCP & UDP
	 *  参数说明：
	 *      sockfd:  Socket句柄
	 *      data:    要写入的数据块
	 *      dlen:    数据块的长度
	 *      timeout: 超时参数
	 *  返回值：
	 *    >0: 成功发送的字节数
	 *     0: 超时，写入的字节数保存在dlen中
	 *    <0: 网络错误
	 */
	static int SendBytes(int sockfd, const void *data, int dlen, int timeout=-1);

	/**
	 *  在指定时间内从Socket读入dlen个字节，
	 *  同时把实际读到的字节数存在dlen中
	 *  适应场景：TCP & UDP
	 *  参数说明：
	 *      sockfd:  Socket句柄
	 *      data:    接收内存
	 *      dlen:    数据块的长度
	 *      timeout: 超时参数
	 *  返回值：
	 *    >0:  成功读取的字节数
	 *     0:  超时，实际读取的字节数存放在dlen中
	 *    <0:  网络错误
	 */
	static int RecvBytes(int sockfd, void *data, int dlen, int timeout=-1);

	/**
	 *  关闭Socket
	 *  适应场景：TCP & UDP
	 */
	static void CloseSocket(int sockfd);

	/**
	 *  立即关闭socket，实际上是首先设置
	 *  Linger等待时间为0，然后在调用CloseSocket
	 *  适应场景：TCP & UDP
	 *  参数说明：
	 *      sockfd：Socket句柄
	 */
	static void CloseImmediate(int sockfd);


	/**
	 * 关闭Socket。
	 */
	static void Shutdown(int socketfd);

	/**
	 * 创建Socket,成功返回创建的Socket句柄，<0创建失败。
	 */
	static int CreateTCPSocket();
	static int CreateUDPSocket();

	/**
	 *  绑定Socket到指定端口和地址
	 *  适应场景：TCP & UDP
	 *  参数说明：
	 *     sockfd：Socket句柄
     *     port：  要绑定的Socket端口号
	 *     addr：  绑定地址，默认为ANY
	 *  返回值：
	 *    >0：返回Socket句柄
	 *    <0：创建失败
	 */
	static int Bind(int sockfd, int port, const char *addr=0);

	/**
	 * 创建侦听端口，端口号为port，用在Tcp调用
	 * 适应场景：TCP only
	 * 参数说明：
	 *     port：侦听端口
	 *     backflag：侦听队列大小
	 *     addr：绑定地址，默认为ANY
	 * 返回值：
	 *    >0：返回Listener Socket句柄
	 *    <0：创建失败
	 */
	static int CreateListener(int port, const char *addr=0, int backflag=20);

	/**
	 * 等待客户端的连接
	 * 适应场景：TCP & UDP
	 * 参数说明：
	 *    timeout：超时，单位（毫秒），小于零表示永不超时
	 * 返回值：
	 *    <0：调用系统函数accept失败
	 *    >0：客户端socket连接句柄
	 *    =0：等待连接超时
	 */
	static int Accept(int sockfd);
	static int AcceptEx(int sockfd,int timeout=-1);

	/**
	 * 连接到服务器
	 * 适应场景：TCP & UDP，通过参数
	 * 参数说明：
	 *     sockfd：Socket句柄
	 *     addr：  已经初始化好的struct sockaddr_in
	 * 返回值：
	 *     >0：成功返回连接后的Socket句柄
	 *     <0：连接失败！
	 * 
	 */
	static int Connect(int sockfd, SOCKADDR_IN *addr);
	static int TCPConnect(const char *host, int port);
	static int UDPConnect(const char *host, int port);


	/**
	 *  设置Socket重用参数，
	 *  适应场景：TCP & UDP
	 *  参数说明：
	 *     opt=true  ：该端口允许重用
	 *     opt=false ：该端口不允许重用
	 */
	static void SetReuseOption(int sockfd, bool opt);

	/**
	 *  设置Socket Linger参数
	 *  适应场景：TCP & UDP
	 *  参数说明：
	 *      opt=true : enable linger  =false disable linger
	 *      timeout : 当发送缓冲还有数据时，该Socket继续保留的时间
	 */
	static void SetLingerOption(int sockfd, bool opt, int timeout=0);

	/**
	 *  下面两个函数等待Socket读写事件，用select实现
	 *  适应场景：TCP & UDP
	 *  参数说明：
	 *      sockfd： Socket句柄
	 *      timeout：超时参数
	 *  返回结果：
	 *      >0：等到期望的事件
	 *      =0：等待超时
	 *      <0：网络错误
	 */
	static int WaitForW(int sockfd, int timeout);
	static int WaitForR(int sockfd, int timeout);


	/**
	 * 从Socket读取数据
	 * 适应场景：TCP & UDP
	 * 参数说明：
	 *     sockfd： Socket句柄
	 *     buf：    内存
	 *     buf_len：内存最大容量
	 * 返回结果：
	 *     返回读到字节数
	 *     网络错误返回结果<0
	 */
	static int Send(int sockfd, void *buf, int buf_len);

	/**
	 * 写数据到指定Socket
	 * 适应场景： TCP & UDP
	 * 参数说明：
	 *     sockfd：  Socket句柄
	 *     buf：     数据内存
	 *     data_len：数据大小
	 * 返回结果：
	 *     返回写入的字节数
	 *     网络错误返回结果<0
	 */
	static int Recv(int sockfd, void *buf, int data_len);


	static int Select(int width, fd_set *rfds, fd_set *wfds=0, fd_set *efds=0, int timeout=-1);

	/////////////////////////////////////////////////////////////////
	// 下面的方法只适应UPD Socket
	////////////////////////////////////////////////////////////////

	/**
	 * 从socket读取数据，封装API recvfrom
	 * 返回值说明：
	 *     返回读取到的字节数。读失败时返回结果小于0。
	 */
	static int RecvFrom(int sockfd, void *pbuf, int buf_len, SOCKADDR_IN *addr_in, int timeout);

	/**
	 * 写数据库块到Socket，封装API sendto
	 * 返回值:
	 *     返回写成功的字节数。写失败时放回结果小于0
	 * 
	 */
	static int SendTo(int sockfd, const void *pBuf, int data_len, SOCKADDR_IN *addr, int timeout);

	static int GetHostName(char *host_name, int name_len);

	static int GetLastError();
	static char *GetErrorMsg(int err, char *szMsg, int len);
	static void SetLastError(int err);
};


#endif /* __SOCKETIMPL_H__ */
