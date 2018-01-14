#include <assert.h>

#include "ps4.h"

const uint8_t payload_data_const[] =
{
#include "payload_data.inc"
};

uint64_t __readmsr(unsigned long __register)
{
  unsigned long __edx;
  unsigned long __eax;
  __asm__ ("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(__register));
  return (((uint64_t)__edx) << 32) | (uint64_t)__eax;
}

#define X86_CR0_WP (1 << 16)

static inline __attribute__((always_inline)) uint64_t readCr0(void)
{
  uint64_t cr0;
  __asm__ volatile ("movq %0, %%cr0" : "=r" (cr0) : : "memory");
  return cr0;
}

static inline __attribute__((always_inline)) void writeCr0(uint64_t cr0)
{
  __asm__ volatile("movq %%cr0, %0" : : "r" (cr0) : "memory");
}

struct payload_info
{
  uint8_t* buffer;
  size_t size;
};

struct syscall_install_payload_args
{
  void* syscall_handler;
  struct payload_info* payload_info;
};

int syscall_install_payload(void* td, struct syscall_install_payload_args* args)
{
  uint64_t cr0;
  typedef uint64_t vm_offset_t;
  typedef uint64_t vm_size_t;
  typedef void* vm_map_t;

  void* (*kernel_memcpy)(void* dst, const void* src, size_t len);
  void (*kernel_printf)(const char* fmt, ...);
  vm_offset_t (*kmem_alloc)(vm_map_t map, vm_size_t size);

  uint8_t* kernel_base = (uint8_t*)(__readmsr(0xC0000082) - 0x30EB30);

  *(void**)(&kernel_printf) = &kernel_base[0x347580];
  *(void**)(&kernel_memcpy) = &kernel_base[0x286CF0];
  *(void**)(&kmem_alloc) = &kernel_base[0x369500];
  vm_map_t kernel_map = *(void**)&kernel_base[0x1FE71B8];

  kernel_printf("\n\n\n\npayload_installer: starting\n");
  kernel_printf("payload_installer: kernel base=%lx\n", kernel_base);

  if (!args->payload_info)
  {
    kernel_printf("payload_installer: bad payload info\n");
    return -1;
  }

  uint8_t* payload_data = args->payload_info->buffer;
  size_t payload_size = args->payload_info->size;

  if (!payload_data || *((uint64_t*)(&payload_data[0])) != 0x5041594C4F414432ull)
  {
    kernel_printf("payload_installer: bad payload data\n");
    return -2;
  }

  int desired_size = (payload_size + 0x3FFFull) & ~0x3FFFull; // align size

  // TODO(idc): clone kmem_alloc instead of patching directly
  cr0 = readCr0();
  writeCr0(cr0 & ~X86_CR0_WP);
  kernel_base[0x36958D] = 7;
  kernel_base[0x3695A5] = 7;
  writeCr0(cr0);

  kernel_printf("payload_installer: kmem_alloc\n");
  uint64_t address = (uint64_t)kmem_alloc(kernel_map, desired_size);
  if (!address)
  {
    kernel_printf("payload_installer: kmem_alloc failed\n");
    return -3;
  }

  // TODO(idc): clone kmem_alloc instead of patching directly
  cr0 = readCr0();
  writeCr0(cr0 & ~X86_CR0_WP);
  kernel_base[0x36958D] = 3;
  kernel_base[0x3695A5] = 3;
  writeCr0(cr0);

  kernel_printf("payload_installer: installing...\n");
  kernel_printf("payload_installer: target=%lx\n", address);
  kernel_printf("payload_installer: payload=%lx,%lu\n", payload_data, payload_size);

  kernel_printf("payload_installer: memcpy\n");
  kernel_memcpy((void*)address, payload_data, payload_size);

  uint64_t real_info_offset = *((uint64_t*)&payload_data[8]);
  kernel_printf("payload_installer: patching payload pointers\n");
  if (real_info_offset != 0 && real_info_offset < payload_size)
  {
    uint64_t* real_info = (uint64_t*)(&payload_data[real_info_offset]);
    for (; real_info[0] != 0; real_info += 2)
    {
      uint64_t* target = (uint64_t*)(address + real_info[1]);
      *target = (uint64_t)(&kernel_base[real_info[0]]);
      kernel_printf("  %x(%lx) = %x(%lx)\n", real_info[1], target, real_info[0], *target);
    }
  }

  uint64_t cave_info_offset = *((uint64_t*)&payload_data[16]);
  kernel_printf("payload_installer: patching caves\n");
  if (cave_info_offset != 0 && cave_info_offset < payload_size)
  {
    uint64_t* cave_info = (uint64_t*)(&payload_data[cave_info_offset]);
    for (; cave_info[0] != 0 && cave_info[1] != 0; cave_info += 2)
    {
      uint8_t* target = &kernel_base[cave_info[0]];
      kernel_printf("  %lx(%lx) : %lx(%lx)\n", cave_info[0], target, cave_info[1], address + cave_info[1]);

#pragma pack(push,1)
      struct
      {
        uint8_t op[2];
        int32_t disp;
        uint64_t address;
      }
      jmp;
#pragma pack(pop)
      jmp.op[0] = 0xFF;
      jmp.op[1] = 0x25;
      jmp.disp = 0;
      jmp.address = address + cave_info[1];
      cr0 = readCr0();
      writeCr0(cr0 & ~X86_CR0_WP);
      kernel_memcpy(target, &jmp, sizeof(jmp));
      writeCr0(cr0);
    }
  }

  kernel_printf("payload_installer: patching calls\n");
  uint64_t disp_info_offset = *((uint64_t*)&payload_data[24]);
  if (disp_info_offset != 0 && disp_info_offset < payload_size)
  {
    uint64_t* disp_info = (uint64_t*)(&payload_data[disp_info_offset]);
    for (; disp_info[0] != 0 && disp_info[1] != 0; disp_info += 2)
    {
      uint8_t* cave = &kernel_base[disp_info[1]];
      uint8_t* rip = &kernel_base[disp_info[0] + 5];
      int32_t disp = (int32_t)(cave - rip);
      uint8_t* target = &kernel_base[disp_info[0] + 1];

      kernel_printf("  %lx(%lx)\n", disp_info[0]+1, target);
      kernel_printf("    %lx(%lx) -> %lx(%lx) = %d\n", disp_info[0]+5, rip, disp_info[1], cave, disp);

      cr0 = readCr0();
      writeCr0(cr0 & ~X86_CR0_WP);
      *((int32_t*)target) = disp;
      writeCr0(cr0);
    }
  }

  uint64_t entrypoint_offset = *((uint64_t*)&payload_data[32]);
  if (entrypoint_offset != 0 && entrypoint_offset < payload_size)
  {
    kernel_printf("payload_installer: entrypoint\n");
    void (*payload_entrypoint)();
    *((void**)&payload_entrypoint) = (void*)(&payload_data[entrypoint_offset]);
    payload_entrypoint();
  }

  kernel_printf("payload_installer: done\n");
  return 0;
}

int _main(void)
{
  uint8_t* payload_data = (uint8_t*)(&payload_data_const[0]);
  size_t payload_size = sizeof(payload_data_const);

  initKernel();
  struct payload_info payload_info;
  payload_info.buffer = payload_data;
  payload_info.size = payload_size;
  errno = 0;
  int result = kexec(&syscall_install_payload, &payload_info);
  return !result ? 0 : errno;
}
