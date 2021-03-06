#include "kinc.h"
#include "iphone_4_openiboot.bin.h"
// black.c
void *hook(void *addr, void *replacement, bool force);
void unhook(void *stub);
// creep.c
int creep_go(void *start, int size);
void creep_get_records(user_addr_t buf, uint32_t bufsize);
void creep_stop();
// protoss.c
int protoss_go();
int protoss_go_watch(uint32_t address, uint32_t mask);
int protoss_get_records(int type, user_addr_t buf, uint32_t bufsize);
void protoss_stop();
void protoss_unload();
uint32_t protoss_dump_debug_reg(uint32_t reg);
int protoss_write_debug_reg(uint32_t reg, uint32_t val);
// failsafe.S
int run_failsafe(void *result, void *func, uint32_t arg1, uint32_t arg2);

//void *PEHaltRestart(void* something);
extern int (*PE_halt_restart)(unsigned int type);

struct mysyscall_args {
    uint32_t mode;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
};

static int boot_oib(unsigned int type) {
uint32_t* addr1 = (uint32_t*) 0xBF10202C;
uint32_t* addr2 = (uint32_t*) 0xBF102024;
uint32_t* addr3 = (uint32_t*) 0xBF102020;
*addr1 = 0;
*addr2 = 1;
*addr3 = 0x80000000;
*addr1 = 4;
*addr3 = 0;
return 0;
}

static uint32_t oib_call_change() {
//	boot_oib();
//	hook(PEHaltRestart, boot_oib, 0);
//	hook(PEHaltRestart, ((void (*)()) (uint32_t)&iphone_4_openiboot), 0);
//	PE_halt_restart = ((int (*)(unsigned int type)) iphone_4_openiboot);
	PE_halt_restart = boot_oib;
	return (uint32_t)&iphone_4_openiboot;
}

#define VOID_STAR_A1_THROUGH_12 void *a1, void *a2, void *a3, void *a4, void *a5, void *a6, void *a7, void *a8, void *a9, void *a10, void *a11, void *a12
#define A1_THROUGH_7 a1, a2, a3, a4, a5, a6, a7
#define A1_THROUGH_12 a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12

static void *(*vt_old)(VOID_STAR_A1_THROUGH_12);
static void *vt_hook(VOID_STAR_A1_THROUGH_12) {
    void *result = vt_old(A1_THROUGH_12);
    IOLog("vt_hook: from:%p <- %p <- %p <- %p <- %p <- %p r0=%p r1=%p r2=%p r3=%p a5=%p a6=%p a7=%p vt=%p result=%p\n",
        __builtin_return_address(0),
        __builtin_return_address(1),
        __builtin_return_address(2),
        __builtin_return_address(3),
        __builtin_return_address(4),
        __builtin_return_address(5),
        A1_THROUGH_7, *((void **) a1), result);
    return result;
}

static void *(*ttbr_old)(VOID_STAR_A1_THROUGH_12);
static void *ttbr_hook(VOID_STAR_A1_THROUGH_12) {
    uint32_t ttbr0, ttbr1;
    asm("mrc p15, 0, %0, c2, c0, 0" :"=r"(ttbr0));
    asm("mrc p15, 0, %0, c2, c0, 1" :"=r"(ttbr1));
    IOLog("ttbr_hook: from:%p <- %p <- %p r0=%p r1=%p r2=%p r3=%p a5=%p a6=%p a7=%p ttbr0=%x ttbr1=%x\n", __builtin_return_address(0), __builtin_return_address(1), __builtin_return_address(2), A1_THROUGH_7, ttbr0, ttbr1);
    return ttbr_old(A1_THROUGH_12);
}

static void *(*logger_old)(VOID_STAR_A1_THROUGH_12);
static void *logger_hook(VOID_STAR_A1_THROUGH_12) {
    void *result = logger_old(A1_THROUGH_12);
    IOLog("logger_hook: from:%p <- %p <- %p <- %p <- %p <- %p r0=%p r1=%p r2=%p r3=%p a5=%p a6=%p a7=%p result=%p pid=%d sp=%p\n",
        __builtin_return_address(0),
        __builtin_return_address(1),
        __builtin_return_address(2),
        __builtin_return_address(3),
        __builtin_return_address(4),
        __builtin_return_address(5),
        A1_THROUGH_7,
        result,
        proc_pid(current_proc()),
        &result);
    return result;
}

