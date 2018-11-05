/*
	AZO 용 산술 디코더 

	- 다음 링크의 산술 디코더를 AZO 용으로 개조 https://github.com/nayuki/Reference-arithmetic-coding
	- 디코더 자체는 산술 디코더이지만, 인터페이의 많은 부분의 작동이 LZMA 의 RANGE DECODER 와 동일하기 때문에, 많은 코드를 xunlzip 에서 가져왔다.
	- 아래는 산술 디코더의 오리지날 라이선스

	==========================================

	License
	-------

	Copyright © 2018 Project Nayuki. (MIT License)

	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
	the Software, and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:

	* The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	* The Software is provided "as is", without warranty of any kind, express or
	implied, including but not limited to the warranties of merchantability,
	fitness for a particular purpose and noninfringement. In no event shall the
	authors or copyright holders be liable for any claim, damages or other
	liability, whether in an action of contract, tort or otherwise, arising from,
	out of or in connection with the Software or the use or other dealings in the
	Software.

	기타 산술 디코더 관련 링크
		https://github.com/ldematte/arithmetic_coding/blob/master/arithmetic_decoder.h
		https://marknelson.us/assets/1991-02-01-arithmetic-coding-statistical-modeling-data-compression/coder.c
		http://michael.dipperstein.com/arithmetic/index.html

*/

#pragma once


// 비트가 0 일 확률 데이타
// 2^10 이 최대값이며 0.5 에 해당하는 중간값은 2^9(512)
struct XAzoProb
{
	XAzoProb()
	{
		prob = 0; //  (1 << (AZO_PROB_BITS - 1));
	}
	void Init(int probBits) { prob = (1 << (probBits - 1)); }
	uint32_t& operator()() { return prob; }
	uint32_t prob;
};



// 1비트 테이타에 대한 확률 
struct SBoolProb
{
	SBoolProb(){boolState = 0;}
	enum { STATES = 256, PROB_BITS = 12, PROB_SHIFT_BITS = 6 };
	XAzoProb	prob[STATES];		// 확률 (1비트)
	uint32_t	boolState;			// 현재 상태
	void Init()
	{
		for (int i = 0; i < STATES; i++)
			prob[i].Init(PROB_BITS);
		boolState = 0;
	}
};



class XAzoArith
{
	friend class XAzoDebug;
public :
	XAzoArith();
	~XAzoArith();
	void			Init(const uint8_t* buf, uint32_t size);
	bool			DecodeDirect(uint32_t& symbol, int bitsLen);
	bool			DecodeTree(XAzoProb* prob, uint32_t& symbol, int bitsLen, int PROB_BITS, int PROB_MOVE_BITS);
	bool			UpdateProb(XAzoProb* prob, uint32_t _symbol, int bitsLen, int PROB_BITS, int PROB_MOVE_BITS) const;
	void			CalcProbSum(const XAzoProb* prob1, const XAzoProb* prob2, uint32_t symbol, int bitsLen, int PROB_BITS, uint32_t &probTot1, uint32_t &probTot2) const;
	bool			DecodeBoolState(SBoolProb& prob, uint32_t& bit);

private :
	bool			DecodeBit(XAzoProb& prob, uint32_t& bit, int PROB_BITS, int PROB_MOVE_BITS);

private :
	void			shift();
	void			update();
	int				readCodeBit();
	void			underflow();

private :
	int				STATE_SIZE;
	uint64_t		MAX_RANGE;		// Maximum range (high+1-low) during coding (trivial), which is 2^STATE_SIZE = 1000...000.
	uint64_t		MIN_RANGE;		// Minimum range (high+1-low) during coding (non-trivial), which is 0010...010.
	uint64_t		MAX_TOTAL;		// Maximum allowed total from a frequency table at all times during coding.
	uint64_t		MASK;			// Bit mask of STATE_SIZE ones, which is 0111...111.
	uint64_t		TOP_MASK;		// The top bit at width STATE_SIZE, which is 0100...000.
	uint64_t		SECOND_MASK;	// The second highest bit at width STATE_SIZE, which is 0010...000. This is zero when STATE_SIZE=1.

public :
	uint64_t		code;
	uint64_t		low;
	uint64_t		high;

	//	입력 데이타 - AZO 는 입력 스트림이 아니라, 항상 고정 버퍼이다.
	const uint8_t*	m_buf;					// 입력 버퍼
	uint32_t		m_bufRemain;			// 남아 있는 버퍼 크기
	const uint8_t*	m_bufCur;				// 읽고 있는 버퍼 위치
	int				m_inbyteRemainBit;		// 현재 바이트에 남아 있는 비트 수
	bool			m_readError;			// 입력 에러 발생
};

