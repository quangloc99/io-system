#ifndef __CONFIG_H__
#define __CONFIG_H__

// for debugging
// #define PRINT_EVENT 

#define VARIANT_NAME "var1"

#define NUM_BUF_SIZE 15
#define READ_BUF_SIZE (1 << 12)   // 1 page

// values stolen from the Linux driver article series
// well they did not match any real device anyway
#define DEV_FIRST_MAJOR 0
#define DEV_COUNT 1

#endif // __CONFIG_H__
