#pragma once
#include "natStream.h"
#include <future>

enum class PipeDirection
{
	In = 1,
	Out = 2,
	Inout = In | Out,
};

enum class PipeMode
{
	Byte,
	Message,
};

enum class PipeOptions
{
	None = 0,

	Asynchronous = 1,
	WriteThrough = 2,
};

class natNamedPipeServerStream
	: public natStream
{
public:
	enum : nuInt
	{
		MaxAllowedServerInstances = std::numeric_limits<nuInt>::max(),
	};

	natNamedPipeServerStream(ncTStr Pipename, PipeDirection Direction, nuInt MaxInstances, nuInt OutBuffer = 1024, nuInt InBuffer = 1024, nuInt TimeOut = 0, PipeMode TransmissionMode = PipeMode::Message, PipeMode ReadMode = PipeMode::Message, PipeOptions Options = PipeOptions::WriteThrough);
	~natNamedPipeServerStream();

	NatErr GetLastErr() const override
	{
		return m_LastErr;
	}

	nBool CanWrite() const override
	{
		return m_bWritable;
	}

	nBool CanRead() const override
	{
		return m_bReadable;
	}

	nBool CanResize() const override
	{
		return false;
	}

	nBool CanSeek() const override
	{
		return false;
	}

	nLen GetSize() const override
	{
		return 0ul;
	}

	nResult SetSize(nLen /*Size*/) override
	{
		return m_LastErr = NatErr_NotSupport;
	}

	nLen GetPosition() const override
	{
		return 0ul;
	}

	nResult SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override
	{
		return m_LastErr = NatErr_NotSupport;
	}

	nLen ReadBytes(nData pData, nLen Length) override;
	nLen WriteBytes(ncData pData, nLen Length) override;
	void Lock() override;
	nResult TryLock() override;
	void Unlock() override;

	void WaitForConnection() const;
	std::future<void> WaitForConnectionAsync() const;

	nBool IsAsync() const
	{
		return m_bAsync;
	}

	nBool IsConnected() const
	{
		return m_bConnected;
	}

	nBool IsMessageComplete() const
	{
		return m_bMessageComplete;
	}

private:
	HANDLE m_hPipe;
	natCriticalSection m_Section;
	nBool m_bAsync, m_bConnected, m_bMessageComplete, m_bReadable, m_bWritable;
	NatErr m_LastErr;
};

class natNamedPipeClientStream
	: public natFileStream
{
public:
	natNamedPipeClientStream(ncTStr Pipename, nBool bReadable, nBool bWritable);
	~natNamedPipeClientStream() = default;

	nBool CanResize() const override
	{
		return false;
	}

	nBool CanSeek() const override
	{
		return false;
	}

	nLen GetSize() const override
	{
		return 0ul;
	}

	nResult SetSize(nLen /*Size*/) override
	{
		return m_LastErr = NatErr_NotSupport;
	}

	nLen GetPosition() const override
	{
		return 0ul;
	}

	nResult SetPosition(NatSeek /*Origin*/, nLong /*Offset*/) override
	{
		return m_LastErr = NatErr_NotSupport;
	}

	nResult Wait();
};