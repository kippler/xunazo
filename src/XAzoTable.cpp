#include "stdafx.h"
#include "XAzoTable.h"

#pragma warning(disable: 26495)

XAzoTable::XAzoTable()
{
	// LEN 초기화
	if (lenTable[0] == 0)
	{
		for (int i = 0; i < 40; i++)
		{
			lenTable[i] = i + MIN_LEN;
			lenExtraBits[i] = 0;
		}

		// 40부터 8마다 1비트씩 extra bit 증가 시키기
		for (int i = 40; i < 128; i++)
		{
			const int bit = (i - 40) / 8 + 1;
			lenExtraBits[i] = bit;
			lenTable[i] = lenTable[i - 1] + (1 << lenExtraBits[i - 1]);
		}
	}

	// DIST 초기화
	if (distTable[0] == 0)
	{
		for (int i = 0; i < 20; i++)
		{
			distTable[i] = i + MIN_DIST;
			distExtraBits[i] = 0;
		}

		// 20부터 4마다 1비트씩 extra bit 증가 시키기
		for (int i = 20; i < 128; i++)
		{
			const int bit = (i - 20) / 4 + 1;
			distExtraBits[i] = bit;
			distTable[i] = distTable[i - 1] + (1 << distExtraBits[i - 1]);
		}
	}
}


XAzoTable::~XAzoTable()
{
}


uint32_t XAzoTable::LenIndex2Len(uint32_t index, uint32_t &lenExtraBits_) const
{
	if (index >= MAX_TABLE) { ASSERT(0); return 0; }
	lenExtraBits_ = lenExtraBits[index];
	return lenTable[index];
}


uint32_t XAzoTable::DistIndex2Dist(uint32_t index, uint32_t &lenExtraBits_) const
{
	if (index >= MAX_TABLE) { ASSERT(0); return 0; }
	lenExtraBits_ = distExtraBits[index];
	return distTable[index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         DIST 에서 역으로 DIST INDEX 를 계산한다. (0~127) 
/// @param  
/// @return 
/// @date   Thu Aug  2 11:46:14 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t XAzoTable::GetDistIndex(uint32_t dist) const
{
	// 최소 거리값 빼고
	const uint32_t dist_1 = dist - MIN_DIST;

	// 20 이하는 index 랑 1:1 대응
	if(dist_1 <= 20)
		return dist_1;

	// 16 이상일 경우, 최초 증가값은 1, 이후 4번 마다 증가값이 <<1 됨
	/*
		index	(index-16)	증가값	shift	(DIST-1)	
														
		16			0		1		0			16		
		17			1		1		0			17		
		18			2		1		0			18		
		19			3		1		0			19		

		20			4		2		1			20		
		21			5		2		1			22		
		22			6		2		1			24		
		23			7		2		1			26		

		24			8		4		2			28		
		25			9		4		2			32		
		26		   10		4		2			36		
		27		   11		4		2			40		

		28		   12		8		3			44		
		29		   13		8		3			52		
		30		   14		8		3			60		
		31		   15		8		3			68	
		....
	
	*/

	uint32_t shift = 0;
	uint32_t temp = ((dist - 1 - 16) / 4 + 1) >> 1;
	while (temp)
	{
		temp >>= 1;
		shift++;
	}

	const uint32_t index = 16 + (shift * 4) + ((dist - 1U - 16) - ((1U << shift) - 1) * 4) / (1 << shift);

	return index;
}

