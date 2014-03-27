#include "frame.h"
#include "swap.h"
#include "page.h"
#include "lib/kernel/hash.h"

//Header prototypes
struct frame
{
	struct hash_elem hash_elem;
	uint32_t *addr;	
};

struct hash frame_table;

/* Returns a hash value for page p. */
unsigned
frame_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct frame *p = hash_entry (p_, struct frame, hash_elem);
  return hash_bytes (&p->addr, sizeof p->addr);
}

/* Returns true if page a precedes page b. */
bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
  const struct frame *a = hash_entry (a_, struct frame, hash_elem);
  const struct frame *b = hash_entry (b_, struct frame, hash_elem);

  return a->addr < b->addr;
}

void
frame_init (void)
{
	hash_init(&frame_table, frame_hash, frame_less, NULL);
}

