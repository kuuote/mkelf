#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>

// PROG内の機械語にELFヘッダーをくっつけてAOUTに吐き出す

#define AOUT "a.out"
#define PROG "program"

Elf64_Ehdr* write_ehdr(Elf64_Ehdr *eh)
{
  //マジックナンバー
  //最初から 0x7f 'E' 'L' 'F'
  eh->e_ident[EI_MAG0] = ELFMAG0;
  eh->e_ident[EI_MAG1] = ELFMAG1;
  eh->e_ident[EI_MAG2] = ELFMAG2;
  eh->e_ident[EI_MAG3] = ELFMAG3;

  //x86-64なのでELFCLASS64=0x2
  eh->e_ident[EI_CLASS] = ELFCLASS64;

  //IntelのCPUはリトルエンディアンなのでELFDATA2LSB=0x1
  eh->e_ident[EI_DATA] = ELFDATA2LSB;

  //ELFのバージョン、EV_CURRENT=0x1しかないのでそれでよい
  eh->e_ident[EI_VERSION] = EV_CURRENT;

  //オブジェクトのOSとABI、LinuxなのでELFOSABI_LINUX=ELFOSABI_GNU=0x3
  eh->e_ident[EI_OSABI] = ELFOSABI_LINUX;
  //EI_ABIVERSIONは0x0でいいのとe_identの後はパディングなのでmemsetしてればおk
  
  //後はhost == targetなので何も考えずに代入していく
  //host != targetの場合はbyteswap噛ます必要がありそう

  //実行可能ファイルなのでET_EXEC=0x0002
  eh->e_type = ET_EXEC;

  //x86_64なのでEM_X86_64=0x003e
  eh->e_machine = EM_X86_64;

  //EV_CURRENTしかないので必然的に0x0001
  eh->e_version = EV_CURRENT;

  //プロセスが開始する仮想アドレス
  //GNU ldと合わせておく
  //0x401000で始まっているのは、ファイル上の機械語の位置が0x1000であり
  //アライメント(後述するp_align)に沿って合わせる必要があるため
  eh->e_entry = 0x401000;

  //プログラムヘッダーの開始番地、実行可能ファイルなので当然必要
  //Ehdrの直後にくっつければ計算不要
  //Linuxでは0x40になる
  eh->e_phoff = sizeof(Elf64_Ehdr);

  //セクションヘッダーの開始番地
  //デバッグしなければいらない
  //セクションが無ければ0でおk
  //TODO:セクション使う場合は考慮が必要
  eh->e_shoff = 0x0;

  //現在使われていないらしいので0x0
  eh->e_flags = 0x0;

  //ELFヘッダー、つまりElf64_Ehdrのサイズ=0x40
  eh->e_ehsize = sizeof(Elf64_Ehdr);

  //プログラムヘッダー一つ辺りのサイズ、つまりElf64_Phdrのサイズ=0x38
  //全てのプログラムヘッダーのサイズは同じ
  eh->e_phentsize = sizeof(Elf64_Phdr);

  //プログラムヘッダーの数
  //今回は雑にやるので1
  //真面目にやるならread only領域とか作るので増やすべし
  eh->e_phnum = 1;

  //セクションヘッダー一つ辺りのサイズ、つまりElf64_Shdrのサイズ=0x40
  //全てのセクションヘッダーのサイズは同じ
  eh->e_shentsize = sizeof(Elf64_Shdr);

  //セクションヘッダーの数
  //今回は作らないので0x0
  eh->e_shnum = 0;

  //セクションヘッダーテーブルインデックス
  //今回はセクション無いのでSHN_UNDEF=0x0
  eh->e_shstrndx = SHN_UNDEF;

  return eh;
}

Elf64_Phdr* write_phdr(Elf64_Phdr* ph, int program_size)
{
  //プログラムヘッダーの種類
  //ロードするのでPT_LOAD=0x1
  ph->p_type = PT_LOAD;
  //ファイル上のプログラムの位置
  //今回は0x1000に配置する
  ph->p_offset = 0x1000;
  //仮想メモリ上の位置
  //GNU ldに合わせる
  ph->p_vaddr = 0x401000;
  //物理アドレス上の位置
  //多分使われていないのでGNU ldに合わせてvaddrと同じにしておく
  ph->p_paddr = 0x401000;
  //プログラムのサイズ
  ph->p_filesz = program_size;
  ph->p_memsz = program_size;
  //セグメントの属性を指定する
  //読めるし書けるし実行できるのでさいつよ
  //セキュリティホールになるので本番では真面目に指定しましょう
  ph->p_flags = PF_X | PF_W | PF_R;
  //セクションのアライメント
  //GNU ldに合わせておく
  //p_offsetとp_vaddrがp_alignを法として合同でなければ動作しない
  ph->p_align = 0x1000;

  return ph;
}

int main()
{
  //ヘッダー領域は0x1000
  uint8_t *header = calloc(0x1000, sizeof(uint8_t));

  //ELFヘッダー生成
  write_ehdr((Elf64_Ehdr *)(header));

  //ファイルサイズ取得
  struct stat st;
  if(stat(PROG, &st) < 0) {
    fprintf(stderr, "stat error\n");
    return 1;
  }

  //プログラムサイズが確定したのでプログラムヘッダ生成
  write_phdr((Elf64_Phdr *)(header + sizeof(Elf64_Ehdr)), st.st_size);

  //プログラム読み込み
  uint8_t *program = malloc(st.st_size);
  FILE *in = fopen(PROG, "rb");
  if (!in) {
    fprintf(stderr, "input open error\n");
    return 1;
  }
  if (fread(program, sizeof(uint8_t), st.st_size, in) != st.st_size) {
    fprintf(stderr, "read error\n");
    return 1;
  }
  fclose(in);

  //出力して
  FILE *out = fopen(AOUT, "wb");
  if (!out) {
    fprintf(stderr, "output open error\n");
    return 1;
  }
  fwrite(header, sizeof(uint8_t), 0x1000, out);
  fwrite(program, sizeof(uint8_t), st.st_size, out);
  fclose(out);

  //実行可能にする
  if (chmod(AOUT, 0755) < 0) {
    fprintf(stderr, "chmod failed\n");
    return 1;
  }
  return 0;
}
