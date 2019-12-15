.global __franken_swapcontext
.type __franken_swapcontext,#function
__franken_swapcontext:
	stp x19, x20, [x0,#0]   /* first buffer (prev) */
	stp x21, x22, [x0,#16]
	stp x23, x24, [x0,#32]
	stp x25, x26, [x0,#48]
	stp x27, x28, [x0,#64]
	stp x29, x30, [x0,#80]
	mov x2, sp
	str x2, [x0,#104]       /* save return addr ptr for new sp */
	stp  d8,  d9, [x0,#112]
	stp d10, d11, [x0,#128]
	stp d12, d13, [x0,#144]
	stp d14, d15, [x0,#160]
	str x30, [x0,#176]

	ldp x19, x20, [x1,#0]   /* second buffer (next) */
	ldp x21, x22, [x1,#16]
	ldp x23, x24, [x1,#32]
	ldp x25, x26, [x1,#48]
	ldp x27, x28, [x1,#64]
	ldp x29, x30, [x1,#80]
	ldr x2, [x1,#104]       /* this ends up being the sp */
	mov sp, x2
	ldp d8 , d9, [x1,#112]
	ldp d10, d11, [x1,#128]
	ldp d12, d13, [x1,#144]
	ldp d14, d15, [x1,#160]
	ldr x30, [x1,#176]      /* this is the instruction pointer */

	ldr x0, [x1,#184]       /* load initial argument into x0 */
	mov x3, #0              /* return value of zero */

	br x30
