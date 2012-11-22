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
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "mslib.h"

#define BUF_SIZE 1024


/* This function is used to save the peaks detected for debug purposes */
void peak_save( msData *ms ) {
	LList *trav;
	FILE *peaks;

	peaks = fopen( "ms.peaks", "w" );

	for( trav = ms->peakList->first; trav != NULL; trav = trav->next ) {
		fprintf( peaks, "%d %d\n", trav->idx, trav->amp );
	}

	fclose( peaks );
}

void audio_process( short *blocks, int len ) {
	msData *ms;
	const char *tmpStr;

	ms = ms_create( blocks, len );

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
	short *data;
	short *blocks;
	int fd;
	struct stat fdStat;
	int offset = 0;

	if( argc == 1 ) {
		printf( "%s <audio file>\n", argv[0] );
		return 1;
	}

	fd = open( argv[1], O_RDONLY );
	if( fd == -1 ) {
		perror( "Failed to open audio file" );
		return -1;
	}

	if( fstat( fd, &fdStat ) ) {
		perror( "Failed to stat file" );
		return -1;
	}

	data = mmap( 0, fdStat.st_size, PROT_READ, MAP_SHARED, fd, 0 );

	if( data == MAP_FAILED ) {
		close( fd );
		perror( "Failed to map file" );
		return -1;
	}

	if( !strncmp( ( argv[1] + strlen( argv[1] - 4 ) ), ".wav", 4 ) ) {
		// use a 44byte offset for .wav files to bypass header
		offset = 44;
	}
	blocks = (short *)(data + offset);

	audio_process( blocks + offset, ( fdStat.st_size - offset ) / sizeof( short ) );

	if( munmap( data, fdStat.st_size ) == -1 ) {
		perror( "Failed to unmap file" );
	}
	close( fd );

	return 0;
}
