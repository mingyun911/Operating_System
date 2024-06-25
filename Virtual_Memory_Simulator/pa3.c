/**********************************************************************
 * Copyright (c) 2020-2024
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr;

/**
 * TLB of the system.
 */
extern struct tlb_entry tlb[];


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * lookup_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Translate @vpn of the current process through TLB. DO NOT make your own
 *   data structure for TLB, but should use the defined @tlb data structure
 *   to translate. If the requested VPN exists in the TLB and it has the same
 *   rw flag, return true with @pfn is set to its PFN. Otherwise, return false.
 *   The framework calls this function when needed, so do not call
 *   this function manually.
 *
 * RETURN
 *   Return true if the translation is cached in the TLB.
 *   Return false otherwise
 */
bool lookup_tlb(unsigned int vpn, unsigned int rw, unsigned int *pfn)
{
    for(int x = 0; x < NR_TLB_ENTRIES; x++) {
        if(tlb[x].valid && tlb[x].vpn == vpn) {
            if((rw == 1 && (tlb[x].rw == 1 || tlb[x].rw == 3)) || (rw == 2 && tlb[x].rw == 3)) {
                *pfn = tlb[x].pfn;
                return true;
            }
        }
    }
    return false;
}


/**
 * insert_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Insert the mapping from @vpn to @pfn for @rw into the TLB. The framework will
 *   call this function when required, so no need to call this function manually.
 *   Note that if there exists an entry for @vpn already, just update it accordingly
 *   rather than removing it or creating a new entry.
 *   Also, in the current simulator, TLB is big enough to cache all the entries of
 *   the current page table, so don't worry about TLB entry eviction. ;-)
 */
void insert_tlb(unsigned int vpn, unsigned int rw, unsigned int pfn)
{
    /// 메모리 는 디스크 의 캐시 이다. localization 캐싱 방법을 이용해 최근 사용한 값은 tlb 에 올린다
    /// 원래는 무슨 process 인지도 확인 해야 하는데, 이는 안 하는 듯 함
    /// However, if a TLB entry should be removed for some reasons, reuse the TLB entry for storing the next TLB entry insertion.
    /// 이건 뭔지 모르 겠음
    //// priv = 순서
    int tag = 0;
    for(int x = 0; x < NR_TLB_ENTRIES; x++){
        if(tlb[x].valid == true && tlb[x].vpn == vpn){
            tlb[x].rw = rw;
            tlb[x].vpn = vpn;
            tlb[x].pfn = pfn;
            tag = 1;
            break;
        }
    }
    if(tag == 1)
        return;

    else{
        for(int x = 0; x < NR_TLB_ENTRIES; x++){
            if(tlb[x].valid == 0){
                tlb[x].valid = 1;
                tlb[x].vpn = vpn;
                tlb[x].pfn = pfn;
                tlb[x].rw = rw;
                return;
            }
        }
    }
}


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with ACCESS_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with ACCESS_READ should not be accessible with
 *   ACCESS_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
#include <string.h>

unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
    int pd_index = vpn / NR_PTES_PER_PAGE;
    int pte_index = vpn % NR_PTES_PER_PAGE;
    struct pte_directory *pd;
    struct pte *pte;
    if (current->pagetable.pdes[pd_index] == NULL) {
        current->pagetable.pdes[pd_index] = malloc(sizeof(struct pte_directory));
        memset(current->pagetable.pdes[pd_index], 0, sizeof(struct pte_directory));
    }
    pd = current->pagetable.pdes[pd_index];
    pte = &pd->ptes[pte_index];
    int target;
    for (int x = 0; x < NR_PAGEFRAMES; x++) {
        if (mapcounts[x] == 0) {
            mapcounts[x]++;
            target = x;
            pte->rw = rw;
            pte->pfn = target;
            pte->valid = true;
            pte->private = rw;
            return target;
        }
    }
    /// 빈 게 없으면 페이지 폴트 해서 하나 victim 만들고 뺴줘 야함
    return -1;
}

/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, rw, pfn) is set @false or 0.
 *   Also, consider the case when a page is shared by two processes,
 *   and one process is about to free the page. Also, think about TLB as well ;-)
 */
void free_page(unsigned int vpn)
{
    int pd_index = vpn / NR_PTES_PER_PAGE;
    int pte_index = vpn % NR_PTES_PER_PAGE;
    current->pagetable.pdes[pd_index]->ptes[pte_index].valid = false;
    current->pagetable.pdes[pd_index]->ptes[pte_index].rw = 0;
    mapcounts[current->pagetable.pdes[pd_index]->ptes[pte_index].pfn]--;
    current->pagetable.pdes[pd_index]->ptes[pte_index].pfn = 0;
    /// 해당 vpn 은 삭제 해줘야 한다.
    for(int x = 0; x < NR_TLB_ENTRIES; x++){
        if(tlb[x].vpn == vpn){
            tlb[x].valid = false;
        }
    }
    return;
}

