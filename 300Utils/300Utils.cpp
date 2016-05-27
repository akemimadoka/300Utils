#include "stdafx.h"
#include "300Utils.h"
#include <natStream.h>
#include <cassert>
#include <zlib.h>
#include <memory>
#include <map>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <list>

namespace
{
	class Pack
	{
	public:
		explicit Pack(LPCTSTR packName)
			: m_Edited(false)
		{
			HANDLE hTmpfile = CreateFile(packName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
			DWORD LastError = GetLastError();
			CloseHandle(hTmpfile);

			m_FileStream = std::move(make_ref<natFileStream>(packName, true, true));

			if (LastError != ERROR_FILE_NOT_FOUND)
			{
				m_pMappedFile = std::move(m_FileStream->MapToMemoryStream());
				memcpy_s(&m_FileHeader, sizeof(FileHeader), m_pMappedFile->GetInternalBuffer(), sizeof(FileHeader));

				if (strcmp(m_FileHeader.header, "DATA1.0") != 0)
				{
					throw std::runtime_error("Header is bad.");
				}

				auto pFileNodeBegin = reinterpret_cast<const FileNode*>(m_pMappedFile->GetInternalBuffer() + sizeof(FileHeader));
				auto pFileNodeEnd = pFileNodeBegin + m_FileHeader.count;
				m_Nodes.resize(m_FileHeader.count);
				m_Nodes.assign(pFileNodeBegin, pFileNodeEnd);

				// 避免其他工具修改过包未保持节点有序的问题
				m_Nodes.sort([this](FileNode const& node1, FileNode const& node2)
				{
					return node1.position < node2.position || !((m_Edited = true));
				});
			}
			else
			{
				FileHeader fh { 0 };
				strcpy_s(fh.header, "DATA1.0");

				m_FileStream->WriteBytes(reinterpret_cast<ncData>(&fh), sizeof(FileHeader));
				m_FileStream->SetPosition(NatSeek::Beg, 0);

				m_pMappedFile = std::move(m_FileStream->MapToMemoryStream());
			}
		}

		Pack(Pack const& other) = delete;
		Pack(Pack&& other) = delete;
		Pack& operator=(Pack const& other) = delete;
		Pack& operator=(Pack&& other) = delete;

		~Pack()
		{
			if (m_Edited)
			{
				CommitOperation();
			}
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

		///	@brief	对包中文件的迭代器包装
		///	@note	包中文件应以文件实际位置为基准始终保持有序
		class Iterator
		{
		public:
			Iterator()
				: m_pPack(nullptr)
			{
			}

			Iterator(std::list<FileNode>::iterator iterator, Pack* pPack)
				: m_InternalIterator(iterator),
				m_pPack(pPack)
			{
			}

			///	@brief	获得指向的文件的内容
			DWORD GetContent(byte* pOut, size_t bufferSize) const noexcept
			{
				if (bufferSize < m_InternalIterator->uncompressed_size)
				{
					return 0;
				}

				std::vector<byte> content(m_InternalIterator->uncompressed_size);
				DWORD RealSize;
				if (uncompress(content.data(), &RealSize, m_pPack->m_pMappedFile->GetInternalBuffer() + m_InternalIterator->position, m_InternalIterator->compressed_size) != Z_OK)
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

			///	@brief	检查边界是否可容纳Size个字节
			bool BorderCheck(size_t Size) const noexcept
			{
				return m_InternalIterator != prev(m_pPack->m_Nodes.end()) ? next(m_InternalIterator)->position - m_InternalIterator->position >= Size : m_pPack->m_FileStream->GetSize() - m_InternalIterator->position > static_cast<nLen>(Size);
			}

			__declspec(deprecated("Internal use"))
			void _Unchecked_SetContent(const byte* pBuffer, size_t bufferSize, size_t originSize) noexcept
			{
				memcpy_s(m_pPack->m_pMappedFile->GetInternalBuffer() + m_InternalIterator->position, m_InternalIterator != prev(m_pPack->m_Nodes.end()) ? next(m_InternalIterator)->position - m_InternalIterator->position : static_cast<size_t>(m_pPack->m_FileStream->GetSize() - m_InternalIterator->position), pBuffer, bufferSize);
				m_InternalIterator->compressed_size = bufferSize;
				m_InternalIterator->uncompressed_size = originSize;
			}

			///	@brief	修改指向的文件的内容
			bool SetContent(const byte* pBuffer, size_t bufferSize, DWORD& RealSize) noexcept
			{
				DWORD compressed_size = compressBound(bufferSize);
				std::vector<byte> content(compressed_size);
				if (compress(content.data(), &RealSize, pBuffer, bufferSize) != Z_OK)
				{
					return false;
				}

				if (!BorderCheck(RealSize))
				{
					return false;
				}

				#pragma warning(suppress : 4996)
				_Unchecked_SetContent(content.data(), bufferSize, RealSize);
				m_pPack->SetEdited();
				return true;
			}

			void Rename(const char* Name)
			{
				if (!Name)
				{
					throw std::invalid_argument("Name is nullptr.");
				}

				memcpy_s(m_InternalIterator->name, sizeof m_InternalIterator->name, Name, strlen(Name) + 1);
				m_pPack->SetEdited();
			}

			bool IsValid() const noexcept
			{
				return m_pPack != nullptr;
			}

			Pack* GetPack() const noexcept
			{
				return m_pPack;
			}

			std::list<FileNode>::iterator GetInternalIterator() const noexcept
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
			std::list<FileNode>::iterator m_InternalIterator;
			Pack* m_pPack;
		};

		Iterator begin()
		{
			return Iterator(m_Nodes.begin(), this);
		}

		Iterator end()
		{
			return Iterator(m_Nodes.end(), this);
		}

		Iterator IteratorAt(size_t location)
		{
			return Iterator(next(m_Nodes.begin(), location), this);
		}

		size_t Count() const noexcept
		{
			return m_Nodes.size();
		}

		void SetEdited(bool Value = true) noexcept
		{
			m_Edited = Value;
		}

		bool IsEdited() const noexcept
		{
			return m_Edited;
		}

		///	@brief		寻找文件中足够容纳Size个字节的空隙
		///	@return		超过文件末尾的长度
		uint32_t FindCave(size_t StartFile, size_t Size, uint32_t& Position) const noexcept
		{
			auto i = next(m_Nodes.begin(), StartFile), end = prev(m_Nodes.end());
			for (; i != end; ++i)
			{
				if (next(i)->position - i->position - i->compressed_size >= Size)
				{
					break;
				}
			}

			Position = i->position + i->compressed_size;
			return static_cast<uint32_t>(Position > m_FileStream->GetSize() - Size ? Position - (m_FileStream->GetSize() - Size) : 0);
		}

		///	@brief		新增文件
		///	@warning	未测试
		Iterator AddFile(LPCSTR FileName, const byte* Content, uint32_t ContentSize)
		{
			if (!FileName)
			{
				throw std::invalid_argument("FileName is nullptr.");
			}

			uint32_t Position;
			if (Expand(FindCave(0, ContentSize, Position)))
			{
				throw std::runtime_error("Failed to expand.");
			}

			FileNode node {0};
			strcpy_s(node.name, FileName);
			node.position = Position;

			auto iter = m_Nodes.insert(std::find_if(m_Nodes.begin(), m_Nodes.end(), [Position](FileNode const& tnode)
			{
				return tnode.position >= Position;
			}), node);

			Iterator ret(iter, this);
			if (Content)
			{
				DWORD RealSize;
				if (!ret.SetContent(Content, ContentSize, RealSize))
				{
					throw std::runtime_error("SetContent failed.");
				}
			}

			m_Edited = true;
			return ret;
		}

		///	@brief		移除文件
		///	@warning	未测试
		void RemoveFile(Iterator iterator)
		{
			m_Nodes.erase(iterator.GetInternalIterator());
			m_Edited = true;
		}

		/// @brief		将文件节点指向指定的偏移位置
		///	@note		简单的修改指向，不会进行额外操作如复制原文件内容到新位置
		///	@warning	不会进行合法性检测，请确保这个位置由FindCave获得
		Iterator AssociateFileNodeTo(Iterator FileNodeIterator, uint32_t Position)
		{
			FileNode fileNode = *FileNodeIterator;
			fileNode.position = Position;
			m_Nodes.erase(FileNodeIterator.GetInternalIterator());
			auto ret = m_Nodes.insert(std::find_if(m_Nodes.begin(), m_Nodes.end(), [&fileNode](FileNode const& tnode)
			{
				return tnode.position >= fileNode.position;
			}), fileNode);

			m_Edited = true;
			return Iterator(ret, this);
		}

		///	@brief		扩大包大小
		///	@return		操作是否成功
		///	@note		若size大于0且成功则会重置m_pMappedFile
		///				不能用于缩小包大小
		bool Expand(uint32_t size)
		{
			if (m_Edited)
				CommitOperation();
			return size && NATOK(m_FileStream->SetSize(m_FileStream->GetSize() + size)) && !!((m_pMappedFile = m_FileStream->MapToMemoryStream()));
		}

		///	@brief		提交操作
		void CommitOperation()
		{
			reinterpret_cast<FileHeader*>(m_pMappedFile->GetInternalBuffer())->count = m_FileHeader.count = m_Nodes.size();
			FileNode* pFileNodeBegin = reinterpret_cast<FileNode*>(m_pMappedFile->GetInternalBuffer() + sizeof(FileHeader));
			FileNode* pFileNodeEnd = reinterpret_cast<FileNode*>(m_pMappedFile->GetInternalBuffer() + m_Nodes.begin()->position);
			if (pFileNodeEnd - pFileNodeBegin < static_cast<int>(m_Nodes.size()))
			{
				throw std::out_of_range("Too many nodes.");
			}

			// 这个操作不应出现，对节点的操作都应该保持列表总是有序的
#ifdef _DEBUG
			m_Nodes.sort([](FileNode const& node1, FileNode const& node2)
			{
				return node1.position < node2.position;
			});
#endif

			for (auto const& fileNode : m_Nodes)
			{
				*pFileNodeBegin++ = fileNode;
			}

			m_FileStream->Flush();
		}

	private:
		natRefPointer<natFileStream> m_FileStream;
		natRefPointer<natMemoryStream> m_pMappedFile;
		bool m_Edited;

		///	@brief		key为Position
		std::list<FileNode> m_Nodes;
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

int findAvailableIteratorIndex()
{
	int last = -1;
	for (auto& iter : g_Iterators)
	{
		if (iter.first - last > 1)
		{
			break;
		}
		last = iter.first;
	}

	return ++last;
}

int CreateIterator(int pack)
{
	assert(IsValidPack(pack) && "Pack is not initialized.");
	auto const& tpack = g_Packs[pack];
	g_Iterators[findAvailableIteratorIndex()] = tpack->begin();
	
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

void SetFileName(int iterator, LPCSTR name)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	g_Iterators[iterator].Rename(name);
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

DWORD SetFileContent(int iterator, const byte* pBuffer, DWORD dwBufferSize)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	auto& iter = g_Iterators[iterator];
	std::vector<byte> compressedbuffer(compressBound(dwBufferSize));
	DWORD RealSize;
	if (compress(compressedbuffer.data(), &RealSize, pBuffer, dwBufferSize) != Z_OK)
	{
		return 0;
	}

	if (!iter.BorderCheck(RealSize))
	{
		uint32_t ExpandSize, Position;
		iter = iter.GetPack()->AssociateFileNodeTo(iter, ((ExpandSize = iter.GetPack()->FindCave(0, RealSize, Position)) > 0 && iter.GetPack()->Expand(ExpandSize), Position));
	}

	#pragma warning(suppress : 4996)
	iter._Unchecked_SetContent(compressedbuffer.data(), RealSize, dwBufferSize);
	iter.GetPack()->SetEdited();

	return dwBufferSize;
}

int AddFile(int pack, LPCSTR FileName, const byte* pBuffer, DWORD dwBufferSize)
{
	assert(IsValidPack(pack) && "Pack is not initialized.");
	assert(FileName && "FileName should be a valid pointer.");
	auto& tpack = g_Packs[pack];
	int Index = findAvailableIteratorIndex();
	g_Iterators[Index] = tpack->AddFile(FileName, pBuffer, dwBufferSize);

	return Index;
}

void RemoveFile(int iterator)
{
	assert(IsValidIterator(iterator) && "Invalid iterator.");
	auto const& iter = g_Iterators[iterator];
	iter.GetPack()->RemoveFile(iter);
}