static void *(*tracer_old)(VOID_STAR_A1_THROUGH_12);
static bool tracer_did_trace;
static void *tracer_hook(VOID_STAR_A1_THROUGH_12) {
    bool should_trace = !tracer_did_trace;
    tracer_did_trace = true;
    if(should_trace) protoss_go();
    void *result = tracer_old(A1_THROUGH_12);
    if(should_trace) protoss_stop();
    IOLog("tracer_hook: from:%p <- %p <- %p r0=%p r1=%p r2=%p r3=%p a5=%p a6=%p a7=%p result=%p\n", __builtin_return_address(0), __builtin_return_address(1), __builtin_return_address(2), A1_THROUGH_7, result);
    return result;
}

static int (*vm_fault_enter_old)(void *m, void *pmap, uint32_t vaddr, vm_prot_t prot, boolean_t wired, boolean_t change_wiring, boolean_t no_cache, int *type_of_fault);
static int vm_fault_enter_hook(void *m, void *pmap, uint32_t vaddr, vm_prot_t prot, boolean_t wired, boolean_t change_wiring, boolean_t no_cache, int *type_of_fault) {
    if((vaddr & 0xf0000000) == 0x10000000) {
        if(!(vaddr & 0xfffff)) { // xxx
            IOLog("vm_map_enter: vaddr=%08x pmap=%p prot=%x wired=%d change_wiring=%d no_cache=%d\n", vaddr, pmap, prot, wired, change_wiring, no_cache);
        }
    }
    return vm_fault_enter_old(m, pmap, vaddr, prot, wired, change_wiring, no_cache, type_of_fault);
}

void *(*weird_old)(VOID_STAR_A1_THROUGH_12);
void *weird_hook(VOID_STAR_A1_THROUGH_12) {
    void *result = weird_old(A1_THROUGH_12);
    uint32_t *p = (uint32_t *) &p;
    for(int i = 0; i < 32; i++) {
        IOLog("%03x: %08x %08x %08x %08x\n", 16*i, p[0], p[1], p[2], p[3]);
        p += 4;
    }
    return result;
}

static int do_something_usb_related() {
    char *base = (void *) 0xd3edc000;
    for(int i = 0; i < 8; i++) {
        volatile uint32_t *control = (void *) (base + i*0x20 + 0x900);
        uint32_t c = *control;
        IOLog("%x\n", c);
    }
    *((volatile uint32_t *) (base + 3*0x20 + 0x914)) = 0x40001000;
    *((volatile uint32_t *) (base + 3*0x20 + 0x910)) = (1 << 19) | 63;
    *((volatile uint32_t *) (base + 3*0x20 + 0x900)) |= 0x84000000;
    IOLog("%08x\n", *((volatile uint32_t *) (base + 3*0x20 + 0x900)));
    IOSleep(100);
    IOLog("%08x\n", *((volatile uint32_t *) (base + 3*0x20 + 0x900)));

    return 0;
}

static int ioreg(uint32_t type, user_addr_t path) {
    char buf[128];
    size_t done;
    copyinstr(path, buf, sizeof(buf), &done);
    void *regentry;
    if(type == 128) {
        regentry = IORegistryEntry_fromPath(buf, NULL, NULL, NULL, NULL);
    } else {
        void *matching;
        switch(type) {
        case 134:
            matching = IOService_serviceMatching(buf, NULL);
            break;
        case 135:
            matching = IOService_nameMatching(buf, NULL);
            break;
        default:
            IOLog("?\n");
            return -1;
        }
        void *iterator = IOService_getMatchingServices(matching);
        if(!iterator) {
            IOLog("null iterator\n");
            return -1;
        }
        void *object;
        regentry = NULL;
        while(object = OSIterator_getNextObject(iterator)) {
            if(!regentry) regentry = object;
            IOLog("- %p\n", object);
        }
        IOLog("\n");
        OSObject_release(iterator);
        OSObject_release(matching);
    }
    return (int) regentry;
}

static uint32_t lookup_metaclass(user_addr_t name) {
    char buf[128];
    size_t done;
    copyinstr(name, buf, sizeof(buf), &done);
    void *symbol = OSSymbol_withCString(buf);
    uint32_t result = (uint32_t) OSMetaClass_getMetaClassWithName(symbol);
    OSObject_release(symbol);
    return result;
}

uint32_t get_proc_map(int pid) {
    struct proc *p = proc_find(pid);
    if(!p) return 0;
    return p->task->map->pmap->phys;
}

