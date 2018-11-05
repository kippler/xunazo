#include "stdafx.h"
#include "XAzo.h"
#include "_private.h"
#include "XAzoDec.h"


#pragma warning(disable: 26486)



XAzo::XAzo()
{
	m_in = nullptr;
	m_out = nullptr;
	m_eob = false;
	m_x86filter = false;
	m_crc = 0;
}

XAzo::~XAzo()
{
}


bool XAzo::Open(XReadStream* in, XWriteStream* out)
{
	m_in = in;
	m_out = out;
	m_eob = false;
	m_crc = 0;

	SAzoMainHeader mainHeader;
	if (ReadByte(mainHeader.version) == FALSE)
		ERROR_RETURN;
	if (ReadByte(mainHeader.filter) == FALSE)
		ERROR_RETURN;

	if (mainHeader.version != '1')
		ERROR_RETURN;

	if (mainHeader.filter != 0 && mainHeader.filter != 1)
		ERROR_RETURN;

	m_x86filter = mainHeader.filter;

	return true;
}

bool XAzo::ReadByte(uint8_t& b)
{
	if (m_in->Read_(&b, sizeof(b)) == FALSE)
		ERROR_RETURN;
	return true;
}

bool XAzo::ReadUint(uint32_t& n)
{
	n = 0;
	for (int i = 0; i < 4; i++)
	{
		uint8_t b;
		if (m_in->Read_(&b, sizeof(b)) == FALSE)
			ERROR_RETURN;
		n = (n << 8) + b;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         블럭 하나 압축 풀기
/// @param  
/// @return 
/// @date   Wed Aug  1 14:39:50 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzo::DecodeOneBlock()
{
	SAzoBlockHeader blockHeader;
	if (ReadUint(blockHeader.uncomp) == FALSE) ERROR_RETURN;
	if (ReadUint(blockHeader.comp) == FALSE) ERROR_RETURN;
	if (ReadUint(blockHeader.check) == FALSE) ERROR_RETURN;

	// 마지막 블럭인가?
	if (blockHeader.uncomp == 0 && blockHeader.comp == 0 && blockHeader.check == 0)
	{
		m_eob = true;
		return true;
	}

	if (blockHeader.comp == blockHeader.uncomp)
		return CopyBlock(blockHeader.comp);

	return DecodeAzoBlock(blockHeader.comp, blockHeader.uncomp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         블럭을 통채로 복사하기
/// @param  
/// @return 
/// @date   Wed Aug  1 14:37:00 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzo::CopyBlock(uint32_t size)
{
	XBuffer buf;
	if (buf.Alloc(size) == FALSE)
		ERROR_RETURN;

	if (m_in->Read_(buf.data, size) == FALSE)
		ERROR_RETURN;

	if (m_x86filter)
		Azox86FilterDecoder(buf.data, size);
		
	m_crc = crc32(m_crc, buf.data, size);

	return m_out->Write(buf.data, size);
}




////////////////////////////////////////////////////////////////////////////////////////////////////
///         블럭 하나를 디코드
/// @param  
/// @return 
/// @date   Sun Jul 29 23:05:52 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzo::DecodeAzoBlock(uint32_t compSize, uint32_t uncompSize)
{
	// 블럭 하나 통채로 읽어서
	XBuffer bufI;
	XBuffer bufO;
	if (bufI.Alloc(compSize) == FALSE)
		ERROR_RETURN;
	if(bufO.Alloc(uncompSize) == FALSE)
		ERROR_RETURN;

	if (m_in->Read_(bufI.data, compSize) == false)
		ERROR_RETURN;

	XAzoDec dec;
	dec.Init(bufI.data, compSize, bufO.data, uncompSize);

	if (dec.Decode() == false)
		ERROR_RETURN;

	if (m_x86filter)
		Azox86FilterDecoder(bufO.data, uncompSize);

	m_crc = crc32(m_crc, bufO.data, uncompSize);

	return m_out->Write(bufO.data, uncompSize);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         압축 풀기
/// @param  
/// @return 
/// @date   Wed Aug  1 14:39:01 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzo::Extract()
{
	//int count = 0;

	while (!IsEndOfBlock())
	{
		const int startPos = (int)m_in->GetPos();
		if (DecodeOneBlock() == false)
			return false;

		//printf("%d pos:0x%x  crc:0x%08x\r", count++, startPos, m_crc);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         X86 필터 - BJC 와 유사하지만, 더 단순하고, 첫번째 블럭에서만 의미가 있다.
/// @param  
/// @return 
/// @date   Tue Aug  7 17:33:31 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
void XAzo::Azox86FilterDecoder(uint8_t* buf, uint32_t size)
{
	if (buf==NULL || size < 5) return;

	for (uint32_t i = 0; i < size - 4; i++)
	{
		if ((buf[i] == 0xe8 || buf[i] == 0xe9))
		{
			if ((buf[i + 4]) == 0 || buf[i + 4] == 0xff)
			{
				// 점프 어드레스의 엔트로피를 줄이기 위해서 상대 주소를 절대 주소로 변환했던것을 다시 상대 주소로 바꿔준다.
				uint32_t val = (*(uint32_t*)(buf + i + 1)) - i;

				// 상위 1바이트가 0xff 나 0x00 이 아니면 0x00 으로 만든다.
				if ((val & 0xff000000) != 0xff000000)
					val = val & 0x00ffffff;

				*(uint32_t*)(buf + i + 1) = val;
			}
			i += 4;
		}
	}
}


