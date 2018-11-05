////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// AZO 압축 해제
/// 
/// @author   park
/// @date     Wed Aug  1 14:27:03 2018
/// 
/// Copyright(C) 2018 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "XStream.h"

class XAzo
{
public :
	XAzo();
	~XAzo();

	bool		Open(XReadStream* in, XWriteStream* out);
	bool		Extract();
	uint32_t	GetCRC() { return m_crc; }

private :
	bool		DecodeOneBlock();
	bool		IsEndOfBlock() { return m_eob; }

private:
	bool		ReadByte(uint8_t& b);
	bool		ReadUint(uint32_t& n);

	bool		CopyBlock(uint32_t size);
	bool		DecodeAzoBlock(uint32_t compSize, uint32_t uncompSize);

	void		Azox86FilterDecoder(uint8_t* buf, uint32_t size);

private :
	struct SAzoMainHeader
	{
		uint8_t	version;
		uint8_t	filter;
	};

	struct SAzoBlockHeader
	{
		uint32_t	uncomp;		// 해제 후 크기
		uint32_t	comp;		// 압축 된 크기
		uint32_t	check;		// 두 값의 xor
	};

private:
	XReadStream *	m_in;
	XWriteStream*	m_out;
	bool			m_eob;		// end of block?
	uint32_t		m_crc;
	bool			m_x86filter;

};

