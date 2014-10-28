#ifndef _DEFS_H_
#define _DEFS_H_

#define HTTP_SVR_NS_BEGIN namespace __http_svr_ns__ {
#define HTTP_SVR_NS_END }
#define USING_HTTP_SVR_NS using namespace __http_svr_ns__;

#define MAX_WEB_RECV_LEN            102400
#define MAX_ACCEPT_COUNT            256
enum CConnState {
    CONN_IDLE,
    CONN_FATAL_ERROR,
    CONN_DATA_ERROR,
    CONN_CONNECTING,
    CONN_DISCONNECT,
    CONN_CONNECTED,
    CONN_DATA_SENDING,
    CONN_DATA_RECVING,
    CONN_SEND_DONE,
    CONN_RECV_DONE,
    CONN_APPEND_SENDING,
    CONN_APPEND_DONE
};

enum TDecodeStatus {
    DECODE_FATAL_ERROR,
    DECODE_DATA_ERROR,
    DECODE_WAIT_HEAD,
    DECODE_WAIT_CONTENT,
    DECODE_HEAD_DONE,
    DECODE_CONTENT_DONE,
    DECODE_DONE,
    DECODE_DISCONNECT_BY_USER
};

typedef struct tagRspList {
    char*   _buffer;
    int     _len;
    int     _sent;
    int     _type;

    struct tagRspList* _next;

} TRspList;

enum {
	CLIENT_CMD_SYC = 0x0002,
	SERVER_CMD_SYC = 0x0001
};

enum ConnType {
	CONN_CLINET,
	CONN_OTHER
};

//server¼äÍ¨ĞÅĞ­Òé
#define CLIENT_PACKET  		 0x0001//ÓÃ»§·¢¹ıÀ´µÄÊı¾İ°ü»òÕßserver·¢¸øÓÃ»§µÄ°ü
#define CLIENT_PACKET2		 0x0004//ÓÃ»§·¢¹ıÀ´µÄÊı¾İ°ü»òÕßserver·¢¸øÓÃ»§µÄ°ü(ĞÂÃüÁî×Ö)

#define CLIENT_CLOSE_PACKET  0x0002//ÓÃ»§¶Ï¿ªÁ¬½Ó£¬hall¸øÂß¼­server·¢µÄ°ü
#define SERVER_CLOSE_PACKET  0x0003//Âß¼­serverÖ÷¶¯¶Ï¿ªÁ¬½ÓµÄ°ü
#define SYS_RELOAD_CONFIG    0x0010//ÖØĞÂ¼ÓÔØÅäÖÃ
const int SYS_RELOAD_IP_MAP = 0x0011;//ÖØĞÂ¼ÓÔØipÓ³Éä
const int SYS_RESET_CONNECT_TIMER = 0x0012;//ÖØĞÂÉèÖÃconnect³¬Ê±Ê±¼ä
const int SYS_OPEN_DEBUG = 0x000D;//ÉèÖÃdebugÈÕÖ¾¿ª¹Ø

const int CLIENT_CMD_CLIENT_LOGIN = 0x0101;//ÓÃ»§´óÌüµÇÂ¼°ü
const int CMD_GET_NEW_ROOM = 0x0117;//Ëæ»ú³¡µã»÷ÁË×¼±¸,ÖØĞÂÇëÇó·¿¼äºÅ
const int CLIENT_CMD_REQUEST_LIST_ROOM = 0x11D;//ÓÃ»§ÇëÇó·¿¼äÁĞ±í
const int CLIENT_CMD_ENTER_ROOM = 0x0125;//ÓÃ»§ÇëÇó½øÈë·¿¼ä

const int SERVER_CMD_LOGIN_SUCCESS = 0x0201;//ÓÃ»§µÇÂ¼´óÌü³É¹¦
const int SERVER_CMD_KICK_OUT = 0x0203;//ÌŞ³ıÓÃ»§
const int SERVER_CMD_ENTER_ROOM = 0x0212;//·µ»ØÓÃ»§½øÈë·¿¼ä

const int SERVER_CMD_SET_IP = 0x0301;
const int CMD_GET_ROOM_LEVER_NUM = 0x0311;//»ñÈ¡¸÷µÈ¼¶³¡ÈËÊı

const int CMD_REQUIRE_IP_PORT = 0x0604;//ÇëÇóserverµÄipºÍ¶Ë¿Ú

const int CMD_SEPARATE = 0x0800;//ÃüÁî·Ö¸ô  ·Ö¸îÓÎÏ··şÎñÆ÷ÃüÁî×Ö

