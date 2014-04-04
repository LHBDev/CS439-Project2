#include <hash.h>
#include <debug.h>
#include <stdint.h>
//Put Desired methods, put desired includes, maybe some struct definitions
//such

struct spt_entry
{
	uint8_t *vpage;
	struct frame *mapped_frame;
	struct hash_elem hash_elem;
};

/* Hash Functions for Frame Tables */
unsigned spt_hash(const struct hash_elem *, void * UNUSED);
bool spt_less(const struct hash_elem *, const struct hash_elem *,
           void * UNUSED);

/*Sup Table functions*/
void sup_table_init (void);