static int poke_mem(void *kaddr, uint32_t uaddr, uint32_t size, bool write, bool phys) {
    void *descriptor = 0, *map = 0;
    int retval;
    if(phys) {
        descriptor = IOMemoryDescriptor_withPhysicalAddress((uint32_t) kaddr, size, write ? kIODirectionOut : kIODirectionIn);
        if(!descriptor) {
            IOLog("couldn't create descriptor\n");
            return -1;
        }
        map = IOMemoryDescriptor_map(descriptor, 0);
        if(!map) {
            IOLog("couldn't map descriptor\n");
            OSObject_release(descriptor);
            return -1;
        }
        kaddr = IOMemoryMap_getAddress(map);
    }

    if(write) {
        retval = copyin(uaddr, kaddr, size);
    } else {
        retval = copyout(kaddr, uaddr, size);
    }

    if(phys) {
        OSObject_release(map);
        OSObject_release(descriptor);
    }
    return retval;
}


int do_something() {
    void *matching = IOService_serviceMatching("AppleS5L8930XIO", NULL); 
    void *service = IOService_waitForMatchingService(matching, 1000000000ULL);
    if(!service) return -1;
    OSObject_release(matching);
    void *platform_device = ((void **) service)[0x54/4];
    if(!platform_device) return -2;
    kern_return_t r = IOService_unregisterInterrupt(platform_device, 0);
    if(r) return r;

    for(unsigned int i = 0x10000; i < 0xFFF0FFFF; i += 0x10000) { // fuck endians
        void *descriptor = IOMemoryDescriptor_withPhysicalAddress(i, 65536, kIODirectionIn);
        void *map = IOMemoryDescriptor_map(descriptor, 0);
        char *q = ((uint32_t *) map)[0x14/4];
        
        if (*((unsigned int *) q) == 0xea00000e && *((unsigned int *) (q + 4)) == 0xe59ff018) {
            return i;
        }
    }
   
    return -4;
}

// from the loader
extern struct sysent sysent[];
struct sysent saved_sysent;

int mysyscall(void *p, struct mysyscall_args *uap, int32_t *retval);
__attribute__((constructor))
void init() {
    IOLog("init %p\n", mysyscall);
    saved_sysent = sysent[8];
    sysent[8] = (struct sysent){ 1, 0, 0, (void *) mysyscall, NULL, NULL, _SYSCALL_RET_INT_T, sizeof(struct mysyscall_args) };
}

void fini_() {
    IOLog("unhook\n");
    creep_stop();
    protoss_unload();
    unhook(logger_old); logger_old = NULL;
    unhook(ttbr_old); ttbr_old = NULL;
    unhook(tracer_old); tracer_old = NULL; tracer_did_trace = false;
    unhook(vt_old); vt_old = NULL;
    unhook(vm_fault_enter_old); vm_fault_enter_old = NULL;
    unhook(weird_old); weird_old = NULL;
}

__attribute__((destructor))
void fini() {
    fini_();
    sysent[8] = saved_sysent;
}

// keep in sync with stuff.c
struct regs {
    uint32_t cpsr;
    uint32_t ttbr0;
    uint32_t ttbr1;
    uint32_t ttbcr;
    uint32_t contextidr;
    uint32_t sctlr;
    uint32_t scr;
    uint32_t dbgdidr;
    uint32_t dbgdrar;
    uint32_t dbgdsar;
    uint32_t id_dfr0;
    uint32_t dbgdscr;
    uint32_t tpidrprw;
    uint32_t dacr;
};

