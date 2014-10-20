#ifndef __FETCHER_H_
#define __FETCHER_H_

#include <string>

using std::string;

class redis_helper_t;

class fetcher_t {
public:
	fetcher_t(redis_helper_t *r):_r(r) {}
	~fetcher_t() { delete _r; }

public:
	string fetch(const char* q);
private:
	redis_helper_t *_r;
};

#endif __FETCHER_H_

