#ifndef __CG__SAVE_LOAD__H
#define __CG__SAVE_LOAD__H

#include "Common.h"


//////////////////////////////////////////////////////////////////////////
// writeSettingsToStream
//////////////////////////////////////////////////////////////////////////

void writeSettingsToStream( cgByteStream &stream ) {
	struct s_header header = {
		SC_VERSION,
		SETTINGS_SIZE
	};

	stream.BeginWrite();
	stream.WriteValue( header );
	stream.WriteValue( settings );

	for ( int i = 0; i < NUM_PAGES; ++i ) {
		stream.WriteValue( pages[ i ] );
	}
}

//////////////////////////////////////////////////////////////////////////
// readSettingsFromStream
//////////////////////////////////////////////////////////////////////////

void readSettingsFromStream( cgByteStream &stream ) {
	stream.BeginRead();

	s_header newHeader;
	stream.ReadValue( newHeader );

	// if loading header and version mismatch, factory reset
	if ( newHeader.version != SC_VERSION ) {
		factoryReset();

		return;
	}

	stream.ReadValue( settings );

	for ( int i = 0; i < NUM_PAGES; ++i ) {
		stream.ReadValue( pages[ i ] );
	}
}

//////////////////////////////////////////////////////////////////////////
// saveSettings
//////////////////////////////////////////////////////////////////////////

void saveSettings() {
	// NOTE: EEPROM can only be written to so many times so be careful when and where you call this
	// NOTE: make sure to stay within EEPROM memory limits for the processor

	char buffer[ MAX_BUFFER_SIZE ];

	cgByteStream stream( buffer, MAX_BUFFER_SIZE );
	writeSettingsToStream( stream );

	// TODO: verify SETTINGS_SIZE and byteswritten match.

	for ( int i = 0; i < stream.GetNumBytesWritten(); ++i ) {
		EEPROM.write( i, buffer[ i ] );
	}

	if ( verbose ) {
		Serial.println( "Bytes Saved: " );
		Serial.println( stream.GetNumBytesWritten() );
	}
}

//////////////////////////////////////////////////////////////////////////
// loadSettings
//////////////////////////////////////////////////////////////////////////

void loadSettings() {
	char buffer[ MAX_BUFFER_SIZE ];

	for ( int i = 0; i < SETTINGS_SIZE; ++i ) {
		buffer[ i ] = EEPROM.read( i );
	}

	cgByteStream stream( buffer, SETTINGS_SIZE );
	readSettingsFromStream( stream );
	
	if ( verbose ) {
		Serial.println( "Bytes Loaded: " );
		Serial.println( SETTINGS_SIZE );
	}		
}

//////////////////////////////////////////////////////////////////////////
// exportSettingsToSerial
//////////////////////////////////////////////////////////////////////////

void exportSettingsToSerial() {
	char buffer[ MAX_BUFFER_SIZE ];

	cgByteStream stream( buffer, MAX_BUFFER_SIZE );
	writeSettingsToStream( stream );

	/*for ( uint32 i = 0; i < stream.GetNumBytesWritten(); ++i ) {
	Serial.write( buffer[ i ] );
	}*/
	Serial.write( (const uint8_t *)buffer, stream.GetNumBytesWritten() );

	Serial.flush();
	
	if ( verbose ) {
		Serial.println( "Bytes Exported: " );
		Serial.println( stream.GetNumBytesWritten() );
	}	
}

//////////////////////////////////////////////////////////////////////////
// importSettingsFromSerial
//////////////////////////////////////////////////////////////////////////

bool importSettingsFromSerial() {
	// make sure there isn't any errant data sitting around
	Serial.flush();

	// wait until we have data
	while ( Serial.available() < 1 ) { delay( 500 ); }

	if ( Serial.available() >= MAX_BUFFER_SIZE ) {
		Serial.println( "ERROR: importSettingsFromSerial: too much serial data for buffer to hold" );

		return false;
	}

	if ( Serial.available() != SETTINGS_SIZE ) {
		Serial.println( "ERROR: importSettingsFromSerial: incoming darta size does not match expected size" );

		return false;
	}

	char buffer[ MAX_BUFFER_SIZE ];
	int size = 0;

	while ( Serial.available() > 0 ) {
		buffer[ size++ ] = Serial.read();
	}

	cgByteStream stream( buffer, size );
	readSettingsFromStream( stream );

	if ( verbose ) {
		Serial.println( "Bytes Exported: " );
		Serial.println( size );
	}	
	
	return true;
}


#endif // __CG__SAVE_LOAD__H
