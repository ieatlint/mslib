/*
 * This file is part of mslib.
 * mslib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mslib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nosebus.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "mslib.h"

/* Data Create/Free functions */
msData *_ms_create() {
	msData *ms;

	ms = g_new0( msData, 1 );
	if ( !ms )
		return NULL;
	ms->peakThreshold = PEAK_THRESHOLD;
	ms->peakOffset = PEAK_OFFSET;
	ms->pcmDataFree = 0;

	return ms;
}

msData *ms_create( const int16_t *pcmData, int pcmDataLen ) {
	msData *ms;

	ms = _ms_create();
	if( !ms )
		return NULL;
	
	ms->pcmData = ( int16_t * )pcmData;
	ms->pcmDataLen = pcmDataLen;

	return ms;
}

msData *ms_free( msData *ms ) {
	if( !ms )
		return NULL;
	
	if( ms->pcmDataFree && ms->pcmData )
		g_free( ms->pcmData );
	
	if( ms->peakList )
		ms_free_peakList( ms );
	
	if( ms->bitStream )
		g_free( ms->bitStream );
	
	if( ms->charStream )
		g_free( ms->charStream );

	g_free( ms );

	return NULL;
}

/* Misc User Functions */
void ms_set_peakThreshold( msData *ms, int peakThreshold ) {
	if( !ms )
		return;
	ms->peakThreshold = peakThreshold;
}

void ms_set_peakOffset( msData *ms, int peakOffset ) {
	if( !ms )
		return;
	ms->peakOffset = peakOffset;
}

const char *ms_get_bitStream( msData *ms ) {
	if( !ms )
		return NULL;
	return ms->bitStream;
}

const char *ms_get_charStream( msData *ms ) {
	if( !ms )
		return NULL;
	return ms->charStream;
}


/* Finding Peaks */
gboolean ms_range( int a, int b1, int b2 ) {
	if( a > b1 && a < b2 )
		return TRUE;
	if( a > b2 && a < b1 )
		return TRUE;
	return FALSE;
}

void ms_peaks_find( msData *ms ) {
	int i;
	
	if( ms == NULL || ( ms->pcmData == NULL ) )
		return;
	
	ms->peakList = NULL;

	for( i = 0; ( i + ms->peakOffset + 2 ) < ms->pcmDataLen; i++ ) {
		if( abs( ms->pcmData[ i ] ) > ms->peakThreshold && ms_range( ms->pcmData[ i ], ms->pcmData[ ( i + ms->peakOffset ) - 2 ], ms->pcmData[ ( i + ms->peakOffset ) + 2 ] ) ) {
			ms->peakList = llist_append( ms->peakList, i, ms->pcmData[ i ] );
		}
	}
}

void ms_peaks_filter_group( msData *ms ) {
	LList *trav;
	LListH *groupList;
	
	int pos;//indicates pos/neg (not position)

	if( !ms || ms->peakList->len < 2 )
		return;
	
	pos = ( ms->peakList->first->amp > 0 );

	groupList = NULL;
	for( trav = ms->peakList->first; trav != NULL; trav = trav->next ) {

		if( ( trav->amp > 0 ) != pos ) {
			pos = !pos;
			groupList = _ms_peaks_filter_groupFind( ms, groupList );
		}
		
		groupList = llist_append( groupList, trav->idx, trav->amp );
	}

	if( groupList )
		groupList = _ms_peaks_filter_groupFind( ms, groupList );
}



LListH *_ms_peaks_filter_groupFind( msData *ms, LListH *groupList ) {
	LList *trav;
	struct {
		int idx;
		short amp;
	} bigPeak;

	if( !ms || groupList == NULL || groupList->len < 2 )
		return NULL;
	
	bigPeak.idx = groupList->first->idx;
	bigPeak.amp = abs( groupList->first->amp );

	for( trav = groupList->first->next; trav != NULL; trav = trav->next ) {
		if( abs( trav->amp ) > bigPeak.amp ) {
			llist_remove_idx( ms->peakList, bigPeak.idx );
			bigPeak.idx = trav->idx;
			bigPeak.amp = abs( trav->amp );
		} else {
			llist_remove_idx( ms->peakList, trav->idx );
		}
	}

	llist_free( groupList );
	return NULL;
}


/* Peak Decode functions */
char _ms_closer( int *oneClock, int dif ) {
	int oneDif = abs( *oneClock - dif );
	int zeroDif = abs( ( *oneClock * 2 ) - dif );
	char bit;

	if( oneDif < zeroDif ) {
		*oneClock = dif;
		bit = '1';
	} else {
		*oneClock = dif / 2;
		bit = '0';
	}

	return bit;
}

