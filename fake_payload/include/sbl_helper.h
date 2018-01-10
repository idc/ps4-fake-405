#ifndef __SBL_HELPER_H
#define __SBL_HELPER_H

struct sbl_mapped_page_group;

#define SIZEOF_SBL_MAP_LIST_ENTRY 0x50 // sceSblDriverMapPages

TYPE_BEGIN(struct sbl_map_list_entry, SIZEOF_SBL_MAP_LIST_ENTRY);
  TYPE_FIELD(struct sbl_map_list_entry* next, 0x00);
  TYPE_FIELD(struct sbl_map_list_entry* prev, 0x08);
  TYPE_FIELD(unsigned long cpu_va, 0x10);
  TYPE_FIELD(unsigned int num_page_groups, 0x18);
  TYPE_FIELD(unsigned long gpu_va, 0x20);
  TYPE_FIELD(struct sbl_mapped_page_group* page_groups, 0x28);
  TYPE_FIELD(unsigned int num_pages, 0x30);
  TYPE_FIELD(unsigned long flags, 0x38);
  TYPE_FIELD(struct proc* proc, 0x40);
  TYPE_FIELD(void* vm_page, 0x48);
TYPE_END();

#define SBL_MSG_SERVICE_MAILBOX_MAX_SIZE 0x80

#endif
