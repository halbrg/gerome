#include "geom.h"

#include <libgeom.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>

#include <ncurses.h>

int grm_get_disks(struct grm_disks *disks)
{
  if (!disks) {
    return -1;
  }
  
  struct gmesh mesh;
  int err = geom_gettree(&mesh);
  if (err != 0) {
    return -1;
  }

  LIST_INIT(&disks->grm_disk);
  disks->count = 0;

  struct gclass* classp;
  LIST_FOREACH(classp, &mesh.lg_class, lg_class) {
    if (strcmp(classp->lg_name, "DISK") == 0) {
      struct ggeom* gg;
      LIST_FOREACH(gg, &classp->lg_geom, lg_geom) {
        struct gprovider* gp;
        struct grm_disk* prev = NULL;
        LIST_FOREACH(gp, &gg->lg_provider, lg_provider) {          
          struct grm_disk* disk = malloc(sizeof(struct grm_disk));
        
          disk->name = malloc(strlen(gp->lg_name) + 1);
          strcpy(disk->name, gp->lg_name);

          disk->size = gp->lg_mediasize;
          disk->sector_size = gp->lg_sectorsize;

          grm_get_partitions_for_disk(&disk->partitions, disk->name);
        
          if (!prev) LIST_INSERT_HEAD(&disks->grm_disk, disk, grm_disk);
          else LIST_INSERT_AFTER(prev, disk, grm_disk);
          disks->count++;

          prev = disk;
        }
      }
      break;
    }
  }

  geom_deletetree(&mesh);

  return 0;
}

void grm_disks_free(struct grm_disks *disks)
{
  if (!disks) {
    return;
  }

  struct grm_disk* disk = LIST_FIRST(&disks->grm_disk);
  struct grm_disk* next;
  while (disk) {
    next = LIST_NEXT(disk, grm_disk);
    free(disk->name);
    grm_partitions_free(&disk->partitions);
    free(disk);
    disk = next;   
  }
  LIST_INIT(&disks->grm_disk);
}

int grm_get_partitions_for_disk(struct grm_partitions* partitions, const char* disk)
{
  if (!partitions) {
    return -1;
  }
  
  struct gmesh mesh;
  int err = geom_gettree(&mesh);
  if (err != 0) {
    return -1;
  }

  LIST_INIT(&partitions->grm_partition);
  partitions->count = 0;

  struct gclass* classp;
  LIST_FOREACH(classp, &mesh.lg_class, lg_class) {
    if (strcmp(classp->lg_name, "PART") == 0) {
      struct ggeom* gg;
      LIST_FOREACH(gg, &classp->lg_geom, lg_geom) {
        if (strcmp(gg->lg_name, disk) == 0) {
          struct gprovider* gp;
          struct grm_partition* prev = NULL;
          LIST_FOREACH(gp, &gg->lg_provider, lg_provider) {            
            struct grm_partition* partition = malloc(sizeof(struct grm_partition));
            
            partition->name = malloc(strlen(gp->lg_name) + 1);
            strcpy(partition->name, gp->lg_name);

            partition->size = gp->lg_mediasize;

            struct gconfig* gc;
            LIST_FOREACH(gc, &gp->lg_config, lg_config) {
              if (strcmp(gc->lg_name, "label") == 0) {
                partition->label = malloc(strlen(gc->lg_val) + 1);
                strcpy(partition->label, gc->lg_val);        
              } else if (strcmp(gc->lg_name, "type") == 0) {
                partition->type = malloc(strlen(gc->lg_val) + 1);
                strcpy(partition->type, gc->lg_val);        
              } else if (strcmp(gc->lg_name, "start") == 0) {
                partition->start = (off_t) strtoimax(gc->lg_val, NULL, 0);
              } else if (strcmp(gc->lg_name, "end") == 0) {
                partition->end = (off_t) strtoimax(gc->lg_val, NULL, 0);
              }
            }
        
            if (!prev) LIST_INSERT_HEAD(&partitions->grm_partition, partition, grm_partition);
            else LIST_INSERT_AFTER(prev, partition, grm_partition);
            partitions->count++;

            prev = partition;
          }
        }
      }
      break;
    }
  }

  geom_deletetree(&mesh);

  return 0;
}

void grm_partitions_free(struct grm_partitions* partitions)
{
  if (!partitions) {
    return;
  }
  
  struct grm_partition* partition = LIST_FIRST(&partitions->grm_partition);
  struct grm_partition* next;
  while (partition) {
    next = LIST_NEXT(partition, grm_partition);
    free(partition->name);
    free(partition->label);
    free(partition->type);
    free(partition);
    partition = next;   
  }
  LIST_INIT(&partitions->grm_partition);
}
