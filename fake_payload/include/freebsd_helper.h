#ifndef __FREEBSD_HELPER_H
#define __FREEBSD_HELPER_H

#define ESRCH 3
#define ENOMEM 12
#define EINVAL 22
#define ENOTSUP 45

struct lock_object
{
  const char* lo_name;
  uint32_t lo_flags;
  uint32_t lo_data;
  void* lo_witness;
};

struct mtx
{
  struct lock_object lock_object;
  volatile void* mtx_lock;
};

#endif
