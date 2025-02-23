/* Implementation for gcc's internal stack-allocation routines.  */
#if defined(__i386__) || defined(__x86_64__)
.global __chkstk

#ifdef _WIN64
.align	4
__chkstk:
	popq	%r11
	movq	%rsp, %r10
	cmpq	$0x1000, %rax
	jb	.Lchkstk_end
.Lchkstk_loop:
	subq	$0x1000, %r10
	subq	$0x1000, %rax
	orl	$0x0, (%r10)
	cmpq	$0x1000, %rax
	ja	.Lchkstk_loop
.Lchkstk_end:
	subq	%rax, %r10
	movq	%rsp, %rax
	orl	$0x0, (%r10)
	movq	%r10, %rsp
	pushq	%r11
	ret
#else
__chkstk:
	pushl	%ecx
	leal	8(%esp), %ecx
	cmpl	$0x1000, %eax		/* > 4k ?*/
	jb	chkstk_end
chkstk_loop:
	subl	$0x1000, %ecx
	subl	$0x1000, %eax
	orl	$0x0, (%ecx)
	cmpl	$0x1000, %eax
	ja	chkstk_loop
chkstk_end:
	subl	%eax, %ecx
	orl	$0x0, (%ecx)
	movl	%esp, %eax
	movl	%ecx, %esp
	movl	(%eax), %ecx
	pushl	4(%eax)
	ret
#endif
#endif
