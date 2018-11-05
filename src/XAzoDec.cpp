#include "stdafx.h"
#include "XAzoDec.h"
#include "_private.h"

#pragma warning(disable: 26495)

XAzoDec::XAzoDec() :
	m_historyShortDist(HISTORY_LEN_SHORT),
	m_historyShortLen(HISTORY_LEN_SHORT),
	m_historyLen(HISTORY_LEN_POS),
	m_historyPos(HISTORY_LEN_POS)
{
	m_inBuf = nullptr;
	m_inBufSize = 0;
	m_outBuf = nullptr;
	m_outBufSize = 0;
	m_literal = new XLiteralProb;
	m_probLen = new XLenProb;
	m_writePos = 0;
}

XAzoDec::~XAzoDec()
{
	delete m_literal;
	delete m_probLen;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         입/출력 버퍼 및 변수 초기화
/// @param  
/// @return 
/// @date   Wed Aug  1 14:55:30 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
void XAzoDec::Init(uint8_t* inBuf, uint32_t inBufSize, uint8_t* outBuf, uint32_t outBufSize)
{
	/////////////////////////////////////////////////////////////////////
	//
	// 버퍼 초기화
	m_inBuf = inBuf;
	m_inBufSize = inBufSize;
	m_outBuf = outBuf;
	m_outBufSize = outBufSize;
	m_writePos = 0;

	m_arith.Init(m_inBuf, inBufSize);

	/////////////////////////////////////////////////////////////////////
	//
	// 확률 변수 초기화
	m_literal->Init();
	m_probMatch.Init();
	m_probHistory.Init();
	m_probLen->Init();

	m_probDistShortHistoryBit.Init();
	m_probLenPosShortHistoryBit.Init();

	for (int i = 0; i < PROB_LEN_SHORTHISTORYINDEX; i++)
	{
		m_probDistShortHistoryIndex[i].Init(PROB_DEFAULT_BIT);
		m_probLenPosShortHistoryIndex[i].Init(PROB_DEFAULT_BIT);
	}

	for (int i = 0; i < PROB_LEN_LENPOS_HISTORY; i++)
		m_probLenPosLongHistoryIndex[i].Init(PROB_DEFAULT_BIT);

	for (int i = 0; i < PROB_LEN_DIST_INDEX; i++)
		m_probDistIndex[i].Init(PROB_DEFAULT_BIT);


	/////////////////////////////////////////////////////////////////////
	//
	// LEN/DIST 테이블 초기화

	for (int i = 0; i < HISTORY_LEN_SHORT; i++)
	{
		m_historyShortDist.m_data[i] = i + XAzoTable::MIN_DIST;	// short dist 초기화 하기
		m_historyShortLen.m_data[i] = i;						// short 2 long 초기화
	}

	for (int i = 0; i < HISTORY_LEN_POS; i++)				// history 초기화
	{
		m_historyLen.m_data[i] = i + XAzoTable::MIN_LEN;
		m_historyPos.m_data[i] = 0;
	}
}

bool XAzoDec::IsEOS()
{
	return (m_writePos >= m_outBufSize) ? true : false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         블럭 디코딩
/// @param  
/// @return 
/// @date   Wed Aug  1 14:55:39 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoDec::Decode()
{
	// 첫번째는 무조건 리터럴 디코딩하고
	if (DecodeLiteral() == false)
		ERROR_RETURN;

	uint32_t bit0 = 0, bit1 = 0;

	while (!IsEOS())
	{
		// 첫번째 비트(MATCH) 가져오고
		if (m_arith.DecodeBoolState(m_probMatch, bit0) == false)
			ERROR_RETURN;

		// 첫번째 비트가 0 이면 리터럴 처리
		if (bit0==0)
		{
			if (DecodeLiteral() == false)
				ERROR_RETURN;
		}
		// 첫번째 비트가 1 이면 MATCH
		else
		{
			// 두번째 비트 가져오기
			if (m_arith.DecodeBoolState(m_probHistory, bit1) == false)
				ERROR_RETURN;

			// 1 이면 history 가 있다.
			if (bit1 == 1)
			{
				// 1 + 1 + 1 + short_his
				// 1 + 1 + 0 + long_his
				if (DecodeLenPosHistory() == false)
					ERROR_RETURN;
			}
			// 0 이면 history 가 없다.
			else
			{
				// 1 + 0  + dist + len
				if (DecodeMatch() == false)
					ERROR_RETURN;
			}
		}
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         사전에서 가져오기
/// @param  
/// @return 
/// @date   Thu Jul 26 14:22:31 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t XAzoDec::Peek(const unsigned int distance) const
{
	// 현재 위치 뒤에서 가져오기
	if (m_writePos > distance)
		return m_outBuf[m_writePos - distance - 1];

	// 맨 처음 리터럴
	if (m_writePos == 0 && distance == 0)
		return 0;

	// 앞쪽에서 가져오기 -> AZO 는 사전이 접히지 않는다.  에러 상황
	ASSERT(0);
	return 0;
	//return m_dic[m_dicSize + m_writePos - distance - 1];
}

// 출력
bool XAzoDec::Write(uint8_t c)
{
	if (m_writePos >= m_outBufSize)
	{
		ASSERT(0);
		return false;
	}

	// 사전에 쓰고
	m_outBuf[m_writePos] = c;
	m_writePos++;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         리터럴 디코딩하기 . 두개의 PROB 에서 이전 데이타에 따라 하나를 선택 후 데이타를 디코딩한다.
/// @param  
/// @return 
/// @date   Wed Aug  1 15:05:10 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoDec::DecodeLiteral()
{
	// 일단 사전에서 이전 글자 가져오기
	const uint8_t prevByte = Peek(0);

	// literal_pos_state 에 대한 위키백과 설명
	//
	// - The pos_state and literal_pos_state values consist of respectively the pb and lp (up to 4, from the LZMA header or LZMA2 properties packet) 
	//   least significant bits of the dictionary position (the number of bytes coded since the last dictionary reset modulo the dictionary size). 
	//
	// lzip 의 설명 literal_state(==literal_pos_state): 최근 디코딩된 바이트의 MSB 3비트의 값
	// AZO 도 LZMA 와 동일하게 상위 3 비트를 사용한다.
	//

	const int literal_pos_state = prevByte >> XLiteralProb::SHIFT;


	// 먼저 이전 리터럴 값을 가지고 사용할 prob 값을 선택한다.
	XAzoProb* prob1 = m_literal->prob1[prevByte];
	XAzoProb* prob2 = m_literal->prob2[literal_pos_state];
	uint32_t		symbol = 0;


	// 글자 상태값 ..
	if (m_literal->state[prevByte] >= 0)
	{	
		// 선택된 prob1 에서 리터럴을 가져오고
		if(m_arith.DecodeTree(prob1, symbol, m_literal->BIT_LEN, m_literal->PROB_BITS, m_literal->PROB_MOVE_BITS)==false)
			ERROR_RETURN;

		// 가져온 리터럴을 가지고 prob2 에도 PROB 를 반영해 준다.
		m_arith.UpdateProb(prob2, symbol, m_literal->BIT_LEN, m_literal->PROB_BITS, m_literal->PROB_MOVE_BITS);
	}
	else
	{
		/*
		LZMA  SPEC 문서 에서 아래와 같은 조건과 비슷
		If (State > 7), the Literal Decoder also uses "matchByte" that represents
		the byte in OutputStream at position the is the DISTANCE bytes before
		current position, where the DISTANCE is the distance in DISTANCE-LENGTH pair
		of latest decoded match.
		*/

		// 여기로 오는 경우는 캐릭터 출력 상태가 아닌 경우가 높음?

		// 따라서 prob2 로 리터럴 가져오기 
		if(m_arith.DecodeTree(prob2, symbol, m_literal->BIT_LEN, m_literal->PROB_BITS, m_literal->PROB_MOVE_BITS)==false)
			ERROR_RETURN;

		// 가져온 리터럴을 가지고 prob1 에도 PROB 를 반영해 준다.
		m_arith.UpdateProb(prob1, symbol, m_literal->BIT_LEN, m_literal->PROB_BITS, m_literal->PROB_MOVE_BITS);
	}


	// 가져온 확률에 따라
	uint32_t prob1Tot = 0, prob2Tot = 0;
	m_arith.CalcProbSum(prob1, prob2, symbol, m_literal->BIT_LEN, m_literal->PROB_BITS, prob1Tot, prob2Tot);

	// state 바꾸기
	if (prob1Tot > prob2Tot) 
		m_literal->state[prevByte] ++;
	else if (prob1Tot < prob2Tot) 
		m_literal->state[prevByte] --;

	if (Write(uint8_t(symbol)) == false)
		ERROR_RETURN;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///		
///         MATCH 처리하기
/// @param  
/// @return 
/// @date   Wed Aug  1 17:57:47 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoDec::DecodeMatch()
{
	uint32_t dist = 0;
	uint32_t len = 0;

	// DIST가져오기
	if (DecodeDist(dist) == false)
		ERROR_RETURN;

	// LEN 가져오기
	if (DecodeLen(len, dist) == false)
		ERROR_RETURN;

	// HISTORY 에 저장
	m_historyLen.AddRecent(len);
	m_historyPos.AddRecent(m_writePos);	// 현재 위치를 저장!

	// 쓰기
	for (unsigned i = 0; i < len; i++)
	{
		if (Write(Peek(dist - 1)) == false)			// lzma 와 거리 계산 방식이 달라서 -1 해줘야 한다
			ERROR_RETURN;
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         DIST 스트림 가져오기
/// @param  
/// @return 
/// @date   Wed Aug  1 19:10:01 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoDec::DecodeDist(uint32_t& dist)
{
	uint32_t shortHistoryBit = 0;

	// DIST 값에 대한 SHORT HISTORY 존재 여부 비트 가져오기
	if (m_arith.DecodeBoolState(m_probDistShortHistoryBit, shortHistoryBit) == false)
		ERROR_RETURN;

	// DIST 에 대한 SHORT HISTORY 가 있으면?
	if (shortHistoryBit)
	{
		uint32_t distShortHistoryIndex = 0;

		// dist short history index 가져오기
		if (m_arith.DecodeTree(m_probDistShortHistoryIndex, distShortHistoryIndex, SHORT_HISTORY_BIT_LEN, PROB_DEFAULT_BIT, PROB_DEFAULT_SHIFT)==false)
			ERROR_RETURN;

		// index 로 최근 목록 참조 하기
		dist = m_historyShortDist.GetDataAndUpdate(distShortHistoryIndex);
	}
	// SHORT HISTORY 가 없으면?
	else
	{
		// DIST INDEX + EXTRABITS 가져오기
		uint32_t distIndex = 0;
		if (m_arith.DecodeTree(m_probDistIndex, distIndex, DIST_INDEX_BIT_LEN, PROB_DEFAULT_BIT, PROB_DEFAULT_SHIFT) == false)
			ERROR_RETURN;

		uint32_t distExtrBits = 0;
		dist = m_table.DistIndex2Dist(distIndex, distExtrBits);

		// 추가 비트 있으면 디코딩 해서 또 추가하고
		if (distExtrBits)
		{
			uint32_t distExtra = 0;
			if (m_arith.DecodeDirect(distExtra, distExtrBits) == false)
				ERROR_RETURN;
			dist += distExtra;
		}

		// 가져온 값은 short history 저장
		m_historyShortDist.AddRecent(dist);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         LEN 가져오기
/// @param  
/// @return 
/// @date   Wed Aug  1 19:30:00 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoDec::DecodeLen(uint32_t& len, uint32_t dist)
{
	// LZMA 는 LEN+DIST 순으로 가져오기 때문에 DIST 를 가져올때 LEN-kMatchMinLen 를 가지고 문맥을 만들어서 사용하는데,
	// AZO 는 DIST+LEN 순으로 가져오기 때문에 LEN 을 가져올때 DIST-MIN_DIST 를 가지고 문맥을 만든다.
	const uint32_t distIndex = m_table.GetDistIndex(dist);

	// LEN 은 LEN INDEX 부터 가져온다.
	uint32_t lenIndex = 0;
	if (DecodeLenIndex(lenIndex, distIndex) == false)
		ERROR_RETURN;

	uint32_t lenExtraBits = 0;

	// LEN 인덱스로 테이블 참조 
	len = m_table.LenIndex2Len(lenIndex, lenExtraBits);

	// 추가 비트 있으면 추가 비트 디코딩해서 붙이기
	if (lenExtraBits)
	{
		uint32_t lenExtra = 0;
		if (m_arith.DecodeDirect(lenExtra, lenExtraBits) == false)
			ERROR_RETURN;
		len += lenExtra;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         LEN INDEX 가져오기
/// @param  
/// @return 
/// @date   Thu Aug  2 15:30:43 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoDec::DecodeLenIndex(uint32_t& lenIndex, uint32_t distIndex)
{
	// 먼저 distIndex 값을 이전 상태값으로 사용한다. 
	XAzoProb* prob1 = m_probLen->prob1[distIndex];
	XAzoProb* prob2 = m_probLen->prob2[distIndex >> XLenProb::SHIFT];

	// 글자 상태값 ..
	if (m_probLen->state[distIndex] >= 0)
	{
		// 선택된 prob1 에서 리터럴을 가져오고
		if(m_arith.DecodeTree(prob1, lenIndex, m_probLen->BIT_LEN, m_probLen->PROB_BITS, m_probLen->PROB_MOVE_BITS)==false)
			ERROR_RETURN;

		// 가져온 리터럴을 가지고 prob2 에도 PROB 를 반영해 준다.
		m_arith.UpdateProb(prob2, lenIndex, m_probLen->BIT_LEN, m_probLen->PROB_BITS, m_probLen->PROB_MOVE_BITS);
	}
	else
	{
		// 따라서 prob2 로 리터럴 가져오기 
		if(m_arith.DecodeTree(prob2, lenIndex, m_probLen->BIT_LEN, m_probLen->PROB_BITS, m_probLen->PROB_MOVE_BITS)==false)
			ERROR_RETURN;

		// 가져온 리터럴을 가지고 prob1 에도 PROB 를 반영해 준다.
		m_arith.UpdateProb(prob1, lenIndex, m_probLen->BIT_LEN, m_probLen->PROB_BITS, m_probLen->PROB_MOVE_BITS);
	}

	// 가져온 확률에 따라
	uint32_t prob1Tot = 0, prob2Tot = 0;
	m_arith.CalcProbSum(prob1, prob2, lenIndex, m_probLen->BIT_LEN, m_probLen->PROB_BITS, prob1Tot, prob2Tot);

	// state 바꾸기
	if (prob1Tot > prob2Tot)
		m_probLen->state[distIndex] ++;
	else if (prob1Tot < prob2Tot)
		m_probLen->state[distIndex] --;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         LEN/POS history 처리하기
//				1 + 1 + 1 + short_his
//				1 + 1 + 0 + long_his
/// @param  
/// @return 
/// @date   Thu Aug  2 18:11:00 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoDec::DecodeLenPosHistory()
{
	// 세번째 비트 가져오기
	uint32_t shortHistoryBit = 0;
	uint32_t len = 0;
	uint32_t pos = 0;
	uint32_t historyIndex = 0;

	// LEN/POS 값에 대한 SHORT HISTORY 비트 가져오기
	if (m_arith.DecodeBoolState(m_probLenPosShortHistoryBit, shortHistoryBit) == false)
		ERROR_RETURN;

	// SHORT HISTORY 가 존재하는가?
	if (shortHistoryBit)
	{
		uint32_t lenShortIndex = 0;

		// len short history index 가져오기
		if (m_arith.DecodeTree(m_probLenPosShortHistoryIndex, lenShortIndex, SHORT_HISTORY_BIT_LEN, PROB_DEFAULT_BIT, PROB_DEFAULT_SHIFT) == false)
			ERROR_RETURN;

		// SHORT HISTORY INDEX 에 참조로 historyIndex 가져오고, 추가 작업도 진행
		historyIndex = m_historyShortLen.GetDataAndUpdate(lenShortIndex);
	}
	// SHORT HISTORY 가 없으면?
	else
	{
		// long history index 가져오기
		if (m_arith.DecodeTree(m_probLenPosLongHistoryIndex, historyIndex, LONG_HISTORY_BIT_LEN, PROB_DEFAULT_BIT, PROB_DEFAULT_SHIFT) == false)
			ERROR_RETURN;

		// SHORT HISTORY INDEX 에도 추가
		m_historyShortLen.AddRecent(historyIndex);
	}

	len = m_historyLen.GetDataAndUpdate(historyIndex);
	pos = m_historyPos.GetDataAndUpdate(historyIndex);

	// DIST 계산
	const uint32_t dist = m_writePos - pos;

	// 쓰기
	for (unsigned i = 0; i < len; i++)
	{
		if (Write(Peek(dist - 1)) == false)			// lzma 와 거리 계산 방식이 달라서 -1 해줘야 한다
			ERROR_RETURN;
	}

	return true;
}
