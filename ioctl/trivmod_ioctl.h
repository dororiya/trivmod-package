#ifndef TRIVMOD_IOCTL_H
#define TRIVMOD_IOCTL_H

#include <linux/ioctl.h>

/* the statistics the driver sends to the program */
struct trivmod_stats {
    int open_count;      /* how many times opened */
    int release_count;   /* how many times closed */
    int users;           /* open right now */
    int bytes_written;   /* total bytes written */
    int bytes_read;      /* total bytes read */
    int buf_used;        /* bytes waiting in the buffer now */
};

#define TRIVMOD_MAGIC 't'

/* get statistics: driver -> program */
#define TRIVMOD_GET_STATS   _IOR(TRIVMOD_MAGIC, 1, struct trivmod_stats)

/* clear statistics: no data */
#define TRIVMOD_CLEAR_STATS _IO(TRIVMOD_MAGIC, 2)

#endif
