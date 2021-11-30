#pragma once

#include "defNetwork.h"

class CCrypt
{
public:
	CCrypt();
	virtual ~CCrypt();

	void		JvEncryption( T_KEY keyPrivate, int len, T_OCTET* datain, T_OCTET* dataout );
	void		JvDecryption( T_KEY keyPrivate, int len, T_OCTET* datain, T_OCTET* dataout );

private:
	T_KEY		m_keyPublic;
};
