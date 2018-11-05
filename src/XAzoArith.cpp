#include "stdafx.h"
#include "XAzoArith.h"
#include "_private.h"

#pragma warning(disable: 6297)
#pragma warning(disable: 26451)
#pragma warning(disable: 26429)


XAzoArith::XAzoArith()
{
	STATE_SIZE = 32;
	MAX_RANGE = ((uint64_t)1) << STATE_SIZE;
	MIN_RANGE = (MAX_RANGE >> 2) + 2;
	MAX_TOTAL = std::min(std::numeric_limits<decltype(MAX_RANGE)>::max() / MAX_RANGE, MIN_RANGE);
	MASK = MAX_RANGE - 1;
	TOP_MASK = MAX_RANGE >> 1;
	SECOND_MASK = TOP_MASK >> 1;

	low = 0;
	high = MASK;
	code = 0;

	m_buf = NULL;
	m_bufRemain = 0;
	m_bufCur = nullptr;
	m_inbyteRemainBit = 0;
	m_readError = false;
}


XAzoArith::~XAzoArith()
{
}

void XAzoArith::Init(const uint8_t* buf, uint32_t size)
{
	// 입력 버퍼 초기화
	m_buf = buf;
	m_bufRemain = size;
	m_bufCur = buf-1;
	m_inbyteRemainBit = 0;
	m_readError = false;

	// 산술코딩 변수 초기화
	low = 0;
	high = MASK;
	code = 0;

	// 초기 코드 읽기
	for (int i = 0; i < STATE_SIZE; i++)
		code = code << 1 | readCodeBit();
}

// 비트 하나 읽기
int XAzoArith::readCodeBit()
{
	if (m_inbyteRemainBit == 0)
	{
		if (m_bufRemain)
		{
			m_inbyteRemainBit = 8;
			m_bufCur++;
			m_bufRemain--;
		}
		else
		{
			// 입력 버퍼가 다 떨어졌다.
			if (code == 0)
			{
				ASSERT(0);
				m_readError = true;
			}
			return 0;
		}
	}
	m_inbyteRemainBit--;
	const int bit = ((*m_bufCur) >> m_inbyteRemainBit) & 1;
	return bit;
}

void XAzoArith::shift() 
{
	code = ((code << 1) & MASK) | readCodeBit();
}

void XAzoArith::underflow()
{
	code = (code & TOP_MASK) | ((code << 1) & (MASK >> 1)) | readCodeBit();
}


