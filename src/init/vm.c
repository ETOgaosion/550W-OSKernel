#include <asm/context.h>
#include <asm/pgtable.h>
#include <asm/vm.h>
#include <common/elf.h>

typedef void (*kernel_entry_t)(unsigned long, uintptr_t);

extern unsigned char _elf_main[];
extern unsigned _length_main;

uintptr_t directmap(uintptr_t kva, uintptr_t pgdir) {
    // ignore pgdir
    return kva;
}

// kernel_entry_t start_kernel = NULL;

/*********** start here **************/
int prepare_vm(unsigned long mhartid) {
    if (mhartid == 0) {
        setup_vm();
        // start_kernel = (kernel_entry_t)load_elf(_elf_main, _length_main, PGDIR_PA, directmap);
    } else {
        enable_vm();
    }
    // start_kernel(mhartid);
    return mhartid;
}
