#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

struct proc;
// copied from xnu

typedef int32_t sy_call_t(struct proc *, void *, int *);
typedef void    sy_munge_t(const void *, void *);

struct sysent {     /* system call table */
    int16_t     sy_narg;    /* number of args */
    int8_t      sy_resv;    /* reserved  */
    int8_t      sy_flags;   /* flags */
    sy_call_t   *sy_call;   /* implementing function */
    sy_munge_t  *sy_arg_munge32; /* system call arguments munger for 32-bit process */
    sy_munge_t  *sy_arg_munge64; /* system call arguments munger for 64-bit process */
    int32_t     sy_return_type; /* system call return types */
    uint16_t    sy_arg_bytes;   /* Total size of arguments in bytes for
                     * 32-bit system calls
                     */
};
#define _SYSCALL_RET_INT_T      1   

// end copied

#define SCRATCH 0xc076c000
#define SYSENT 0xc0255924

// search for 01 00 00 00 0c 00 00 00

struct sysent my_sysent = { 1, 0, 0, (void *) (SCRATCH | 1), NULL, NULL, _SYSCALL_RET_INT_T, 3 * sizeof(uint32_t) };

int mysyscall(uint32_t a, uint32_t b, uint32_t c) {
    return syscall(8, a, b, c);
}

int main() {
    assert(sizeof(struct sysent) == 0x18);
    int fd = open("kcode.bin", O_RDONLY);
    assert(fd > 0);
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *buf = malloc(size);
    assert(read(fd, buf, size) == size);
    int k = open("/dev/kmem", O_WRONLY);
    assert(k > 0);
    assert(pwrite(k, buf, size, SCRATCH) == size);
    assert(pwrite(k, &my_sysent, sizeof(struct sysent), SYSENT + 8 * sizeof(struct sysent)) == sizeof(struct sysent));
    printf("ok I wrote it\n"); sleep(3);
    printf("%d\n", mysyscall(0, 0, 0));
}