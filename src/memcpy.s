	.file	"protocol.c"
	.text
	.globl	crc16arc
	.type	crc16arc, @function
crc16arc:
.LFB6:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movw	$0, -12(%rbp)
	movw	$-24575, -10(%rbp)
	movl	$0, -8(%rbp)
	jmp	.L2
.L7:
	movl	-8(%rbp), %eax
	movslq	%eax, %rdx
	movq	-24(%rbp), %rax
	addq	%rdx, %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	xorw	%ax, -12(%rbp)
	movl	$0, -4(%rbp)
	jmp	.L3
.L6:
	movzwl	-12(%rbp), %eax
	andl	$1, %eax
	testl	%eax, %eax
	je	.L4
	movzwl	-12(%rbp), %eax
	shrw	%ax
	xorw	-10(%rbp), %ax
	movw	%ax, -12(%rbp)
	jmp	.L5
.L4:
	shrw	-12(%rbp)
.L5:
	addl	$1, -4(%rbp)
.L3:
	cmpl	$7, -4(%rbp)
	jle	.L6
	addl	$1, -8(%rbp)
.L2:
	movl	-8(%rbp), %eax
	cltq
	cmpq	-32(%rbp), %rax
	jb	.L7
	movzwl	-12(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE6:
	.size	crc16arc, .-crc16arc
	.globl	packet_encode
	.type	packet_encode, @function
packet_encode:
.LFB7:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movq	%rdi, -24(%rbp)
	movq	%rsi, -32(%rbp)
	movq	%rdx, -40(%rbp)
	cmpq	$5, -40(%rbp)
	jbe	.L10
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	addl	$6, %eax
	cltq
	cmpq	%rax, -40(%rbp)
	jnb	.L11
.L10:
	call	__errno_location@PLT
	movl	$105, (%rax)
	movl	$0, %eax
	jmp	.L12
.L11:
	movq	-32(%rbp), %rax
	movb	$43, (%rax)
	movq	-32(%rbp), %rax
	leaq	1(%rax), %rcx
	movq	-24(%rbp), %rax
	movl	$3, %edx
	movq	%rax, %rsi
	movq	%rcx, %rdi
	call	memcpy@PLT
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %edx
	movq	-24(%rbp), %rax
	movq	8(%rax), %rax
	movq	-32(%rbp), %rcx
	addq	$4, %rcx
	movq	%rax, %rsi
	movq	%rcx, %rdi
	call	memcpy@PLT
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %edx
	movq	-24(%rbp), %rax
	movq	8(%rax), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	crc16arc
	movw	%ax, -2(%rbp)
	movzwl	-2(%rbp), %eax
	shrw	$8, %ax
	movl	%eax, %ecx
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	addl	$4, %eax
	movslq	%eax, %rdx
	movq	-32(%rbp), %rax
	addq	%rdx, %rax
	movl	%ecx, %edx
	movb	%dl, (%rax)
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	addl	$5, %eax
	movslq	%eax, %rdx
	movq	-32(%rbp), %rax
	addq	%rdx, %rax
	movzwl	-2(%rbp), %edx
	movb	%dl, (%rax)
	movq	-24(%rbp), %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	addl	$6, %eax
	cltq
.L12:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE7:
	.size	packet_encode, .-packet_encode
	.section	.rodata
.LC0:
	.string	"123456789"
.LC1:
	.string	"%ld bytes written.\n"
.LC2:
	.string	"%#04x "
	.text
	.globl	main
	.type	main, @function
main:
.LFB8:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$64, %rsp
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movl	$261, %edi
	call	malloc@PLT
	movq	%rax, -56(%rbp)
	leaq	.LC0(%rip), %rax
	movq	%rax, -48(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, %rdi
	call	strlen@PLT
	movb	%al, -32(%rbp)
	movb	$0, -31(%rbp)
	movb	$0, -30(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, -24(%rbp)
	movw	$0, -16(%rbp)
	movq	-56(%rbp), %rcx
	leaq	-32(%rbp), %rax
	movl	$261, %edx
	movq	%rcx, %rsi
	movq	%rax, %rdi
	call	packet_encode
	movq	%rax, -40(%rbp)
	movq	-40(%rbp), %rax
	leaq	.LC1(%rip), %rdx
	movq	%rax, %rsi
	movq	%rdx, %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$0, -60(%rbp)
	jmp	.L14
.L15:
	movl	-60(%rbp), %eax
	movslq	%eax, %rdx
	movq	-56(%rbp), %rax
	addq	%rdx, %rax
	movzbl	(%rax), %eax
	movzbl	%al, %eax
	leaq	.LC2(%rip), %rdx
	movl	%eax, %esi
	movq	%rdx, %rdi
	movl	$0, %eax
	call	printf@PLT
	addl	$1, -60(%rbp)
.L14:
	movl	-60(%rbp), %eax
	cltq
	cmpq	-40(%rbp), %rax
	jb	.L15
	movl	$0, %eax
	movq	-8(%rbp), %rdx
	subq	%fs:40, %rdx
	je	.L17
	call	__stack_chk_fail@PLT
.L17:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE8:
	.size	main, .-main
	.ident	"GCC: (GNU) 16.1.1 20260430"
	.section	.note.GNU-stack,"",@progbits
