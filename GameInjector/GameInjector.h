#pragma once
#include <GenericInjector.h>
#include <natMultiThread.h>

using namespace NatsuLib;

struct UnknownFileObject
{
	byte* pData;
	size_t Size;

	bool Unknown;
};

class GameInjector final
	: public GenericInjector
{
public:
	GameInjector() = default;
	~GameInjector() = default;

	static GameInjector& GetInjector();

protected:
	void OnLoad() override;
	void OnUnload() override;

private:
	natCriticalSection m_CriSection;

	byte HackCode[22] =
	{
		0x51,								//push ecx //save ecx
		0xB9, 0x00, 0x00, 0x00, 0x00,		//mov ecx, 0x00000000
		0xB8, 0x00, 0x00, 0x00, 0x00,		//mov eax, 0x00000000
		0xFF, 0x74, 0x24, 0x08,				//push DWORD PTR [esp+0x8]
		0xFF, 0x74, 0x24, 0x08,				//push DWORD PTR [esp+0x8]
		0xFF, 0xD0,							//call eax
		0x59,								//pop ecx
	};
	DWORD m_GameInstance_GetPackFile_Offset;
	byte m_GameInstance_GetPackFile_OriginBytes[5];

	DWORD m_New_Offset;

	DWORD HookReadPackFile(const char* FileName, UnknownFileObject* UnknownObject);

	template <typename Ret, typename... Args>
	Ret CallCdeclFuncByOffset(DWORD Offset, Args&&... args)
	{
		return reinterpret_cast<Ret(_cdecl *)(Args...)>(reinterpret_cast<DWORD>(GetInstance()) + Offset)(std::forward<Args>(args)...);
	}

	template <typename Ret, typename... Args>
	Ret CallStdcallFuncByOffset(DWORD Offset, Args&&... args)
	{
		return reinterpret_cast<Ret(_stdcall *)(Args...)>(reinterpret_cast<DWORD>(GetInstance()) + Offset)(std::forward<Args>(args)...);
	}
};
