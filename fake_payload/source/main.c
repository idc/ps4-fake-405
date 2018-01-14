#include <stddef.h>
#include <stdint.h>

#include "sections.h"
#include "sparse.h"
#include "freebsd_helper.h"
#include "elf_helper.h"
#include "self_helper.h"
#include "sbl_helper.h"

void* M_TEMP PAYLOAD_DATA;
void* (*real_malloc)(unsigned long size, void* type, int flags) PAYLOAD_DATA;
void (*real_free)(void* addr, void* type) PAYLOAD_DATA;

void (*real_dealloc)(void*) PAYLOAD_DATA;
void* (*real_memcpy)(void* dst, const void* src, size_t len) PAYLOAD_DATA;
void (*real_printf)(const char* fmt, ...) PAYLOAD_DATA;

int (*real_sceSblServiceMailbox)(unsigned long service_id, uint8_t request[SBL_MSG_SERVICE_MAILBOX_MAX_SIZE], void* response) PAYLOAD_DATA;
int (*real_sceSblAuthMgrGetSelfInfo)(struct self_context* ctx, struct self_ex_info** info) PAYLOAD_DATA;
void (*real_sceSblAuthMgrSmStart)(void**) PAYLOAD_DATA;
int (*real_sceSblAuthMgrIsLoadable2)(struct self_context* ctx, struct self_auth_info* old_auth_info, int path_id, struct self_auth_info* new_auth_info) PAYLOAD_DATA;
int (*real_sceSblAuthMgrVerifyHeader)(struct self_context* ctx) PAYLOAD_DATA;

extern const struct sbl_map_list_entry** sbl_driver_mapped_pages PAYLOAD_DATA;
extern const uint8_t* mini_syscore_self_binary PAYLOAD_DATA;

extern int my_sceSblAuthMgrIsLoadable2(struct self_context* ctx, struct self_auth_info* old_auth_info, int path_id, struct self_auth_info* new_auth_info) PAYLOAD_CODE;
extern int my_sceSblAuthMgrVerifyHeader(struct self_context* ctx) PAYLOAD_CODE;
extern int my_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox(unsigned long service_id, uint8_t* request, void* response) PAYLOAD_CODE;
extern int my_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox(unsigned long service_id, uint8_t* request, void* response) PAYLOAD_CODE;

PAYLOAD_CODE void my_entrypoint()
{
  // initialization, etc
}

struct real_info
{
  const size_t kernel_offset;
  const void* payload_target;
};

struct cave_info
{
  const size_t kernel_offset;
  const void* payload_target;
};

struct disp_info
{
  const size_t call_offset;
  const size_t cave_offset;
};

struct real_info real_infos[] PAYLOAD_DATA =
{
  { 0x1D1700, &real_malloc },
  { 0x1D18D0, &real_free },
  { 0x286CF0, &real_memcpy },
  { 0x347580, &real_printf },
  { 0x606F40, &real_sceSblServiceMailbox },
  { 0x614A80, &real_sceSblAuthMgrIsLoadable2 },
  { 0x614AE0, &real_sceSblAuthMgrVerifyHeader },
  { 0x615360, &real_sceSblAuthMgrGetSelfInfo },
  { 0x6153F0, &real_sceSblAuthMgrSmStart },
  { 0x134B730, &M_TEMP },
  { 0x136B3E8, &mini_syscore_self_binary },
  { 0x234ED68, &sbl_driver_mapped_pages },
  { 0, NULL },
};

struct cave_info cave_infos[] PAYLOAD_DATA =
{
  { 0x6116F1, &my_sceSblAuthMgrIsLoadable2 },
  { 0x612EA1, &my_sceSblAuthMgrVerifyHeader },
  { 0x617A32, &my_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox },
  { 0x617B80, &my_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox },
  { 0, NULL },
};

struct disp_info disp_infos[] PAYLOAD_DATA =
{
  { 0x6119B5, 0x6116F1 }, // my_sceSblAuthMgrIsLoadable2
  { 0x612149, 0x612EA1 }, // my_sceSblAuthMgrVerifyHeader
  { 0x612D81, 0x612EA1 }, // my_sceSblAuthMgrVerifyHeader
  { 0x616A6D, 0x617A32 }, // my_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox
  { 0x6176C4, 0x617B80 }, // my_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox
  { 0, 0 },
};

struct
{
  uint64_t signature;
  struct real_info* real_infos;
  struct cave_info* cave_infos;
  struct disp_info* disp_infos;
  void* entrypoint;
}
payload_header PAYLOAD_HEADER =
{
  0x5041594C4F414432ull,
  real_infos,
  cave_infos,
  disp_infos,
  &my_entrypoint,
};

// dummies -- not included in output payload binary

void PAYLOAD_DUMMY dummy()
{
  dummy();
}

int _start()
{
  return 0;
}
