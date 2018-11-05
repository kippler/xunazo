////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// LEN / DIST 테이블
/// 
/// @author   park
/// @date     Thu Aug  2 11:43:53 2018
/// 
/// Copyright(C) 2018 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class XAzoTable
{
public :
	XAzoTable();
	~XAzoTable();

public :
	uint32_t		LenIndex2Len(uint32_t index, uint32_t &lenExtraBits) const;
	uint32_t		DistIndex2Dist(uint32_t index, uint32_t &lenExtraBits) const;

public :
	enum { MIN_DIST = 1 };					// 최소 거리값
	enum { MIN_LEN = 2 };					// 최소 길이
	enum { MAX_TABLE = 128};

public :
	uint32_t		GetDistIndex(uint32_t dist) const;

private :
	uint32_t		lenTable[128];			// len 기본값 
	uint32_t		distTable[128];			// dist 기본값 
	uint32_t		lenExtraBits[128];		// 추가 bits
	uint32_t		distExtraBits[128];		// 추가 bits

};
