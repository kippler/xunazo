////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// AZO 비트스트림 디코더
/// 
/// @author   park
/// @date     Wed Aug  1 14:44:16 2018
/// 
/// Copyright(C) 2018 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "XAzoArith.h"
#include "XAzoHistory.h"
#include "XAzoTable.h"
#include "XStream.h"

#pragma warning(push)
#pragma warning(disable: 26495)

// 리터럴 확률 데이타
struct XLiteralProb
{
	void Init()
	{
		for (int i = 0; i < SIZE; i++) state[i] = 0;

		for (int i = 0; i < PREV_STATE; i++)
			for (int j = 0; j < SIZE; j++) 
				prob1[i][j].Init(PROB_BITS);

		for (int i = 0; i < (PREV_STATE >> SHIFT); i++)
			for (int j = 0; j < SIZE ; j++) 
				prob2[i][j].Init(PROB_BITS);
	}
	enum { PREV_STATE = 256 };		// 이전 상태 선택값
	enum { BIT_LEN = 8 };			// 8 비트
	enum { SIZE = (1 << BIT_LEN) };	// 256
	enum { SHIFT = 5 };				// 
	enum { PROB_BITS = 10, PROB_MOVE_BITS = 4 };
	XAzoProb	prob1[PREV_STATE][SIZE];
	XAzoProb	prob2[PREV_STATE >> SHIFT][SIZE];
	int			state[SIZE];		// 상태값
};


// LEN 확률 데이타
struct XLenProb
{
	void Init()
	{
		for (int i = 0; i < SIZE; i++) state[i] = 0;

		for (int i = 0; i < PREV_STATE; i++)
			for (int j = 0; j < SIZE; j++)
				prob1[i][j].Init(PROB_BITS);

		for (int i = 0; i < (PREV_STATE >> SHIFT); i++)
			for (int j = 0; j < SIZE; j++)
				prob2[i][j].Init(PROB_BITS);
	}
	enum { PREV_STATE = 128 };		// 이전 상태 선택값
	enum { BIT_LEN = 7 };			// LEN 은 7 비트
	enum { SIZE = (1 << BIT_LEN) };	// 256
	enum { SHIFT = 4 };				// LEN 의 SHIFT 는 4
	enum { PROB_BITS = 10, PROB_MOVE_BITS = 4 };
	XAzoProb	prob1[PREV_STATE][SIZE];
	XAzoProb	prob2[PREV_STATE >> SHIFT][SIZE];
	int			state[SIZE];		// 상태값
};

#pragma warning(pop)




class XAzoDec
{
	friend class XAzoDebug;
public :
	XAzoDec();
	~XAzoDec();
	void			Init(uint8_t* inBuf, uint32_t inBufSize, uint8_t* outBuf, uint32_t outBufSize);
	bool			Decode();

private :
	uint8_t			Peek(const unsigned int distance) const;
	bool			DecodeLiteral();
	bool			IsEOS();
	bool			Write(uint8_t c);
	bool			DecodeMatch();
	bool			DecodeDist(uint32_t& dist);
	bool			DecodeLen(uint32_t& len, uint32_t dist);
	bool			DecodeLenIndex(uint32_t& lenIndex, uint32_t distIndex);
	bool			DecodeLenPosHistory();

private :
	uint8_t *		m_inBuf;
	uint32_t		m_inBufSize;
	uint8_t*		m_outBuf;
	uint32_t		m_outBufSize;
	uint32_t		m_writePos;				// m_outBuf 배열에 대한 현재 "쓸" 위치

private :
	XAzoArith		m_arith;
	XLiteralProb*	m_literal;
	SBoolProb		m_probMatch;					// (첫번째) MATCH 비트에 대한 확률 + 상태
	SBoolProb		m_probHistory;					// (두번째) HISTORY(LONG+SHORT) 비트에 대한 확률 + 상태
	SBoolProb		m_probDistShortHistoryBit;		// DIST 값에 대한 SHORT HISTORY 여부 + 상태
	SBoolProb		m_probLenPosShortHistoryBit;	// LEN/POS 값에 대한 SHORT HISTORY 여부 + 상태
	XLenProb		*m_probLen;						// LEN INDEX 갑에 대한 확률


private :
	enum { SHORT_HISTORY_BIT_LEN  = 1 };			// DIST 와 LEN/POS 의 SHORT HISTORY 는 1비트 2개짜리 배열에 저장된다.
	enum { LONG_HISTORY_BIT_LEN = 7 };				// LEN/POS 가 저장되는 LONG HISTORY 는 128 개짜리 배열에 저장된다.
	enum { DIST_INDEX_BIT_LEN = 7 };				// DIST INDEX 는 7 비트(128개)
	enum { PROB_LEN_SHORTHISTORYINDEX = 2 };
	enum { PROB_LEN_LENPOS_HISTORY = 128 };
	enum { PROB_LEN_DIST_INDEX = 128 };
	enum { PROB_DEFAULT_BIT = 10 };
	enum { PROB_DEFAULT_SHIFT = 4 };

	XAzoProb		m_probDistShortHistoryIndex[PROB_LEN_SHORTHISTORYINDEX];	// DIST 값에 대한 SHORT HISTORY 의 INDEX - 2개(==1<<2)
	XAzoProb		m_probLenPosShortHistoryIndex[PROB_LEN_SHORTHISTORYINDEX];	// LEN/DIST 값에 대한 SHORT INDEX - 2개(1<<1)
	XAzoProb		m_probLenPosLongHistoryIndex[PROB_LEN_LENPOS_HISTORY];		// LEN/DIST 값에 대한 HISTTORY INDEX - 128개(1<<7)
	XAzoProb		m_probDistIndex[PROB_LEN_DIST_INDEX];						// DIST INDEX 값에 대한 PROB - 128개

private :
	enum { HISTORY_LEN_SHORT = 2 };
	enum { HISTORY_LEN_POS = 128 };
	XAzoHistory		m_historyShortDist;		// 거리에 대한 short history
	XAzoHistory		m_historyShortLen;		// LEN/POS 의 index 에 대한 short history
	XAzoHistory		m_historyLen;			// LEN/POS 히스토리 - index 로 참조함
	XAzoHistory		m_historyPos;			// ..
	XAzoTable		m_table;				// 상수 테이블
};