void XAzoArith::update()
{
	// State check
	//if (low >= high || (low & MASK) != low || (high & MASK) != high)
	//	ASSERT(0);
	//	throw "Assertion error: Low or high out of range";

	//uint64_t range = high - low + 1;
	//if (range < MIN_RANGE || range > MAX_RANGE)
	//	throw "Assertion error: Range out of range";

	// Frequency table values check
	//uint32_t total = freqs.getTotal();
	//uint32_t symLow = freqs.getLow(symbol);
	//uint32_t symHigh = freqs.getHigh(symbol);
	//if (symLow == symHigh)
	//	throw "Symbol has zero frequency";
	//if (total > MAX_TOTAL)
	//	throw "Cannot code symbol because total is too large";

	// Update range
	//uint64_t newLow = low + symLow * range / total;
	//uint64_t newHigh = low + symHigh * range / total - 1;
	//low = newLow;
	//high = newHigh;

	// While the highest bits are equal
	while (((low ^ high) & TOP_MASK) == 0) {
		shift();
		low = (low << 1) & MASK;
		high = ((high << 1) & MASK) | 1;
	}

	// While the second highest bit of low is 1 and the second highest bit of high is 0
	while ((low & ~high & SECOND_MASK) != 0) {
		underflow();
		low = (low << 1) & (MASK >> 1);
		high = ((high << 1) & (MASK >> 1)) | TOP_MASK | 1;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         다이렉트 디코딩 - 고정확률 디코딩 LZMA 의 Direct Decoding 과 동일한 개념
///			서로 연관성이 없어서 압축이 불가능한 데이타는 이걸로 처리하는듯하다.
/// @param  
/// @return 
/// @date   Fri Jul 27 14:24:08 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoArith::DecodeDirect(uint32_t& symbol, int bitsLen)
{
	const uint64_t range = (high - low + 1) >> bitsLen;
	symbol = (uint32_t)((code - low) / range);
	high = low + range * (symbol + 1) - 1;
	low = low + range * symbol;
	update();
	return true;
}


// 비트 하나 디코딩하고, 확률 업데이트 하기 
// LZMA 의 RANGE CODING 처리 방식과 동일하다. - https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm 참고
// Context-based range decoding of a bit using the prob probability variable proceeds in this way:
bool XAzoArith::DecodeBit(XAzoProb& prob, uint32_t& bit, int PROB_BITS, int PROB_MOVE_BITS)
{
	// 비트 확률 계산하고
	const uint64_t range = (high - low + 1) >> PROB_BITS;
	const uint32_t bound = (uint32_t)((code - low) / ((high - low + 1) >> PROB_BITS));

	/* Decode our bit. */
	if (bound < prob.prob) // mid >= prob && prob >= low)
	{
		high = (uint32_t)(low + range * prob.prob - 1);
		bit = 0;

		// Set prob to prob + floor((2^11 - prob) / 2^5)
		prob.prob = prob.prob + ((1 << PROB_BITS) - prob.prob) / (1 << PROB_MOVE_BITS);
	}
	else //if (high >= prob && prob > mid)
	{
		low = (uint32_t)(low + range * prob.prob);
		bit = 1;

		// Set prob to prob - floor(prob / 2^5)
		prob.prob = prob.prob - (prob.prob / (1 << PROB_MOVE_BITS));
	}
	update();

	if (m_readError)
		return false;

	return true;
}

// boolstate - 1비트 가져오고, 보너스로 boolstate 의 state 도 업데이트 해준다.
bool XAzoArith::DecodeBoolState(SBoolProb& prob, uint32_t& bit)
{
	if (DecodeBit(prob.prob[prob.boolState], bit, prob.PROB_BITS, prob.PROB_SHIFT_BITS) == false)
		ERROR_RETURN;

	// state 업데이트
	// lzma 의 pos_state(posState) 랑 비트수만 다를뿐 작동은 동일하다.
	prob.boolState = ((prob.boolState << 1) & (prob.STATES-1)) + bit;
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
///         한번에 여러 비트 디코드
///			LZMA 의 DECODE TREE 와 동일하게 작동한다.
/// @return 
/// @date   Tue Aug  7 14:54:29 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
bool XAzoArith::DecodeTree(XAzoProb* prob, uint32_t& symbol, int bitsLen, int PROB_BITS, int PROB_MOVE_BITS)
{
	// 그냥 0 으로 해도 효율에는 차이 없을거 같은데 , LZMA SDK 에 그냥 1로 되어 있다. 따라하자.
	symbol = 1;

	uint32_t bit = 0;

	// 지정된 비트만큼
	for (int i = 0; i < bitsLen; i++)
	{
		// 변경된 심볼값에 따라 prob 의 선택도 바뀐다.
		if (DecodeBit(prob[symbol], bit, PROB_BITS, PROB_MOVE_BITS) == false)
			ERROR_RETURN;

		// 비트를 순서대로 가져온다. - 하위에 비트 추가
		symbol = (symbol << 1) + bit;
	}

	// 맨 처음에 1로 세팅했팅하고 시프트된 값 다시 빼주기
	symbol = symbol - (1 << bitsLen);

	return true;
}

// 비트를 가져오지는 않고, symbol 로 prob 값만 바꿔주기
bool XAzoArith::UpdateProb(XAzoProb* _prob, uint32_t _symbol, int bitsLen, int PROB_BITS, int PROB_MOVE_BITS) const
{
	uint32_t symbol = 1;
	const uint32_t MSB = 1 << (bitsLen - 1);		// 8비트인 경우 0x80

	// 지정된 비트만큼
	for (int i = 0; i < bitsLen; i++)
	{
		const uint32_t bit = (_symbol & MSB) ? 1 : 0;		// MSB 부터 LSB 로 가져온다.
		_symbol = _symbol << 1;

		XAzoProb& prob = _prob[symbol];

		// 가져온 비트에 따라서 확률값 바꿔주기
		if (bit == 0)
		{
			// Set prob to prob + floor((2^11 - prob) / 2^5)
			prob.prob = prob.prob + ((1 << PROB_BITS) - prob.prob) / (1 << PROB_MOVE_BITS);
		}
		else
		{
			// Set prob to prob - floor(prob / 2^5)
			prob.prob = prob.prob - (prob.prob / (1 << PROB_MOVE_BITS));
		}

		// 비트를 순서대로 가져온다. - 하위에 비트 추가
		symbol = (symbol << 1) + bit;
	}

	return true;
}


// 확률 구하기?
void XAzoArith::CalcProbSum(const XAzoProb* prob1, const XAzoProb* prob2, uint32_t symbol, int bitsLen, int PROB_BITS, uint32_t &probTot1, uint32_t &probTot2) const
{
	uint32_t prev = 1;		// 이전값은 1 로 시작
	const uint32_t MSB = 1 << (bitsLen - 1);		// 8비트인 경우 0x80
	const uint32_t maxProb = 1 << PROB_BITS;

	probTot1 = 1;
	probTot2 = 1;

	for (int i = 0; i < bitsLen; i++)
	{
		const uint32_t probCur1 = prob1[prev].prob;
		const uint32_t probCur2 = prob2[prev].prob;
		const uint32_t bit = (symbol & MSB) ? 1 : 0;		// 심볼의 비트를 MSB 부터 LSB 로 순서대로 가져와서
		symbol = symbol << 1;

		if ((probTot1 | probTot2) & 0xffc00000)				// 너무 크면 SHIFT>> 시켜주고
		{
			probTot1 >>= PROB_BITS;
			probTot2 >>= PROB_BITS;
		}

		probTot1 = probTot1 * (bit ? (maxProb - probCur1) : probCur1);	// 비트에 따라 적당히 곱해준다.
		probTot2 = probTot2 * (bit ? (maxProb - probCur2) : probCur2);

		prev = (prev << 1) + bit;										// 이전 값은 증가시키면서 순회시켜준다.
	}
}

