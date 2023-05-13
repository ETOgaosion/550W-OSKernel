#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."


/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE * img, int num);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, first = 1;
    int lastbytes = 0, filenum = 0;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen(IMAGE_FILE, "w+");
    /* for each input file */
    while (nfiles-- > 0) {

        lastbytes = nbytes;
        /* open input file */
        fp = fopen(*files, "r+");
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {
            if(options.extended == 1)
            printf("\tsegment %d\n",ph);

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
        }
        if(filenum != 0)
            write_os_size(nbytes - lastbytes, img, filenum);
        fclose(fp);
        files++;
        filenum++;
    }

    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    int size = sizeof(Elf64_Ehdr);
    fread(ehdr, 1, size, fp);
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    int offset = ehdr.e_phoff + ph * ehdr.e_phentsize;
    fseek(fp, offset, 0);
    fread(phdr, 1, sizeof(Elf64_Phdr), fp);
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{
    int fp_offset = phdr.p_offset;
    int fp_vaddr = phdr.p_vaddr;
    int fp_filesz = phdr.p_filesz;
    int fp_memsz = phdr.p_memsz;
    int fp_writing = fp_filesz;
    
    int img_offset = *nbytes;
    //fillsz: count of zeros to fill
    int fillsz = 512 - (phdr.p_filesz % 512);
    (*nbytes) += phdr.p_filesz;
 
    if(fillsz != 512) (*nbytes) += fillsz;
    int fp_padding = *nbytes;

    unsigned char info;
    fseek(fp, fp_offset, 0);
    fseek(img, img_offset, 0);
    for(int i=0;i<phdr.p_filesz;i++)
    {
        info = fgetc(fp);
        fputc(info, img);
    }
    for(int i=0;i<fillsz;i++)
    {
        fputc(0, img);
    }
    if(options.extended == 1)
    {
        printf("\t\t offset 0x%04x\t\t vaddr 0x%04x\n",fp_offset,fp_vaddr);
        printf("\t\t filesz 0x%04x\t\t memsz 0x%04x\n",fp_filesz,fp_memsz);
        if(fp_memsz != 0)
        printf("\t\t writing 0x%04x bytes\n\t\t padding up to 0x%04x\n",fp_writing,fp_padding);
    }
}

static void write_os_size(int nbytes, FILE * img, int num)
{
    int size = nbytes/512;
    int offset = (num - 1) * 2 + 0x1f0;
    fseek(img, offset, 0);
    fwrite(&size, 2, 1, img);

    if(options.extended == 1)
    printf("os_size: %d sectors\n",size);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
