#ifndef GEROME_GEOM_H
#define GEROME_GEOM_H

#include <libgeom.h>
#include <sys/queue.h>
#include <sys/types.h>

struct grm_disks {
  LIST_HEAD(, grm_disk) grm_disk;
  int count;
};

struct grm_partitions {
  LIST_HEAD(, grm_partition) grm_partition;
  int count;
};
  
struct grm_disk {
  char* name;
  off_t size;
  off_t sector_size;
  struct grm_partitions partitions;
  
  LIST_ENTRY(grm_disk) grm_disk;
};

struct grm_partition {
  char* name;
  char* label;
  char* type;

  off_t size;
  off_t start;
  off_t end;
  
  LIST_ENTRY(grm_partition) grm_partition;
};

int grm_get_disks(struct grm_disks* disks);
void grm_disks_free(struct grm_disks* disks);

int grm_get_partitions_for_disk(struct grm_partitions* partitions, const char* disk);
void grm_partitions_free(struct grm_partitions* partitions);

#endif
