////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// DIST/LEN/POS 에 대한 최근 값 관리 클래스
/// 
/// @author   park
/// @date     Wed Aug  1 19:18:53 2018
/// 
/// Copyright(C) 2018 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

class XAzoHistory
{
	friend class XAzoDebug;
public :
	XAzoHistory(uint32_t size);
	~XAzoHistory();

	void		AddRecent(uint32_t data);
	uint32_t	GetDataAndUpdate(uint32_t index);

public :
	uint32_t*	m_data;
	uint32_t	m_size;

};

