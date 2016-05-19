////////////////////////////////////////////////////////////////////////////////
///	@file	natException.h
///	@brief	异常相关头文件
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "natType.h"
#include <Windows.h>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
///	@brief	NatsuLib异常基类
///	@note	异常由此类派生，请勿使用可能抛出异常的代码，使用pCausedby实现异常链
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
///	@brief	NatsuLib WinAPI调用异常
///	@note	会自动附加LastErr信息
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