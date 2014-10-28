#ifndef __H_TTC_CLIENT_UNIT_H__
#define __H_TTC_CLIENT_UNIT_H__

#include <global.h>
#include <noncopyable.h>
#include <client_unit.h>

class CPollerUnit;
class CTimerList;

HTTP_SVR_NS_BEGIN

class CClientUnit;
class CHelperUnit;
class CGameUnit;
class CListener;

class CDecoderUnit : private noncopyable
{
    friend class CListener;
public:
	CDecoderUnit (CListener* owner);
	~CDecoderUnit (void);

	int listener_input (void);
    int web_input (void);
    int web_output (void);
    int web_hangup (void);
    int web_timer (void);  
    void complete (void);

	int ProcessUserClose();

    inline CTimerList* get_web_timer (void) const 
	{ 
		return _webTimer;
	}

	inline void set_web_timer (CTimerList* timerlist)
	{ 
		_webTimer = timerlist;
	}

    inline CTimerList* get_helper_timer (void) const 
	{ 
		return _helperTimer; 
	}

	inline void set_helper_timer (CTimerList* timerlist) 
	{ 
		_helperTimer = timerlist;
	}
    
    inline CPollerUnit* pollerunit (void) const 
	{ 
		return _pollerUnit;
	}

	inline void pollerunit (CPollerUnit* pollerunit) 
	{ 
		_pollerUnit = pollerunit;
	}    

	inline CClientUnit* get_web_unit (void) const 
	{	
		return _webUnit;
	}

	inline CGameUnit* get_game_unit(void) const
	{
		return _gameUnit;
	}
	inline void set_flag(int flag)
	{
		_flag = flag;
	}

	inline int get_flag() const
	{ 
		return _flag;
	}

	inline void set_netfd(int fd)
	{
		_fd = fd;
	}

	inline int get_netfd() const
	{
		return _fd;
	}

	inline void set_ip(unsigned int ip)
	{
		_ip = ip;
	}

	inline int get_conn_type()
	{
		return _conn_type;
	}

	inline void set_conn_type(int type)
	{
		_conn_type = type;
	}

	inline unsigned int get_ip() const
	{
		return _ip;
	}
private:
    CPollerUnit*    _pollerUnit;
    CTimerList*     _webTimer;
    CTimerList*     _helperTimer;
    CClientUnit*    _webUnit;
	CGameUnit*   	_gameUnit;
    CListener*      _listener;
	int				_flag;
	int 			_fd;
	uint32_t        _ip;
	int             _conn_type;
};

HTTP_SVR_NS_END
#endif