const int UDP_SERVER_LOG    =0x0901;    //ÉÏ±¨ÃüÁî×ÖÈÕÖ¾server
const int UDP_SERVER_CLIENT_CLOSE_LOG = 0x0902;//ÉÏ±¨ÓÃ»§Àë¿ª´óÌü
const int UDP_SERVER_USER_COUNT      = 0x0903;//ÉÏ±¨µ±Ç°ÓÃ»§ÓĞ¶àÉÙÈË

const int CLIENT_COMMAND_LOGIN = 0x1001;//ÓÃ»§µÇÂ¼Ä³¸ö·¿¼ä
const int SERVER_COMMAND_LOGOUT_SUCCESS = 0x2013;//ÓÃ»§³É¹¦ÍË³ö·¿¼ä

const int CLIENT_COMMAND_BREAK_TIME = 0x1016;//ĞÄÌø°ü

const int SERVER_GET_PLAYER_COUNT   = 0x3018;  //»ñµ½½ø³ÌÓĞ¶àÉÙÍæ¼Ò

const int SERVER_COMMAND_BREAK_TIME = 0x2017;//·µ»ØĞÄÌø

const int SERVER_COMMAND_CHANGE_GAMESERVER = 0x7213;

const int SERVER_BROADCAST_INFO = 0x7050;  // broadcast system
const int SERVER_BROADCAST_INFO_NEW = 0x7054;//ĞÂµÄ¹ã²¥Ğ­Òé
const int CLINET_REQUEST_BROADCAST_INFO = 0x7055; //Çø·ÖÖÕ¶ËÀàĞÍµÄ¹ã²¥

#ifndef CONST_T
#define CONST_T(T, V, v, b) const T V = v + b
#define CMD(V, v, b) CONST_T(int, V, v, b)
#else
#undef CONST_T
#undef CMD
#define CONST_T(T, V, v, b) const T V = v + b
#define CMD(V, v, b) CONST_T(int, V, v, b)
#endif

#ifndef UNUSED
#define UNUSED(v) (void)(v)
#endif

#define CLIENT_BASE 	0x0
#define SERVER_BASE 	0x1000
#define INTER_REQ_BASE	0x8000
#define INTER_RES_BASE	0x9000

CMD(CLIENT_CMD_REQ, 0x1, CLIENT_BASE);					/* æ”¯ä»˜è¯·æ±‚ */
CMD(SERVER_CMD_REP, 0x1, SERVER_BASE);					/* æ”¯ä»˜å“åº” */

CMD(INTER_CMD_REQ, 0x1, INTER_REQ_BASE);				/* å†…éƒ¨æ”¯ä»˜è¯·æ±‚ */
CMD(INTER_CMD_RES, 0x1, INTER_RES_BASE);				/* å†…éƒ¨æ”¯ä»˜å“åº” */

CMD(INTER_CMD_WAITER_STAT_REQ, 0x2, INTER_REQ_BASE);   	/* Waiteré˜Ÿåˆ—çŠ¶æ€è¯·æ±?*/
CMD(INTER_CMD_WAITER_STAT_RES, 0x2, INTER_RES_BASE);  	/* Waiteré˜Ÿåˆ—çŠ¶æ€å“åº?*/

CMD(INTER_CMD_NOTIFY_STAT_REQ, 0x3, INTER_REQ_BASE);	/* Notifyé˜Ÿåˆ—çŠ¶æ€è¯·æ±‚*/	
CMD(INTER_CMD_NOTIFY_STAT_RES, 0x3, INTER_RES_BASE);	/* Notifyé˜Ÿåˆ—çŠ¶æ€å“åº”*/

CMD(CLIENT_CMD_LOGIN_REQ, 0x1, CLIENT_BASE);			/* åŒæ­¥å®¢æˆ·ç«¯ç™»é™†è¯·æ±‚ */
CMD(SERVER_CMD_LOGIN_REP, 0x1, SERVER_BASE);			/* åŒæ­¥å®¢æˆ·ç«¯ç™»é™†å“åº” */

CMD(CLIENT_CMD_DATA_REQ, 0x2, CLIENT_BASE);				/* æ•°æ®åŒæ­¥è¯·æ±‚ */
CMD(SERVER_CMD_DATA_REP, 0x2, SERVER_BASE);

enum {
	JobWorkerType = 1,
	NotifyWorkerType
};

typedef struct server_stat_s server_stat_t;
struct server_stat_s {
	unsigned int _w_qlen; /* length of waiter's queue */
	unsigned int _n_qlen; /* length of notify's queue */
	unsigned int _w_ctime;
	unsigned int _n_ctime;
};

typedef struct server_switch_s server_switch_t;
struct server_switch_s {
	bool _w_is_busyed;
	bool _n_is_busyed;
};

CONST_T(int, SVR_MAX_W_QLEN, 1024, 0);
CONST_T(int, SVR_MAX_N_QLEN, 1024, 0);


#endif

