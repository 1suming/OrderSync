#ifndef _COMM_PROTOCOL_H_
#define _COMM_PROTOCOL_H_

#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define EXTEND_HTTP_HEAD_VER    0x01

struct extend_http_head
{
    int32_t     _len;
    int64_t     _timestamp;
    uint32_t    _remote_ip;
    char        _version;
} __attribute__((packed));

class CExtendHttpHead
{
public:
    static inline int64_t get_timestamp (void)
    {
        struct timeval tv;
	    gettimeofday(&tv, NULL);
    
	    return ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec);
    }
    
    static inline void set_head (char* headbuf, int* len, const uint32_t remote_ip, const int bodylen, int is_timestamp = 1)
    {
        struct extend_http_head* http_head = (struct extend_http_head*)headbuf;

        http_head->_len         = sizeof (struct extend_http_head) + bodylen;

        if (is_timestamp)
        {
            http_head->_timestamp = get_timestamp ();
        }

        http_head->_version     = EXTEND_HTTP_HEAD_VER;
        http_head->_remote_ip   = remote_ip;

        //set headlen
        *len = http_head->_len;
    }
    
    static inline const int32_t get_length (const char* inbuf)
    {
        return ((const struct extend_http_head*)(inbuf))->_len;
    }
    static inline void set_length (char* inbuf, const int len)
    {
        ((struct extend_http_head*)(inbuf))->_len = len;
    }
    
    static inline const int64_t get_timestamp (const char* inbuf)
    {
        return ((const struct extend_http_head*)(inbuf))->_timestamp;
    }
    static inline void set_timestamp (char* inbuf, int64_t timestamp = 0)
    {
        if (0 == timestamp)
        {
            ((struct extend_http_head*)(inbuf))->_timestamp = get_timestamp ();
        }
        else
        {
            ((struct extend_http_head*)(inbuf))->_timestamp = timestamp;
        }
    }
    
    static inline const char get_version (const char* inbuf)
    {
        return ((const struct extend_http_head*)(inbuf))->_version;
    }
    static inline void set_version (char* inbuf, const char version)
    {
        ((struct extend_http_head*)(inbuf))->_version = version;
    }

    static inline const char* get_remote_ip (const char* inbuf)
    {
        struct in_addr inaddr;        
        inaddr.s_addr = ((const struct extend_http_head*)(inbuf))->_remote_ip;

        return inet_ntoa (inaddr);
    }
    static inline void set_remote_ip (char* inbuf, const char* remote_ip)
    {
        struct in_addr inaddr;
        inet_aton (remote_ip, &inaddr);
        
        ((struct extend_http_head*)(inbuf))->_remote_ip = (uint32_t)(inaddr.s_addr);
    }

    static inline int get_body (const char* inbuf, char* http_buf, int* bodylen)
    {
        const struct extend_http_head* http_head = ((const struct extend_http_head*)(inbuf));
        int32_t body_len = http_head->_len - sizeof(struct extend_http_head);
        
        memcpy (http_buf, inbuf, body_len);
        *bodylen = body_len;

        return 0;
    }
    static inline void set_body (char* inbuf, const char* http_buf, const int bodylen)
    {
        struct extend_http_head* http_head = ((struct extend_http_head*)(inbuf));
        http_head->_len = bodylen + sizeof(struct extend_http_head);
        
        memcpy (inbuf + sizeof(struct extend_http_head), http_buf, bodylen);
    }
    
};

#endif //_COMM_PROTOCOL_H_

