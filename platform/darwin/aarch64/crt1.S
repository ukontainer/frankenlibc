/* from https://opensource.apple.com/source/Csu/Csu-85/start.s.auto.html */
.text
.global _start
	.align 4
_start:
	nop
	bl      __start1	; main(x0=argc, x1=argv, x2=envp, x3=apple)
	b       _exit
