.globl _start
_start:
	subq $10, %rsp
  movq %rsp, %rdi
  movb $0x48, (%rdi) # H
  movb $0x65, 1(%rdi) # e
  movb $0x6c, 2(%rdi) # l
  movb $0x6c, 3(%rdi) # l
  movb $0x6f, 4(%rdi) # o
  movb $0x20, 5(%rdi) # <Space>
  movb $0x41, 6(%rdi) # A
  movb $0x53, 7(%rdi) # S
  movb $0x4d, 8(%rdi) # M
  movb $0x0a, 9(%rdi) # <NL>
  # Write
  movq $1, %rax # sys_write
  movq $1, %rdi # stdout
  movq $10, %rdx # length
  movq %rsp, %rsi # address
  syscall
	# Exit
	movq $60, %rax # sys_exit
	movq $0, %rdi # normal exit code
	syscall
