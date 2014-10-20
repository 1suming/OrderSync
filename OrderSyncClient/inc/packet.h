﻿#ifndef __PACKET_H_
#define __PACKET_H_

#include <string>
#include <stdint.h>

using std::string;

#ifndef __MAX_PACKET_SIZE__
#define __MAX_PACKET_SIZE__ 16 * (1 << 10)
#endif

#pragma pack(push, 1)

typedef struct header_s header_t;
struct header_s {
	unsigned short 	len;		/* 包体长度，不包含字段本身 */
	unsigned char 	mark[2]; 	/* "BY" */
	unsigned char 	main;		/* 主版本 */
	unsigned char 	sub;		/* 子版本 */
	unsigned short 	cmd;		/* 命令字 */
	unsigned char 	code;		/* 校验码 */
};
#pragma pack(pop)


class packet_t {
public:
	packet_t()
		:_data(NULL),
		sz(__MAX_PACKET_SIZE__),
		idx(0),
		len(0)
	{
	}

	~packet_t() { delete _data; }
public:
	int begin(unsigned short cmd) 
	{
		char *c;

		if (_data == NULL) {
			_data = malloc(sz);

			if (_data == NULL) {
				return -1;
			}
		}

		c = _data + 6;

		memcpy(c, &cmd, sizeof cmd);

		return 0;
	}

	int end() 
	{
		encode();

		*(_data + 2) = 'B';
		*(_data + 3) = 'Y';
		*(_data) = len -2;

		return 0;
	}

	int write_int8(int8_t b);
	int write_uint8(uint8_t b);
	int write_int16(int16_t s);
	int write_uint16(uint16_t s);
	int write_int(int i);
	int write_uint(unsigned int u);
	int write_int64(int64_t l);
	int write_uint64(uint64_t u);
	int write_double(double d);
	int write_string(const char* s);
	int write_string(const string& s);

	int8_t read_int8();
	uint8_t read_uint8();
	int16_t read_int16();
	uint16_t read_uint16();
	int read_int();
	unsigned int read_uint();
	int64_t read_int64();
	uint64_t read_uint64();
	double read_double();
	char* read_string();
	string read_string();

	header_t* get_header();
private:
	int encode();
	int decode();
private:
	char 			*_data;
	unsigned short 	sz;	 // 缓冲区大小
	unsigned short	idx; // 缓冲区已读长度
	unsigned short	len; // 缓冲区已写长度
};

#endif