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
#ifndef MSLIB_H
#define MSLIB_H

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

#define PEAK_THRESHOLD	750
#define PEAK_OFFSET	3

#define MAX_BITSTREAM_LEN 1024

#define ABA_SS		"11010"
#define MAX_ABA_LEN	41

#define IATA_SS		"1010001"
#define MAX_IATA_LEN	83 // some reports say max len is 79, but AAMVA says 82 ...

typedef enum {
	UNSET = 0,
	IATA,
	ABA,
	UNKNOWN = 9
} ms_dataType;


typedef struct {
	int idx;
	short amp;
} peakData;

typedef struct {
	/* PCM Data */
	int16_t *pcmData;
	int pcmDataLen;
	int pcmDataFree; // if ms_create_list used, this will be 1
	
	/* Peaks */
	GList *peakList;
	int peakThreshold;
	int peakOffset;

	/* Decode */
	ms_dataType dataType;
	char *bitStream;
	char *charStream;
} msData;


/* Init Functions */

/* internal helper function, do not use */
msData *_ms_create();

/* Create an msData object using the given 16bit array of PCM data
 * pcmData - A 16bit signed array of PCM data.
 * pcmDataLen - the number of elements in pcmData
 * Returns an msData object on success and NULL on failure */
msData *ms_create( const int16_t *pcmData, int pcmDataLen );

/* Create an msData object using a GList of 16bit arrays (Convenience function)
 * pcmDataList - a GList of 16bit arrays
 * blockSize - the size of each array in the list
 * Returns an msData object on success and NULL on failure */
msData *ms_create_list( GList *pcmDataList, int blockSize );

/* Garbage Collection */

/* Frees all data allocated for the given msData objected
 * Always returns NULL.
 * This function will not free any data that mslib did not allocate itself */
msData *ms_free( msData *ms );

/* internal helper function, do not use */
void ms_free_peakList( msData *ms );

/* Configuration functions */

/* Set the peakThreshold value.
 * The value determines the minimum amplitude to be considered a peak
 * You probably can stay with the default value */
void ms_set_peakThreshold( msData *ms, int peakThreshold );

/* Set the peakOffset value.
 * The value determines the offset to use when looking for intersects
 * You probably can stay with the default value */
void ms_set_peakOffset( msData *ms, int peakOffset );


/* Get the bit stream.
 * This would be the binary data that the peaks are decoded to.
 * Can only be run after ms_decode_peaks().  Format is a NULL-terminated
 * const char array.
 * Returns NULL on error (such as if ms_decode_peaks() has not been run) */
const char *ms_get_bitStream( msData *ms );

/* Get the char stream.
 * This is the decoded binary data that is human readable.
 * Can only be run after ms_decode_bits().  Format is a NULL-terminated
 * const char array.
 * Returns NULL on error (such as if ms_decode_bits() has not been run) */
const char *ms_get_charStream( msData *ms );




/* Find Peaks Functions */

/* internal helper function, do not use */
gboolean ms_range( int a, int b1, int b2 );

/* Finds the peaks in the stream.
 * This should be run immediately after ms_create().
 */
void ms_peaks_find( msData *ms );

/* Filters the list of peaks found using a grouping method based on signedness
 * of the amplitude.
 * This typically should be run after ms_peaks_find().
 */
void ms_peaks_filter_group( msData *ms );

/* internal helper function, do not use */
GList *_ms_peaks_filter_groupFind( msData *ms, GList *groupList );


/* Peak Decoding Functions */

/* internal helper function, do not use */
char _ms_closer( int *oneClock, int dif );

/* This decodes the peaks found through the ms_peaks*() functions.
 * It should be run after them.
 */
void ms_decode_peaks( msData *ms );

/* Bit Decoding functions */

/* This determines the type of encoding used: ABA or IATA.
 * At present, only ABA is supported (as found on track 2 of most cards).
 *
 * This is generally used as a helper function internally, but can be run
 * directly.  It will return 1 on error and 0 on success.
 * Success means it was able to detect either ABA or IATA. */
int ms_decode_typeDetect( msData *ms );

/* This decodes the bits into the charstream.
 * It should be run after ms_decode_peaks().
 * The encoding is auto-detected using ms_decode_typeDetect()
 * Returns -1 on error, 0 on success, and from the enum ms_dataType UNKNOWN (9)
 * if the card format was not detected (de facto error) */
int ms_decode_bits( msData *ms );

/* internal helper function, do not use */
char _ms_decode_bits_char( char *bitStream, char *LRC, ms_dataType type );

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif
