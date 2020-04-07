#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>

//INから実行可能領域を切り出してOUTに保存する

#define IN "hello"
#define OUT "program"

int main(int argc, char **argv)
{
  struct stat st;
  if(stat(IN, &st) < 0) {
    fprintf(stderr, "stat error\n");
    return 1;
  }
  uint8_t *buf = malloc(st.st_size);

  FILE *in = fopen(IN, "rb");
  if (!in) {
    fprintf(stderr, "input open error\n");
    return 1;
  }
  if (fread(buf, sizeof(uint8_t), st.st_size, in) != st.st_size) {
    fprintf(stderr, "read error\n");
    return 1;
  }

  Elf64_Ehdr *eh = (Elf64_Ehdr *)(buf);
  //実行できる領域までスキップ
  Elf64_Phdr *ph;
  for(ph = (Elf64_Phdr *)(buf + eh->e_phoff); (ph->p_flags & PF_X) == 0; ph++);

  FILE *out = fopen(OUT, "wb");
  if (!out) {
    fprintf(stderr, "output open error\n");
    return 1;
  }
  if (fwrite(buf + ph->p_offset, sizeof(uint8_t), ph->p_filesz, out) != ph->p_filesz) {
    fprintf(stderr, "write error\n");
    return 1;
  }

  return 0;
}
