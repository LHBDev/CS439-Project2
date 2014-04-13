#include <hash.h>
#include <debug.h>
#include <stdint.h>
#include "filesys/file.h"
#include "threads/thread.h"

/* Show what type the page is */
enum page_type
{
	IN_FILE,
	IN_SWAP
};

//A sup. table entry. Item descriptions in Siva's design doc
struct page
{
	void *vaddr;
	enum page_type type;
	struct file *load_file;
	off_t file_ofs;
	uint32_t read_bytes;
	int swp;
	bool read_only;
	bool has_loaded;
	bool zero_page;
	struct hash_elem hash_elem;
};

/* Hash Functions for Frame Tables */
unsigned page_hash(const struct hash_elem *, void * UNUSED);
bool page_less(const struct hash_elem *, const struct hash_elem *,
           void * UNUSED);
void page_action_func (struct hash_elem *, void * UNUSED);

/*Sup Table functions*/
void sup_table_init (struct hash *);
void sup_table_free (void);
struct page * lookup_page (void *, struct thread *);
struct page * insert_page (void *);
void free_page (struct page *);
