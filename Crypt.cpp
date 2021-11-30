#include "StdAfx.h"
#include "Crypt.h"

CCrypt::CCrypt()
{
	//Packet Decryption Key Generate
	m_keyPublic		= (LONGLONG)rand();
	m_keyPublic		*= (LONGLONG)rand();
	m_keyPublic		*= (LONGLONG)rand();
	m_keyPublic		*= (LONGLONG)rand();
}

CCrypt::~CCrypt()
{
}

void CCrypt::JvEncryption( T_KEY keyPrivate, int len, T_OCTET* datain, T_OCTET* dataout )
{
	T_KEY	tkey;
	T_OCTET	*pkey, lkey, rsk;
	int		rkey;

	tkey = m_keyPublic ^ keyPrivate;
	pkey = (T_OCTET*)&tkey;
	rkey = 2157;
	lkey = (len * 157) & 0xff;

	for( int i = 0; i < len; i++ )
	{
		rsk = (rkey >> 8) & 0xff;
		dataout[i] = ((datain[i] ^ rsk) ^ pkey[(i % 8)]) ^ lkey;
		rkey *= 2171;
	}
	
}

void CCrypt::JvDecryption( T_KEY keyPrivate, int len, T_OCTET* datain, T_OCTET* dataout )
{
	int		rkey;
	T_KEY	tkey;
	T_OCTET	*pkey, lkey, rsk;

	tkey = m_keyPublic ^ keyPrivate;
	pkey = (T_OCTET *)&tkey;
	rkey = 2157;
	lkey = (len * 157) & 0xff;

	for( int i = 0; i < len; i++ )
	{
		rsk = (rkey >> 8) & 0xff;
		dataout[i] = ((datain[i] ^ lkey) ^ pkey[(i % 8)]) ^ rsk;
		rkey *= 2171;
	}
}