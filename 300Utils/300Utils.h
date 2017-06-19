#pragma once
#include <Windows.h>
#include <cstdint>

#ifdef UTILS_EXPORT
#	define UTILS_FUNC __declspec(dllexport)
#else
#	define UTILS_FUNC __declspec(dllimport)
#endif // UTILS_EXPORT

typedef uint8_t byte;
typedef uint32_t uint;
typedef uint64_t ulong;

extern "C"
{
	UTILS_FUNC int OpenPack(LPCTSTR packName);
	UTILS_FUNC void ClosePack(int pack);
	UTILS_FUNC bool IsValidPack(int pack);
	UTILS_FUNC size_t GetFileCount(int pack);
	UTILS_FUNC int CreateIterator(int pack);
	UTILS_FUNC void CloseIterator(int iterator);
	UTILS_FUNC int GetPackFromIterator(int iterator);
	UTILS_FUNC bool MoveToNextFile(int iterator);
	UTILS_FUNC bool SeekIterator(int iterator, size_t dest);
	UTILS_FUNC size_t TellIterator(int iterator);
	UTILS_FUNC bool IsValidIterator(int iterator);
	UTILS_FUNC LPCSTR GetFileName(int iterator);
	UTILS_FUNC void SetFileName(int iterator, LPCSTR name);
	UTILS_FUNC size_t GetFileCompressedSize(int iterator);
	UTILS_FUNC size_t GetFileUncompressedSize(int iterator);
	UTILS_FUNC DWORD GetFileContent(int iterator, byte* pBuffer, DWORD dwBufferSize);
	UTILS_FUNC DWORD SetFileContent(int iterator, const byte* pBuffer, DWORD dwBufferSize);

	UTILS_FUNC int AddFile(int pack, LPCSTR FileName, const byte* pBuffer, DWORD dwBufferSize);
	UTILS_FUNC void RemoveFile(int iterator);
}
