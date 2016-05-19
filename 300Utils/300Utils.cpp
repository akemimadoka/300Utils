#include "stdafx.h"
#include "300Utils.h"
#include <cassert>
#include <zlib.h>
#include <memory>
#include <map>
#include <algorithm>

namespace
{
	class Pack
	{
	public:
		explicit Pack(LPCTSTR packName)
		{
			m_hFile = CreateFile(packName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (!m_hFile || m_hFile == INVALID_HANDLE_VALUE)
			{
				throw std::system_error(std::error_code(GetLastError(), std::system_category()), "Cannot open file.");
			}

			m_FileSize = GetFileSize(m_hFile, NULL);
			
			m_hMappedFile = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (!m_hMappedFile || m_hMappedFile == INVALID_HANDLE_VALUE)
			{
				throw std::system_error(std::error_code(GetLastError(), std::system_category()), "Cannot create mapping of file.");
			}

			m_pFile = static_cast<const byte*>(MapViewOfFile(m_hMappedFile, FILE_MAP_READ, 0, 0, 0));
			if (!m_pFile/* || IsBadReadPtr(m_pFile, m_FileSize)*/)
			{
				throw std::system_error(std::error_code(GetLastError(), std::system_category()), "Cannot map view of file.");
			}

			memcpy_s(&m_FileHeader, sizeof(FileHeader), m_pFile, sizeof(FileHeader));

			if (strcmp(m_FileHeader.header, "DATA1.0") != 0)
			{
				throw std::runtime_error("Header is bad.");
			}

			m_Nodes.resize(m_FileHeader.count);
			memcpy_s(m_Nodes.data(), m_FileHeader.count * sizeof(FileNode), m_pFile + sizeof(FileHeader), m_FileHeader.count * sizeof(FileNode));
		}

		Pack(Pack const& other) = delete;
		Pack(Pack&& other) = delete;
		Pack& operator=(Pack const& other) = delete;
		Pack& operator=(Pack&& other) = delete;

		~Pack()
		{
			CloseHandle(m_hMappedFile);
			CloseHandle(m_hFile);
		}

	private:
#pragma pack(1)
		struct FileNode
		{
			char name[MAX_PATH];
			uint32_t position;
			uint32_t compressed_size;
			uint32_t uncompressed_size;
			char reserved[32];
		};

		struct FileHeader
		{
			char header[50];
			uint32_t count;
		} m_FileHeader;
#pragma pack()

	public:
		class Iterator
		{
		public:
			Iterator()
				: m_pPack(nullptr)
			{
			}

			Iterator(const std::vector<FileNode>::const_iterator& iterator, const Pack* pPack)
				: m_InternalIterator(iterator),
				m_pPack(pPack)
			{
			}

			DWORD GetContent(byte* pOut, size_t bufferSize) const
			{
				if (bufferSize < m_InternalIterator->uncompressed_size)
				{
					return 0;
				}

				std::vector<byte> content(m_InternalIterator->uncompressed_size);
				DWORD RealSize;
				if (uncompress(content.data(), &RealSize, m_pPack->m_pFile + m_InternalIterator->position, m_InternalIterator->compressed_size) != Z_OK)
				{
					return 0;
				}

				if (bufferSize < RealSize)
				{
					return 0;
				}

				memcpy_s(pOut, bufferSize, content.data(), RealSize);
				return RealSize;
			}

			bool IsValid() const
			{
				return m_pPack != nullptr;
			}

			const Pack* GetPack() const noexcept
			{
				return m_pPack;
			}

			std::vector<FileNode>::const_iterator GetInternalIterator() const noexcept
			{
				return m_InternalIterator;
			}

			bool operator!=(Iterator const& other) const
			{
				return m_InternalIterator != other.m_InternalIterator || m_pPack != other.m_pPack;
			}

			bool operator==(Iterator const& other) const
			{
				return !(*this != other);
			}

			Iterator& operator++()
			{
				++m_InternalIterator;
				return *this;
			}

			FileNode const& operator*() const
			{
				return *m_InternalIterator;
			}

			const FileNode* operator->() const
			{
				return &*m_InternalIterator;
			}

		private:
			std::vector<FileNode>::const_iterator m_InternalIterator;
			const Pack* m_pPack;
		};

		Iterator begin() const
		{
			return Iterator(m_Nodes.begin(), this);
		}

		Iterator end() const
		{
			return Iterator(m_Nodes.end(), this);
		}

		Iterator IteratorAt(size_t location) const
		{
			return Iterator(next(m_Nodes.begin(), location), this);
		}

		size_t Count() const noexcept
		{
			return m_Nodes.size();
		}

	private:
		HANDLE m_hFile, m_hMappedFile;
		const byte* m_pFile;
		DWORD m_FileSize;

		std::vector<FileNode> m_Nodes;
	};

