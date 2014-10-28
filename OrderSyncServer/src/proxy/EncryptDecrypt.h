#ifndef BOYAA_ENCRYPT_DECRYPT_H
#define BOYAA_ENCRYPT_DECRYPT_H

#include "ICHAT_PacketBase.h"

class CEncryptDecrypt
{
public:
	CEncryptDecrypt(void);
	~CEncryptDecrypt(void);

public:
	WORD EncryptBuffer(NETOutputPacket *pPacket);	// 加密数据
	int  DecryptBuffer(NETInputPacket *pPacket);	// 解密数据

private:
	BYTE MapRecvByte(BYTE const cbData);
	BYTE MapSendByte(BYTE const cbData); 
private:
	static BYTE	m_SendByteMap[256];		//字节映射表
	static BYTE	m_RecvByteMap[256];		//字节映射表
};
#endif

