#include "stdafx.h"
#include "natException.h"

natException::natException(ncTStr Src, ncTStr Desc, const natException* pCausedby) noexcept
	: m_Time(GetTickCount()), m_Source(Src), m_Description(Desc), m_pCausedby(nullptr)
{
	if (pCausedby != nullptr)
	{
		// ReSharper disable once CppSmartPointerVsMakeFunction
		m_pCausedby = std::shared_ptr<const natException>(new(std::nothrow) natException(*pCausedby));
	}
}

natException::natException(natException const & Other) noexcept
	: m_Time(Other.m_Time), m_Source(Other.m_Source), m_Description(Other.m_Description), m_pCausedby(Other.m_pCausedby)
{
}

nuInt natException::GetTime() const noexcept
{
	return m_Time;
}

ncTStr natException::GetSource() const noexcept
{
	return m_Source.c_str();
}

ncTStr natException::GetDesc() const noexcept
{
	return m_Description.c_str();
}

const natException* natException::GetCausedbyException() const noexcept
{
	return m_pCausedby.get();
}

natWinException::natWinException(ncTStr Src, ncTStr Desc) noexcept
	: natException(Src, Desc, nullptr), m_LastErr(GetLastError())
{
	nTChar tErrnoStr[16] = { _T('\0') };
	_itot_s(m_LastErr, tErrnoStr, 10);
	m_Description = m_Description + _T(" ( LastErr=") + tErrnoStr + _T(" )");
}

DWORD natWinException::GetLastErr() const noexcept
{
	return m_LastErr;
}