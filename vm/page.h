#include <hash.h>
#include <debug.h>
#include <stdint.h>
//Put Desired methods, put desired includes, maybe some struct definitions
//such

/* Show what type the page is */
enum page_type
{
	IN_FILE,
	IN_SWAP;
};

//Following struct definition slightly adapted from the pintos
//hash table example (Section A.8.5)
struct page
{
	void *vaddr;
	enum page_type type;
	//should probably include booleans to indicate statuses of pages,
	//dirty, reference, pinned etc.
	bool read_only;
	bool has_loaded;
	bool zero_page;
	struct frame *mapped_frame;
	struct hash_elem hash_elem;
};

/* Hash Functions for Frame Tables */
unsigned page_hash(const struct hash_elem *, void * UNUSED);
bool page_less(const struct hash_elem *, const struct hash_elem *,
           void * UNUSED);

/*Sup Table functions*/
void sup_table_init (struct hash *);
void sup_table_free (struct hash *);
struct page *page_lookup (void *);
struct page *insert_page (void *);
void page_free (struct page *);
