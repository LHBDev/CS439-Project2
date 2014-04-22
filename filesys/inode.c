#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

enum level
  {
    DIRECT,
    FIRST_INDIRECT,
    SECOND_INDIRECT
  };

#define DIRECT_SIZE 120
#define F_INDIR_SIZE (128 * 128 * BLOCK_SECTOR_SIZE)

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    bool is_dir;                        /* True if directory. */
    off_t length;                       /* File size in bytes. */
    off_t end;
    off_t f_end;
    off_t s_end;
    unsigned magic;                     /* Magic number. */
    block_sector_t *first_indir;
    block_sector_t *second_indir;
    block_sector_t inode_blocks[DIRECT_SIZE];
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    struct lock lock;
  };

//Added methods
static enum level find_indir (int);
static bool allocate_sectors (int, struct inode_disk *);
static bool allocate_direct (int, struct inode_disk *, int);
static bool allocate_first_indirect (int, struct inode_disk *, bool);
static bool allocate_second_indirect (int, struct inode_disk *);
static void deallocate_sectors (struct inode_disk *);

static enum level
find_indir (int num_sectors)
{
  enum level result;
  int num_alloc;
  num_alloc = DIV_ROUND_UP(num_sectors, BLOCK_SECTOR_SIZE);

  if(num_alloc < DIRECT_SIZE)
    result = DIRECT;
  else if(num_alloc < 128 * 128)
    result = FIRST_INDIRECT;
  else
    result = SECOND_INDIRECT;

  return result;
}

static bool
allocate_sectors (int num_sectors, struct inode_disk *data)
{
  enum level level_indir;

  if(num_sectors > 0)
    {
      level_indir = find_indir(num_sectors);

      if(level_indir == DIRECT)
        return allocate_direct(num_sectors, data, 0);
      else if (level_indir == FIRST_INDIRECT)
        return allocate_first_indirect(num_sectors, data, false);
      else
        return allocate_second_indirect(num_sectors, data);
    }
  else
    return true;
}

//allocate blocks directly for new inode
static bool
allocate_direct (int num_sectors, struct inode_disk *data, int indir)
{
  static char zeros[BLOCK_SECTOR_SIZE];

  if(!indir) {
    if(!free_map_allocate(num_sectors - data->end, &data->inode_blocks[data->end]))
      return false;
    while(data->end < num_sectors)
      block_write(fs_device, data->inode_blocks[(data->end)++], zeros);
  } else if(indir == 1) {
    if(!free_map_allocate(num_sectors - data->f_end, &data->first_indir[data->f_end]))
      return false;
    while(data->f_end < num_sectors)
      block_write(fs_device, data->first_indir[(data->f_end)++], zeros);
  } else {
    if(!free_map_allocate(num_sectors - data->s_end, &data->second_indir[data->s_end]))
      return false;
    while(data->s_end < num_sectors)
      block_write(fs_device, data->second_indir[(data->s_end)++], zeros);
  }
  return true;
}

static bool
allocate_first_indirect (int num_sectors, struct inode_disk *data, bool second_indir)
{
  if(!second_indir) {
    data->first_indir = calloc(128 * 128, BLOCK_SECTOR_SIZE);
    if(!allocate_direct(num_sectors, data, 1))
      return false;
  } else {
    if(!allocate_direct(num_sectors, data, 2))
      return false;
  }
  return true;
}

static bool
allocate_second_indirect (int num_sectors, struct inode_disk *data)
{
  int num_alloc = DIV_ROUND_UP(num_sectors, F_INDIR_SIZE);
  data->second_indir = calloc(num_alloc, F_INDIR_SIZE);

  while(num_alloc-- > 0)
    if(!allocate_first_indirect(F_INDIR_SIZE, data, true))
      return false;
  return true;
}

static void
deallocate_sectors (struct inode_disk *data)
{
  free_map_release(data->inode_blocks[0], data->end);
  free_map_release(data->first_indir[0], data->f_end);
  free_map_release(data->second_indir[0], data->s_end);
  free(data->first_indir);
  free(data->second_indir);
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  enum level level_indir = find_indir(pos);

  ASSERT (inode != NULL);
  if(pos < 0)
    return -1;

  if (pos < inode->data.length) {
    if(level_indir == DIRECT)
      return inode->data.inode_blocks[pos/BLOCK_SECTOR_SIZE];
    else if (level_indir == FIRST_INDIRECT) {
      pos -= DIRECT_SIZE;
      return inode->data.first_indir[pos];
    }
    else {
      pos -= F_INDIR_SIZE;
      return inode->data.second_indir[pos];
    }
  }
  else
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->is_dir = is_dir;
      disk_inode->end = 0;
      disk_inode->f_end = 0;
      disk_inode->s_end = 0;
      if(allocate_sectors(sectors, disk_inode)) {
        block_write (fs_device, sector, disk_inode);
        success = true;
      }
      free (disk_inode);
      // if (free_map_allocate (sectors, &disk_inode->start)) 
      //   {
      //     block_write (fs_device, sector, disk_inode);
      //     if (sectors > 0) 
      //       {
      //         static char zeros[BLOCK_SECTOR_SIZE];
      //         size_t i;
              
      //         for (i = 0; i < sectors; i++) 
      //           block_write (fs_device, disk_inode->start + i, zeros);
      //       }
      //     success = true; 
      //   } 
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init(&inode->lock);
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk. (Does it?  Check code.)
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          deallocate_sectors(&inode->data);
          // free_map_release (inode->data.start,
          //                   bytes_to_sectors (inode->data.length)); 
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  if(offset >= inode->data.length)
    return bytes_read;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  lock_acquire(&inode->lock);
  if((int) byte_to_sector(inode, offset+size) == -1)
    if(allocate_sectors(DIV_ROUND_UP(offset+size, BLOCK_SECTOR_SIZE), &inode->data))
      inode->data.length = offset + size;
  lock_release(&inode->lock);

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

bool
inode_is_dir (struct inode *inode)
{
  return inode->data.is_dir;
}
