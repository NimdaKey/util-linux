/*
 * sysfs based topology -- gathers topology information from Linux sysfs
 *
 * Copyright (C) 2009 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "topology.h"

static unsigned long dev_topology_attribute(const char *attribute,
				dev_t dev, dev_t *primary)
{
	const char *sysfs_fmt_str = "/sys/dev/block/%d:%d/%s";
	char path[PATH_MAX];
	int len;
	FILE *fp = NULL;
	struct stat info;
	unsigned long result = 0UL;

	len = snprintf(path, sizeof(path), sysfs_fmt_str,
			major(dev), minor(dev), attribute);
	if (len < 0 || len + 1 > sizeof(path))
		goto err;

	/*
	 * check if the desired sysfs attribute exists
	 * - if not: either the kernel doesn't have topology support or the
	 *   device could be a partition
	 */
	if (stat(path, &info) < 0) {
		if (!*primary &&
		    blkid_devno_to_wholedisk(dev, NULL, 0, primary))
			goto err;

		/* get attribute from partition's primary device */
		len = snprintf(path, sizeof(path), sysfs_fmt_str,
				major(*primary), minor(*primary), attribute);
		if (len < 0 || len + 1 > sizeof(path))
			goto err;
	}

	fp = fopen(path, "r");
	if (!fp) {
		DBG(DEBUG_LOWPROBE, printf(
			"topology: %s: fopen failed, errno=%d\n", path, errno));
		goto err;
	}

	if (fscanf(fp, "%lu", &result) != 1) {
		DBG(DEBUG_LOWPROBE, printf(
			"topology: %s: unexpected file format\n", path));
		goto err;
	}

	fclose(fp);

	DBG(DEBUG_LOWPROBE,
		printf("topology: attribute %s = %lu\n", attribute, result));

	return result;
err:
	if (fp)
		fclose(fp);
	DBG(DEBUG_LOWPROBE,
		printf("topology: failed to read %s attribute\n", attribute));
	return 0;
}

/*
 * Sysfs topology values
 */
static struct topology_val {

	/* /sys/dev/block/<maj>:<min>/NAME */
	const char *sysfs_name;

	/* function to set probing resut */
	int (*set_result)(blkid_probe, unsigned long);

} topology_vals[] = {
	{ "alignment_offset", blkid_topology_set_alignment_offset },
	{ "queue/minimum_io_size", blkid_topology_set_minimum_io_size },
	{ "queue/optimal_io_size", blkid_topology_set_optimal_io_size },
};

static int probe_sysfs_tp(blkid_probe pr, const struct blkid_idmag *mag)
{
	dev_t dev, pri_dev = 0;
	int i, rc = 0, count = 0;

	dev = blkid_probe_get_devno(pr);
	if (!dev)
		goto nothing;		/* probably not a block device */

	for (i = 0; i < ARRAY_SIZE(topology_vals); i++) {
		struct topology_val *val = &topology_vals[i];
		unsigned long data;

		/*
		 * Don't bother reporting any of the topology information
		 * if it's zero.
		 */
		data = dev_topology_attribute(val->sysfs_name, dev, &pri_dev);
		if (!data)
			continue;

		rc = val->set_result(pr, data);
		if (rc)
			goto err;
		count++;
	}

	if (count)
		return 0;
nothing:
	return 1;
err:
	return -1;
}

const struct blkid_idinfo sysfs_tp_idinfo =
{
	.name		= "sysfs",
	.probefunc	= probe_sysfs_tp,
	.magics		= BLKID_NONE_MAGIC
};

