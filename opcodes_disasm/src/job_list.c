/* job_list.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "job_list.h"

/* allocate a job list */
job_list_t job_list_alloc( void ) {
	return (job_list_t) calloc( 1, sizeof(struct JOB_LIST_HEAD) );
}

/* free an allocated job list */
void job_list_free( job_list_t jobs ) {
	job_list_item_t * item, * next;

	if (! jobs ) {
		return;
	}

	for ( item = jobs->head; item; item = next ) {
		next = item->next;
		free( item );
	}

	free( jobs );
}

/* add a memspec job to a job list */
static job_list_item_t * add_item( job_list_t jobs, enum job_type_t type, 
			   const char * spec, unsigned int target ) {
	job_list_item_t * item, *prev;

	if (! jobs || ! spec ) {
		return 0;
	}

	item = (job_list_item_t *) calloc( 1, sizeof(job_list_item_t) );
	if (! item ) {
		return 0;
	}

	item->type = type;
	item->spec = spec;
	item->target = target;

	if (! jobs->head ) {
		jobs->head = item;
	} else {
		for ( prev = jobs->head; prev->next; prev = prev->next )
			;
		prev->next = item;
	}

	jobs->num_items++;

	return item;
}

unsigned int job_list_add( job_list_t jobs, enum job_type_t type, 
			   const char * spec, unsigned int target, 
			   opdis_off_t offset, opdis_vma_t vma, 
			   opdis_off_t size ) {
	job_list_item_t * item = add_item( jobs, type, spec, target );
	if (! item ) {
		return 0;
	}

	item->offset = offset;
	item->vma = vma;
	item->size = size;

	return jobs->num_items;
}


/* add a bfd job to a job list */
unsigned int job_list_add_bfd( job_list_t jobs, enum job_type_t type, 
			       const char * spec, unsigned int target, 
			       const char * bfd_name ) {
	job_list_item_t * item = add_item( jobs, type, spec, target );
	if (! item ) {
		return 0;
	}

	item->bfd_name = bfd_name;

	return jobs->num_items;
}

/* invoke a callback for every job in list */
void job_list_foreach( job_list_t jobs, JOB_LIST_FOREACH_FN fn, void * arg ) {
	job_list_item_t * item;
	unsigned int id = 1;

	if (! jobs || ! fn ) {
		return;
	}

	for ( item = jobs->head; item; item = item->next, id++ ) {
		fn( item, id, arg );
	}
}

static int perform_job( job_list_item_t * job, tgt_list_t targets,
			mem_map_t map, opdis_t o ) {
	if (! job || ! targets || ! map || ! o ) {
		return 0;
	}

	switch (job->type) {
		case job_cflow:
			// control for job args
			break;
		case job_linear:
			// cflow for job args
			break;
		case job_bfd_symbol:
			// get symbol vma, buf for vma, do cflow
			break;
		case job_bfd_section:
			// get section vma and size, buf for vma, do linear
			break;
		default:
			break;
	}
	return 0;
}

/* perform the specified job */
int job_list_perform( job_list_t jobs, unsigned int id, tgt_list_t targets,
		      mem_map_t map, opdis_t o ) {
	job_list_item_t * item;
	unsigned int curr_id = 1;

	if (! jobs ) {
		return;
	}

	for ( item = jobs->head; item; item = item->next, curr_id++ ) {
		if ( curr_id == id ) {
			return perform_job( item, targets, map, o );
		}
	}

	return 0;
}

/* perform all jobs */
int job_list_perform_all( job_list_t jobs , tgt_list_t targets, mem_map_t map, 
			  opdis_t o, int quiet ) {
	job_list_item_t * item;
	int rv = 1;

	if (! jobs ) {
		return;
	}

	for ( item = jobs->head; item; item = item->next ) {
		int result = perform_job( item, targets, map, o );
		if (! result ) {
		}
		rv &= result;
	}

	return rv;
}

static void print_details( FILE * f, job_list_item_t * item ) {
	if ( item->size ) {
		fprintf( f, "%d bytes at ", item->size );
	}

	if ( item->offset != OPDIS_INVALID_ADDR ) {
		fprintf( f, "offset %p ", (void *) item->offset );
	}

	if ( item->vma != OPDIS_INVALID_ADDR ) {
		fprintf( f, "VMA %p", (void *) item->vma );
	}

	if ( item->vma == OPDIS_INVALID_ADDR && 
	     item->offset == OPDIS_INVALID_ADDR ) {
		fprintf( f, "Invalid Address" );
	}
}

static void print_job( job_list_item_t * item, unsigned int id, void * arg ) {
	FILE * f = (FILE *) arg;
	if (! f ) {
		return;
	}

	fprintf( f, "%d\tTarget %d:", id, item->target );

	switch ( item->type ) {
		case job_cflow:
			fprintf( f, "Control Flow disassembly of " );
			print_details( f, item );
			fprintf( f, "\n" );
			break;

		case job_linear:
			fprintf( f, "Linear disassembly of " );
			print_details( f, item );
			fprintf( f, "\n" );
			break;

		case job_bfd_section:
			fprintf( f, "Linear disassembly of BFD section '%s'\n", 
				item->bfd_name );
			break;

		case job_bfd_symbol:
			fprintf( f, 
				"Control Flow disassembly of BFD symbol '%s'\n",
				item->bfd_name );
			break;
		default:
			fprintf( f, "Unknown job type for '%s'\n", item->spec );
	}
	
}

void job_list_print( job_list_t jobs, FILE * f ) {
	job_list_foreach( jobs, print_job, f );
}
