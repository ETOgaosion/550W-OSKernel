#include <common/elf.h>
#include <lib/math.h>

uintptr_t load_elf(elf_info_t *target, bool *is_dynamic, unsigned char elf_binary[], unsigned length, uintptr_t pgdir, uintptr_t (*prepare_page_for_va)(uintptr_t va, uintptr_t pgdir)) {
    target->edata = 0;
    elf64_ehdr_t *ehdr = (elf64_ehdr_t *)elf_binary;
    elf64_phdr_t *phdr = NULL;
    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    unsigned char *ptr_ph_table = NULL;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    int i = 0;

    // check whether `binary` is a ELF file.
    if (length < 4 || !is_elf_format(elf_binary)) {
        return 0; // return NULL when error!
    }

    ptr_ph_table = elf_binary + ehdr->e_phoff;
    ph_entry_count = ehdr->e_phnum;
    ph_entry_size = ehdr->e_phentsize;

    // save all useful message
    target->phoff = ehdr->e_phoff;
    target->phent = ehdr->e_phentsize;
    target->phnum = ehdr->e_phnum;
    target->entry = ehdr->e_entry;

    bool is_first = true;

    while (ph_entry_count--) {
        phdr = (elf64_phdr_t *)ptr_ph_table;
        if (phdr->p_type == PT_INTERP) {
            *is_dynamic = true;
            return 0;
        }

        if (phdr->p_type == PT_LOAD) {
            for (i = 0; i < phdr->p_memsz; i += NORMAL_PAGE_SIZE) {
                if (i < phdr->p_filesz) {
                    unsigned char *bytes_of_page = (unsigned char *)prepare_page_for_va((uintptr_t)(phdr->p_vaddr + i), pgdir);
                    k_memcpy(bytes_of_page, elf_binary + phdr->p_offset + i, MIN(phdr->p_filesz - i, NORMAL_PAGE_SIZE));
                    if (phdr->p_filesz - i < NORMAL_PAGE_SIZE) {
                        for (int j = phdr->p_filesz % NORMAL_PAGE_SIZE; j < NORMAL_PAGE_SIZE; ++j) {
                            bytes_of_page[j] = 0;
                        }
                    }
                } else {
                    long *bytes_of_page = (long *)prepare_page_for_va((uintptr_t)(phdr->p_vaddr + i), pgdir);
                    for (int j = 0; j < NORMAL_PAGE_SIZE / sizeof(long); ++j) {
                        bytes_of_page[j] = 0;
                    }
                }
                target->edata = phdr->p_vaddr + phdr->p_memsz;
                if (is_first) {
                    target->text_begin = phdr->p_vaddr;
                    is_first = 0;
                }
            }
        }

        ptr_ph_table += ph_entry_size;
    }

    return ehdr->e_entry;
}