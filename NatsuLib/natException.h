////////////////////////////////////////////////////////////////////////////////
///	@file	natException.h
///	@brief	�쳣���ͷ�ļ�
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natType.h"
#include <Windows.h>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
///	@brief	NatsuLib�쳣����
///	@note	�쳣�ɴ�������������ʹ�ÿ����׳��쳣�Ĵ��룬ʹ��pCausedbyʵ���쳣��
////////////////////////////////////////////////////////////////////////////////
class natException
{
public:
	natException(ncTStr Src, ncTStr Desc, const natException* pCausedby = nullptr) noexcept;
	natException(natException const& Other) noexcept;

	virtual ~natException() = default;

	nuInt GetTime() const noexcept;
	ncTStr GetSource() const noexcept;
	ncTStr GetDesc() const noexcept;
	const natException* GetCausedbyException() const noexcept;

protected:
	nuInt m_Time;
	nTString m_Source;
	nTString m_Description;
	std::shared_ptr<const natException> m_pCausedby = nullptr;
};

////////////////////////////////////////////////////////////////////////////////
///	@brief	NatsuLib WinAPI�����쳣
///	@note	���Զ�����LastErr��Ϣ
////////////////////////////////////////////////////////////////////////////////
class natWinException
	: public natException
{
public:
	natWinException(ncTStr Src, ncTStr Desc) noexcept;

	DWORD GetLastErr() const noexcept;

private:
	DWORD m_LastErr;
};