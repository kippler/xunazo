#include "stdafx.h"
#include "XAzoHistory.h"

XAzoHistory::XAzoHistory(uint32_t size)
{
	m_size = size;
	m_data = new uint32_t[size];
}

XAzoHistory::~XAzoHistory()
{
	delete[]m_data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         데이타를 리턴하고, 해당 데이타를 가장 최근 위치로 옮긴다.
/// @param  
/// @return 
/// @date   Wed Aug  1 19:24:54 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t XAzoHistory::GetDataAndUpdate(uint32_t index)
{
	if(index>=m_size) 
	{
		ASSERT(0);
		return 0;
	}

	const uint32_t ret = m_data[index];

	// 뒤로 하나씩 밀기
	for (uint32_t i = index; i > 0; i--)
	{
		m_data[i] = m_data[i - 1];
	}

	// 맨 앞에 넣기
	m_data[0] = ret;

	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///         데이타를 하나씩 뒤로 밀고 맨 앞에 현재 데이타를 넣는다.
/// @param  
/// @return 
/// @date   Thu Aug  2 15:46:37 2018
////////////////////////////////////////////////////////////////////////////////////////////////////
void XAzoHistory::AddRecent(uint32_t data)
{
	// 뒤로 하나씩 밀기
	for (uint32_t i = m_size-1; i > 0; i--)
	{
		m_data[i] = m_data[i - 1];
	}

	// 맨 앞에 넣기
	m_data[0] = data;
}