int mysyscall(void *p, struct mysyscall_args *uap, int32_t *retval)
{
    //IOLog("Hi mode=%d\n", uap->mode);
    //IOLog("kernel_pmap = %p nx_enabled = %d\n", kernel_pmap, kernel_pmap[0x420/4]);
    // Turn off nx_enabled so we can make pages executable in kernel land.
    kernel_pmap[0x420/4] = 0;
    switch(uap->mode) {
    case 0: { // get regs
        struct regs regs;
        asm("mrs %0, cpsr" :"=r"(regs.cpsr));
        asm("mrc p15, 0, %0, c2, c0, 0" :"=r"(regs.ttbr0));
        asm("mrc p15, 0, %0, c2, c0, 1" :"=r"(regs.ttbr1));
        asm("mrc p15, 0, %0, c2, c0, 2" :"=r"(regs.ttbcr));
        asm("mrc p15, 0, %0, c13, c0, 1" :"=r"(regs.contextidr));
        asm("mrc p15, 0, %0, c1, c0, 0" :"=r"(regs.sctlr));
        //asm("mcr p15, 0, %0, c1, c1, 0" :: "r"(1 << 6));
        asm("mrc p15, 0, %0, c1, c1, 0" :"=r"(regs.scr));
        asm("mrc p14, 0, %0, c0, c0, 0" :"=r"(regs.dbgdidr));
        asm("mrc p14, 0, %0, c1, c0, 0" :"=r"(regs.dbgdrar));
        asm("mrc p14, 0, %0, c2, c0, 0" :"=r"(regs.dbgdsar));
        asm("mrc p15, 0, %0, c0, c1, 2" :"=r"(regs.id_dfr0));
        asm("mrc p14, 0, %0, c0, c1, 0" : "=r"(regs.dbgdscr));
        asm("mrc p15, 0, %0, c13, c0, 4" : "=r"(regs.tpidrprw));
        asm("mrc p15, 0, %0, c3, c0, 0" : "=r"(regs.dacr));
        int error;
        if(error = copyout(&regs, (user_addr_t) uap->b, sizeof(regs))) return error;
        *retval = 0;
        break;
    }
    case 1: // copy data
        *retval = poke_mem((void *) uap->b, uap->c, uap->d, uap->e, uap->f);
        break;
    case 4: // crash
        ((void (*)()) 0xdeadbeef)();
        *retval = 0;
        break;
    case 6: // unhook
        fini_();
        break;
    case 7: // hook a function, log args
        *retval = 0;
        if(!(logger_old = hook((void *) uap->b, logger_hook, uap->c))) {
            *retval = -1;
        }
        break;
    case 29:
        *retval = 0;
        if(!(ttbr_old = hook((void *) uap->b, ttbr_hook, uap->c))) {
            *retval = -1;
        }
        break;
    case 8: // hook vm_fault_enter
        *retval = 0;
        if(!(vm_fault_enter_old = hook((void *) uap->b, vm_fault_enter_hook, false))) {
            *retval = -1;
        }
        break;
    case 9: // hook weird
        *retval = 0;
        if(!(weird_old = hook((void *) uap->b, weird_hook, false))) {
            *retval = -1;
        }
        break;
    case 10:
        *retval = creep_go((void *) uap->b, (int) uap->c);
        break;
    case 11:
        creep_get_records((user_addr_t) uap->b, uap->c);
        *retval = 0;
        break;
    case 12:
        *retval = do_something_usb_related();
        break;
    case 13:
        *retval = ioreg(uap->b, (user_addr_t) uap->c);
        break;
    case 14:
        *retval = protoss_get_records(uap->b, (user_addr_t) uap->c, uap->d);
        break;
    case 15:
        if(!(*retval = protoss_go())) {
            IOLog("Hi\n");
            protoss_stop();
        }
        break;
    case 19: // vt
        *retval = 0;
        if(!(vt_old = hook((void *) uap->b, vt_hook, false))) {
            *retval = -1;
        }
        break;
    case 20: // tracer
        *retval = 0;
        tracer_did_trace = false;
        if(!(tracer_old = hook((void *) uap->b, tracer_hook, uap->c))) {
            *retval = -1;
        }
        break;
    case 21:
        *retval = lookup_metaclass(uap->b);
        break;
    case 22:
        *retval = protoss_go_watch(uap->b, uap->c);
        break;
    case 23:
        *retval = protoss_dump_debug_reg(uap->b);
        break;
    case 24:
        *retval = protoss_write_debug_reg(uap->b, uap->c);
        break;
    case 25: {
        void *metaclass;
        if(run_failsafe(&metaclass, &OSObject_getMetaClass, uap->b, 0)) {
            *retval = 5;
        } else {
            const char *name = OSMetaClass_getClassName(metaclass);
            size_t done;
            *retval = copyoutstr(name, uap->c, uap->d, &done);
        }
        break;
    }
    case 26:
        *retval = OSObject_getRetainCount((void *) uap->b);
        break;
    case 27:
        *retval = do_something();
        break;
    case 28:
        *retval = (int32_t) get_proc_map((int) uap->b);
        break;
    case 30:
	*retval = oib_call_change();
	break;
    case 31: // crash
        ((void (*)()) uap->b)();
        *retval = 0;
        break;
    default:
        IOLog("Unknown mode %d\n", uap->mode);
        *retval = -1;
        break;
    }

    return 0;
}
