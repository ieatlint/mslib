#include "llist.h"


LListH *llist_init() {
	LListH *list;

	list = ( LListH * ) malloc( ( sizeof( LListH ) * 1 ) );
	if( list == NULL )
		return NULL;
	
	list->first = NULL;
	list->last = NULL;
	list->len = 0;

	return list;
}


LListH *llist_append( LListH *list, int idx, short amp ) {
	LList *node;

	if( list == NULL )
		list = llist_init();

	node = ( LList * ) malloc( ( sizeof( LList ) * 1 ) );
	if( node == NULL )
		return NULL;
	
	node->idx = idx;
	node->amp = amp;
	node->next = NULL;

	if( list->first == NULL )
		list->first = node;
	if( list->last != NULL )
		list->last->next = node;
	
	list->last = node;

	list->len++;

	return list;
}

void llist_remove_idx( LListH *list, int idx ) {
	LList *trav,*prev;

	for( trav = list->first, prev = NULL; trav != NULL; trav = trav->next ) {
		if( trav->idx == idx )
			break;
		prev = trav;
	}

	if( trav != NULL ) {//if found
		if( prev != NULL ) {
			prev->next = trav->next;
		} else {
			list->first = trav->next;
		}
		if( trav == list->last )
			list->last = prev;

		free( trav );
		list->len--;
	}
}

LListH *llist_free( LListH *list ) {
	LList *trav,*prev;

	if( list == NULL )
		return NULL;

	for( trav = list->first, prev = NULL; trav != NULL; ) {
		prev = trav;
		trav = trav->next;
		if( prev ) {
			free( prev );
			prev = NULL;
		}
	}

	if( prev )
		free( prev );
	
	free( list );

	return NULL;
}

int llist_length( LListH *list ) {
	return list->len;
}
