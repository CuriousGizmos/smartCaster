#ifndef __BYTE__STREAM__H__
#define __BYTE__STREAM__H__


//////////////////////////////////////////////////////////////////////////
//
// cgByteStream
//
//////////////////////////////////////////////////////////////////////////

class cgByteStream {
public:
	explicit cgByteStream( char *buffer, unsigned int size ) {
		memset( &state, 0, sizeof( state ) );

		data.m_buffer = buffer;
		data.m_size = size;
	}

	void BeginRead();
	void BeginWrite();

	void WriteBytes( char *data, unsigned int numBytes );
	void ReadBytes( char *data, unsigned int numBytes );

	template< typename Type >
	void WriteValue( Type &value );

	template< typename Type >
	void ReadValue( Type &value );

	char *GetBuffer() { return data.m_buffer; }
	unsigned int GetSize() { return data.m_size; }

	unsigned int GetNumBytesWritten() { return state.m_bytesWritten; }
	unsigned int GetNumBytesRead() { return state.m_bytesRead; }

private:
	struct s_data {
		char			*m_buffer;
		unsigned int	m_size;
	} data;

	struct s_state {
		char			*m_readCursor;
		char			*m_writeCursor;

		unsigned int	m_bytesWritten;
		unsigned int	m_bytesRead;
	} state;
};


//////////////////////////////////////////////////////////////////////////
// cgByteStream::BeginRead
//////////////////////////////////////////////////////////////////////////

void cgByteStream::BeginRead() {
	state.m_readCursor = data.m_buffer;
	state.m_bytesRead = 0;
}

//////////////////////////////////////////////////////////////////////////
// cgByteStream::BeginWrite
//////////////////////////////////////////////////////////////////////////

void cgByteStream::BeginWrite() {
	state.m_writeCursor = data.m_buffer;
	state.m_bytesWritten = 0;
}

//////////////////////////////////////////////////////////////////////////
// cgByteStream::WriteBytes
//////////////////////////////////////////////////////////////////////////

void cgByteStream::WriteBytes( char *data, unsigned int numBytes ) {
	memcpy( state.m_writeCursor, data, numBytes );

	state.m_writeCursor += numBytes;
	state.m_bytesWritten += numBytes;
}

//////////////////////////////////////////////////////////////////////////
// cgByteStream::ReadBytes
//////////////////////////////////////////////////////////////////////////

void cgByteStream::ReadBytes( char *data, unsigned int numBytes ) {
	memcpy( data, state.m_readCursor, numBytes );

	state.m_readCursor += numBytes;
	state.m_bytesRead += numBytes;
}

//////////////////////////////////////////////////////////////////////////
// cgByteStream::WriteValue
//////////////////////////////////////////////////////////////////////////

template< typename Type >
void cgByteStream::WriteValue( Type &value ) {
	WriteBytes( (char *)&value, sizeof( value ) );
}

//////////////////////////////////////////////////////////////////////////
// cgByteStream::ReadValue
//////////////////////////////////////////////////////////////////////////

template< typename Type >
void cgByteStream::ReadValue( Type &value ) {
	ReadBytes( (char *)&value, sizeof( value ) );
}


#endif	// __BYTE__STREAM__H__