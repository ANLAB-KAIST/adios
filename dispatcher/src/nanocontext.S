/* Taken from glibc source, and modified. Removed the call to sigprocmask */

#define oRBX		0x00
#define oRBP		0x08
#define oR12		0x10
#define oR13		0x18
#define oR14		0x20
#define oR15		0x28
#define oRDI		0x30
#define oRIP		0x38
#define oRSP		0x40
#define oMXCSR		0x48
#define oFPUCW		0x4c

.text
.align 4
.globl swapcontext_nano
.type swapcontext_nano, @function

swapcontext_nano:
	/* Save the preserved registers, the registers used for passing args,
	   and the return address.  */
	movq	%rbx, oRBX(%rdi)
	movq	%rbp, oRBP(%rdi)
	movq	%r12, oR12(%rdi)
	movq	%r13, oR13(%rdi)
	movq	%r14, oR14(%rdi)
	movq	%r15, oR15(%rdi)

	movq	%rdi, oRDI(%rdi)

	movq	(%rsp), %rcx
	movq	%rcx, oRIP(%rdi)
	leaq	8(%rsp), %rcx		/* Exclude the return address.  */
	movq	%rcx, oRSP(%rdi)

	fnstcw	oFPUCW(%rdi)
	stmxcsr oMXCSR(%rdi)


	/* Restore the floating-point context.  Not the registers, only the
	   rest.  */
	ldmxcsr oMXCSR(%rsi)
	fldcw	oFPUCW(%rsi)


	/* Load the new stack pointer and the preserved registers.  */
	movq	oRSP(%rsi), %rsp
	movq	oRBX(%rsi), %rbx
	movq	oRBP(%rsi), %rbp
	movq	oR12(%rsi), %r12
	movq	oR13(%rsi), %r13
	movq	oR14(%rsi), %r14
	movq	oR15(%rsi), %r15

	/* The following ret should return to the address set with
	getcontext.  Therefore push the address on the stack.  */
	movq	oRIP(%rsi), %rcx
	pushq	%rcx

	/* Setup registers used for passing args.  */
	movq	oRDI(%rsi), %rdi


	/* Clear rax to indicate success.  */
	xorl	%eax, %eax

	ret


.text
.align 4
.globl getcontext_nano
.type getcontext_nano, @function

getcontext_nano:
	movq	%rbx, oRBX(%rdi)
	movq	%rbp, oRBP(%rdi)
	movq	%r12, oR12(%rdi)
	movq	%r13, oR13(%rdi)
	movq	%r14, oR14(%rdi)
	movq	%r15, oR15(%rdi)

	movq	%rdi, oRDI(%rdi)

	movq	(%rsp), %rcx
	movq	%rcx, oRIP(%rdi)
	leaq	8(%rsp), %rcx		/* Exclude the return address.  */
	movq	%rcx, oRSP(%rdi)

	fnstcw	oFPUCW(%rdi)
	stmxcsr oMXCSR(%rdi)

	/* Clear rax to indicate success.  */
	xorl	%eax, %eax

	ret


.text
.align 4
.globl setcontext_nano
.type setcontext_nano, @function

setcontext_nano:
	/* Restore the floating-point context.  Not the registers, only the
	   rest.  */
	ldmxcsr oMXCSR(%rdi)
	fldcw	oFPUCW(%rdi)

	/* Load the new stack pointer and the preserved registers.  */
	movq	oRSP(%rdi), %rsp
	movq	oRBX(%rdi), %rbx
	movq	oRBP(%rdi), %rbp
	movq	oR12(%rdi), %r12
	movq	oR13(%rdi), %r13
	movq	oR14(%rdi), %r14
	movq	oR15(%rdi), %r15
	/* The following ret should return to the address set with
	getcontext.  Therefore push the address on the stack.  */
	movq	oRIP(%rdi), %rcx
	pushq	%rcx

	/* Setup finally  %rdi.  */
	movq	oRDI(%rdi), %rdi

	/* Clear rax to indicate success.  */
	xorl	%eax, %eax

	ret

