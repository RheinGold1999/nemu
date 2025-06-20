/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  int mmu_mode = MMU_DIRECT;
  if ((cpu.csr[RV32_CSR_SATP] >> 30) & 0x1) {
    mmu_mode = MMU_TRANSLATE;
  }
  return mmu_mode;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  size_t dir_i = (vaddr >> 22) & 0x3FF;
  size_t tb_i = (vaddr >> 12) & 0x3FF;
  uint32_t *pdir = (uint32_t *)(uintptr_t)(cpu.csr[RV32_CSR_SATP] << 12);
  uint32_t *ptb = (uint32_t *)(uintptr_t)(pdir[dir_i] & (~0xFFF));
  paddr_t pg_addr = ptb[tb_i] & (~0xFFF);
  paddr_t paddr = pg_addr | (vaddr & 0xFFF);
  assert(paddr == vaddr);
  return paddr;
}