	std::vector<std::unique_ptr<Pack>> g_Packs;
	std::map<int, Pack::Iterator> g_Iterators;
}

int OpenPack(LPCTSTR packName)
{
	if (!packName)
	{
		return -1;
	}
	
	auto pPack = std::make_unique<Pack>(packName);
	for (auto iter = g_Packs.begin(); iter != g_Packs.end(); ++iter)
	{
		if (!*iter)
		{
			*iter = move(pPack);
			return distance(g_Packs.begin(), iter);
		}
	}

	g_Packs.emplace_back(move(pPack));
	return g_Packs.size() - 1;
}

void ClosePack(int pack)
{
	if (IsValidPack(pack))
	{
		g_Packs[pack] = nullptr;
	}
}

bool IsValidPack(int pack)
{
	return pack >= 0 && static_cast<size_t>(pack) < g_Packs.size() && g_Packs[pack];
}

size_t GetFileCount(int pack)
{
	assert(IsValidPack(pack) && "Pack is not initialized.");
	return g_Packs[pack]->Count();
}

int CreateIterator(int pack)
{
	assert(IsValidPack(pack) && "Pack is not initialized.");
	auto const& tpack = g_Packs[pack];
	int last = -1;
	for (auto& iter : g_Iterators)
	{
		if (iter.first - last > 1)
		{
			break;
		}
		last = iter.first;
	}
	g_Iterators[last + 1] = tpack->begin();
	
	return g_Iterators.size() - 1;
}

void CloseIterator(int iterator)
{
	if (IsValidIterator(iterator))
	{
		g_Iterators.erase(iterator);
	}
}

inline auto findPackFromPointer(const Pack* pPack)
{
	return std::find_if(g_Packs.begin(), g_Packs.end(), [pPack](auto const& pack)
	{
		return pack.get() == pPack;
	});
}

int GetPackFromIterator(int iterator)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	auto pPack = g_Iterators[iterator].GetPack();
	auto iter = findPackFromPointer(pPack);

	if (iter == g_Packs.end())
	{
		return -1;
	}
	
	return distance(g_Packs.begin(), iter);
}

bool MoveToNextFile(int iterator)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	auto& iter = g_Iterators[iterator];
	return iter == iter.GetPack()->end() ? false : ++iter != iter.GetPack()->end();
}

bool SeekIterator(int iterator, size_t dest)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	auto& iter = g_Iterators[iterator];
	
	if (dest >= iter.GetPack()->Count())
	{
		return false;
	}
	
	iter = iter.GetPack()->IteratorAt(dest);
	return true;
}

size_t TellIterator(int iterator)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	auto& iter = g_Iterators[iterator];

	return distance(iter.GetPack()->begin().GetInternalIterator(), iter.GetInternalIterator());
}

bool IsValidIterator(int iterator)
{
	auto iter = g_Iterators.find(iterator);
	return iter != g_Iterators.end() && iter->second.IsValid() && findPackFromPointer(iter->second.GetPack()) != g_Packs.end();
}

LPCSTR GetFileName(int iterator)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	return g_Iterators[iterator]->name;
}

size_t GetFileCompressedSize(int iterator)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	return g_Iterators[iterator]->compressed_size;
}

size_t GetFileUncompressedSize(int iterator)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	return g_Iterators[iterator]->uncompressed_size;
}

DWORD GetFileContent(int iterator, byte* pBuffer, DWORD dwBufferSize)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	return g_Iterators[iterator].GetContent(pBuffer, dwBufferSize);
}
