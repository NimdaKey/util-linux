#ifndef BLKID_TOPOLOGY_H
#define BLKID_TOPOLOGY_H

#include "blkidP.h"

extern int blkid_topology_set_alignment_offset(blkid_probe pr, unsigned long val);
extern int blkid_topology_set_minimum_io_size(blkid_probe pr, unsigned long val);
extern int blkid_topology_set_optimal_io_size(blkid_probe pr, unsigned long val);

/*
 * topology probers
 */
extern const struct blkid_idinfo sysfs_tp_idinfo;

#endif /* BLKID_TOPOLOGY_H */

