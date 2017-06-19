// GameInjector.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "GameInjector.h"
#include <natMisc.h>
#include <sstream>

typedef natScope<std::function<void()>> DefaultScope;

namespace
{
	constexpr byte g_WildCard[] = { '*' };
	constexpr byte g_GameInstance_GetPackFile_Sig[] = { 0x6A, 0xFF, 0x68, '*', '*', '*', '*', 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x50, 0x81, 0xEC, 0x80, 0x01, 0x00, 0x00 };
	constexpr byte g_New_Sig[] =
	{
		0x75, 0x05,						// jnz $ + 5
		0xB8, 0x01, 0x00, 0x00, 0x00,	// mov eax, 1
		0x50,							// push eax
		0xE8, '*', '*', '*', '*'		// call operator new[] (+0x9)
	};

	template <typename Memfunc>
	void* CastToRawPointer(Memfunc func)
	{
		union
		{
			std::decay_t<Memfunc> OriginFunctionPointer;
			void* RawPointer;
		} Result;

		Result.OriginFunctionPointer = func;
		return Result.RawPointer;
	}
}

GameInjector& GameInjector::GetInjector()
{
	static GameInjector s_Instance;
	return s_Instance;
}

void GameInjector::OnLoad()
{
	std::stringstream ss;
	*reinterpret_cast<GameInjector**>(HackCode + 2) = this;
	*reinterpret_cast<void**>(HackCode + 7) = CastToRawPointer(&GameInjector::HookReadPackFile);
	m_GameInstance_GetPackFile_Offset = reinterpret_cast<DWORD>(FindMemory(GetInstance(), nullptr, g_GameInstance_GetPackFile_Sig, g_WildCard, 1)) - reinterpret_cast<DWORD>(GetInstance());
	if (!m_GameInstance_GetPackFile_Offset)
	{
		throw std::runtime_error("Cannot find address of GetPackFile.");
	}
	GetCode(m_GameInstance_GetPackFile_Offset, m_GameInstance_GetPackFile_OriginBytes);
	InjectCode(m_GameInstance_GetPackFile_Offset, 5, HackCode);

	auto FoundNew = FindMemory(reinterpret_cast<void*>(reinterpret_cast<DWORD>(GetInstance()) + m_GameInstance_GetPackFile_Offset), nullptr, g_New_Sig, g_WildCard, 1);
	if (!FoundNew)
	{
		throw std::runtime_error("Cannot find address of operator new[].");
	}
	m_New_Offset = reinterpret_cast<DWORD>(FoundNew + 0xD + *reinterpret_cast<LPDWORD>(FoundNew + 0x9));

	ss << std::hex << m_GameInstance_GetPackFile_Offset << std::endl << m_New_Offset;
	MessageBoxA(NULL, ss.str().c_str(), "Test!", MB_OK);
	exit(EXIT_SUCCESS);
}

void GameInjector::OnUnload()
{
	ModifyCode(m_GameInstance_GetPackFile_Offset, 5, m_GameInstance_GetPackFile_OriginBytes);
}

DWORD GameInjector::HookReadPackFile(const char* FileName, UnknownFileObject* UnknownObject)
{
	m_CriSection.Lock();
	ModifyCode(m_GameInstance_GetPackFile_Offset, 5, m_GameInstance_GetPackFile_OriginBytes);
	DefaultScope scope([this]()
	{
		InjectCode(m_GameInstance_GetPackFile_Offset, 5, HackCode);
		m_CriSection.UnLock();
	});
	
	MessageBoxA(NULL, FileName, "Test", MB_OK);
	return CallCdeclFuncByOffset<DWORD>(m_GameInstance_GetPackFile_Offset, FileName, UnknownObject);
}