/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
    int pd_index = vpn / NR_PTES_PER_PAGE;
    int pte_index = vpn % NR_PTES_PER_PAGE;
    /// case 2
    /// cow
    /// 일단 명령이 write 인데 해당 페이지는 read 권한만 있는데, private 이 read write 임
    if(rw == ACCESS_WRITE && current->pagetable.pdes[pd_index]->ptes[pte_index].rw == ACCESS_READ && (current->pagetable.pdes[pd_index]->ptes[pte_index].private != ACCESS_READ && current->pagetable.pdes[pd_index]->ptes[pte_index].private != ACCESS_WRITE)){
        /// valid 부분 수정
        current->pagetable.pdes[pd_index]->ptes[pte_index].rw = current->pagetable.pdes[pd_index]->ptes[pte_index].private;
        /// 오류난 프레임 번호
        int error_pfn = current->pagetable.pdes[pd_index]->ptes[pte_index].pfn;
        /// 해당 프레임 map count -1
        mapcounts[error_pfn]--;

        for(int x = 0; x < NR_PAGEFRAMES; x++){
            /// 새로 할당 받을 프레임 번호
            if(mapcounts[x] == 0){
                current->pagetable.pdes[pd_index]->ptes[pte_index].pfn = x;
                mapcounts[x]++;
                return true;
            }
        }
        return true;
    }
    return false;
}

/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., 즉, @current)
 *   page table.
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for
 *   storing some useful information :-)
 */
/// 실행 되고 있는 프로 세스가 바뀌 어야 한다
/// ready que 에서 pid 에 해당 하는 process 고르고 현재 current 을 다시 ready que 에 넣어 줘야 함
/// ready que 이름은 processes
/// context switch 해야함
/// ptbr 바꿔 줘야함. 새로운 프로 세스 = 새로운 페이지 테이블
void switch_process(unsigned int pid)
{
    struct process *next,*temp;
    int a=0;
    list_for_each_entry(temp,&processes,list) {
        if(temp->pid==pid){
            next=temp;
            a=1;
            break;
        }
    }
    if(a==1){
        printf("DEBUG FOUND\n");
        list_del_init(&next->list);
        list_add_tail(&current->list, &processes);
        current = next;
        ptbr = &current->pagetable;
        for(int x = 0; x < NR_TLB_ENTRIES; x++){
            tlb[x].valid = false;
            tlb[x].vpn = 0;
            tlb[x].pfn = 0;
            tlb[x].rw = 0;
        }
    }
    else{
        printf("DEBUG NOT FOUND\n");
        next=malloc(sizeof(struct process));
        memset(next, 0, sizeof(struct process));
        next->pid=pid;
        for (int x = 0; x < NR_PDES_PER_PAGE; x++) {
            next->pagetable.pdes[x] = malloc(sizeof(struct pte_directory));
            memset(next->pagetable.pdes[x], 0, sizeof(struct pte_directory));
        }
        for(int x=0;x<NR_PDES_PER_PAGE;x++){
            if(current->pagetable.pdes[x]){
                for(int y=0;y<NR_PTES_PER_PAGE;y++){
                    if (current->pagetable.pdes[x]->ptes[y].valid == true) {
                        /// copy
                        int target_frame = current->pagetable.pdes[x]->ptes[y].pfn;
                        int target_valid = current->pagetable.pdes[x]->ptes[y].valid;
                        int target_rw = current->pagetable.pdes[x]->ptes[y].rw;
                        int target_private = current->pagetable.pdes[x]->ptes[y].private;
                        next->pagetable.pdes[x]->ptes[y].valid = target_valid;
                        next->pagetable.pdes[x]->ptes[y].pfn = target_frame;
                        next->pagetable.pdes[x]->ptes[y].rw = target_rw;
                        next->pagetable.pdes[x]->ptes[y].private = target_private;
                        /// w 떼주기
                        if (target_rw != ACCESS_WRITE && target_rw != ACCESS_READ) {
                            target_rw = ACCESS_READ;
                            next->pagetable.pdes[x]->ptes[y].rw = ACCESS_READ;
                            current->pagetable.pdes[x]->ptes[y].rw = ACCESS_READ;
                        }
                        /// map counts 증가
                        mapcounts[target_frame]++;
                    }
                }
            }
        }
        for(int x = 0; x < NR_TLB_ENTRIES; x++){
            tlb[x].valid = false;
            tlb[x].vpn = 0;
            tlb[x].pfn = 0;
            tlb[x].rw = 0;
        }
        list_add_tail(&current->list, &processes);
        current = next;
        ptbr = &current->pagetable;
    }
    return;
}