void ms_decode_peaks( msData *ms ) {
	LList *trav;
	int clock, len;
	int lastPeakidx;
	char lastBit, curBit, bitStream[ MAX_BITSTREAM_LEN + 1 ];

	if( !ms || ms->peakList == NULL || ms->peakList->len < 3 )
		return;
	
	lastPeakidx = ms->peakList->first->next->idx;
	clock = ( ms->peakList->first->next->next->idx - lastPeakidx ) / 2;

	len = 0;
	lastBit = '\0';
	for( trav = ms->peakList->first->next; trav != NULL && len < MAX_BITSTREAM_LEN; trav = trav->next ) {
		curBit = _ms_closer( &clock, ( trav->idx - lastPeakidx ) );
		if( curBit == '0' ) {
			bitStream[ len++ ] = curBit;
		} else if( curBit == lastBit ) {
			bitStream[ len++ ] = curBit;
			curBit = '\0';
		}
		
		lastBit = curBit;
		lastPeakidx = trav->idx;
	}

	bitStream[ len ] = '\0';

	ms->bitStream = g_strdup( bitStream );
}



/* Bit Decode functions */
int ms_decode_typeDetect( msData *ms ) {
	char *bitStream;
	int loop = 2;

	if( !ms || !ms->bitStream )
		return 1;


	do {
		bitStream = strchr( ms->bitStream, '1' );
		if( bitStream == NULL )
			break;
	
		if( !strncmp( bitStream, ABA_SS, ABA_CHAR_LEN ) ) {
			ms->dataType = ABA;
			return 0;
		} else if( !strncmp( bitStream, IATA_SS, IATA_CHAR_LEN ) ) {
			ms->dataType = IATA;
			return 0;
		}

		g_strreverse( ms->bitStream );
		loop--;
	} while( loop );


	ms->dataType = UNKNOWN;
	return 1;
}


int ms_decode_bits( msData *ms ) {
	char *bitStream;
	char charStream[ MAX_IATA_LEN + 1 ], curChar;
	char LRC[ IATA_CHAR_LEN ] = { 0 };
	int bitStreamLen, i, x, len, validSwipe;
	int maxLen, charLen, badChars;

	if( !ms || !ms->bitStream )
		return -1;

	if( ms_decode_typeDetect( ms ) )
		return -1;

	if( ms->dataType == ABA ) {
                maxLen = MAX_ABA_LEN;
                charLen = ABA_CHAR_LEN;
	} else {
		maxLen = MAX_IATA_LEN;
                charLen = IATA_CHAR_LEN;
	}

	validSwipe = 0;

	bitStream = strchr( ms->bitStream, '1' );
	if( bitStream == NULL ) // if stream contains no 1s, it's bad, just quit
		return 1;
	
	bitStreamLen = strlen( bitStream );

	/* Traverse the bitstream to decode all the bits into a charstream */
	curChar = '\0';
	badChars = 0;
	for( i = 0, len = 0; ( i + charLen ) < bitStreamLen && len < maxLen && curChar != '?'; i += charLen, len++ ) {
		curChar = _ms_decode_bits_char( bitStream + i, LRC, ms->dataType );
		charStream[ len ] = curChar;
		if( curChar == BAD_CHAR )
			badChars++; // count the bad chars
	}
	charStream[ len ] = '\0';

	/* Print warning about any detected bad characters */
	if( badChars ) {
		fprintf( stderr, "ms_decode_bits(): Warning: %d chars failed parity check\n", badChars );
		validSwipe = 1;
	}

	ms->charStream = g_strdup( charStream );
	if( !ms->charStream )
		return 1;
	

	/* Calculate the parity bit for the LRC */
	LRC[ ( charLen - 1 ) ] = 1;
	for( x = 0; x < ( charLen - 1 ); x++ ) {
		if( LRC[ x ] == 1 )
			LRC[ ( charLen - 1 ) ] = !LRC[ ( charLen - 1 ) ];
		LRC[ x ] += NUM_ASCII_OFFSET;
	}
	LRC[ ( charLen - 1 ) ] += NUM_ASCII_OFFSET;
	
	/* Verify the LRC */
	if( strncmp( LRC, bitStream + i, charLen ) ) {
		fprintf( stderr, "ms_decode_bits(): Warning: LRC error decoding stream\n" );
		validSwipe = 1;
	}

	return validSwipe;
}

char _ms_decode_bits_char( char *bitStream, char *LRC, ms_dataType type ) {
	int parity = 0, i;
	char out;
	int len; // char length not including parity
	int offset; // offset to make it ASCII

	if( type == ABA ) {
		len = ABA_CHAR_LEN - 1;
		offset = ABA_ASCII_OFFSET;
	} else {
		len = IATA_CHAR_LEN - 1;
		offset = IATA_ASCII_OFFSET;
	}

	for( i = 0, out = 0; i < len; i++ ) {
		out |= ( bitStream[ i ] - NUM_ASCII_OFFSET ) << i; // using OR to assign the bits into the char
		if( bitStream[ i ] == '1' ) {
			LRC[ i ] = !LRC[ i ]; // flip the bit in the LRC for all 1 bits in the char
			parity++; // count the number of 1 bits for the parity bit
		}
	}
	out += offset;

	if( ( parity & 1 ) == ( bitStream[ len ] - NUM_ASCII_OFFSET ) )
		out = BAD_CHAR; // return the error char if the calculated parity bit doesn't match the recorded one
	
	return out;
}

