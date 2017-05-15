/* Stanko Novakovic
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_REGION_PAGES;
#define RMAP 1
#define GETREF 2
#define RUNMAP 0

typedef struct ioctl_info {
  int op;
  int node_id;
  int desc_gref;
} ioctl_info_t;


int main() {
  int i;  
  int fd; // = rmc_open((char *)"/root/node");
  
  if ((fd=open("/root/node", O_RDWR|O_SYNC)) < 0) {
    return -1;
  }
  
  ioctl_info_t info; // = (ioctl_info_t *)malloc(sizeof(memory_region_reference_t));

  info.op = GETREF;
  info.node_id = 1;
  if(ioctl(fd, 0, (void *)&info) == -1) {
    printf("[soft_rmc] failed to unmap a remote region\n");
    return -1;
  }

  //memory_region_reference_t *ref = (memory_region_reference_t *)info;
  printf("number of grant references %u\n", info.desc_gref);
}
