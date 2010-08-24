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
#include <stdio.h>
#include <glib.h>
#include <sys/types.h>

#include "mslib.h"

#define BUF_SIZE 1024


/* This function is used to save the peaks detected for debug purposes */
void peak_save( msData *ms ) {
	GList *trav;
	FILE *peaks;
	peakData *curPeak;

	peaks = fopen( "ms.peaks", "w" );

	for( trav = ms->peakList; trav != NULL; trav = trav->next ) {
		curPeak = trav->data;
		fprintf( peaks, "%d %d\n", curPeak->idx, curPeak->amp );
	}

	fclose( peaks );
}

void audio_process( GList *blockList ) {
	msData *ms;
	const char *tmpStr;

	ms = ms_create_list( blockList, BUF_SIZE );

	ms_peaks_find( ms );
	ms_peaks_filter_group( ms );
	peak_save( ms );

	ms_decode_peaks( ms );
	tmpStr = ms_get_bitStream( ms );
	printf("Decoded to bits:\n%s\n", tmpStr );

	if( ms_decode_bits( ms ) ) {
		fprintf( stderr, "Warning: Appears to be unsuccessful swipe!\n" );
		// but will display the data we got anyway...
	}
	tmpStr = ms_get_charStream( ms );
	printf( "Decoded to chars:\n%s\n", tmpStr );

	ms = ms_free( ms );
}

int main( int argc, char **argv ) {
	GList *blocks = NULL;
	int16_t *block;
	FILE *input;

	if( argc == 1 ) {
		printf( "%s <audio file>\n", argv[0] );
		return 1;
	}


	input = fopen( argv[1], "r" );

	if( g_str_has_suffix( argv[1], "wav" ) ) {
		/* WAV file input, skip the 44 byte WAV header */
		fseek( input, 44, 0 );
	}

	/* load the data into a list
	 * Note that normally an mmap would be more effective, but this method
	 * was used to debug the ms_create_list() function. */
	while( !feof( input ) ) {
		block = g_new0( int16_t, BUF_SIZE );
		fread( block, sizeof( int16_t ), BUF_SIZE, input );
		blocks = g_list_append( blocks, block );
	}

	/* process the audio */
	audio_process( blocks );

	fclose( input );

	return 0;
}
