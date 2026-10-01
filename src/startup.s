	.module startup

	.globl _firmware_boot
	.globl _isr_b_tx_stub
	.globl _isr_b_ext_stub
	.globl _isr_b_rx_stub
	.globl _isr_b_special_stub
	.globl _isr_a_tx_stub
	.globl _isr_a_ext_stub
	.globl _isr_a_rx_stub
	.globl _isr_a_special_stub

	.globl _hardware_set_im2
	.globl _hardware_irq_disable
	.globl _hardware_irq_enable
	.globl _hardware_random

	.area _HEADER (ABS)
	.org 0x0000
reset:
	di
	ld sp,#0x0000
	xor a
	ld hl,#0x8000
	ld b,#0x00
clear_state_page:
	ld (hl),a
	inc hl
	djnz clear_state_page
	call _firmware_boot
halt_forever:
	halt
	jr halt_forever

	.org 0x0100
im2_vectors:
	.dw _isr_b_tx_stub
	.dw _isr_b_ext_stub
	.dw _isr_b_rx_stub
	.dw _isr_b_special_stub
	.dw _isr_a_tx_stub
	.dw _isr_a_ext_stub
	.dw _isr_a_rx_stub
	.dw _isr_a_special_stub

	.area _CODE
_hardware_set_im2:
	ld a,#0x01
	ld i,a
	im 2
	ei
	ret

_hardware_irq_disable:
	di
	ret

_hardware_irq_enable:
	ei
	ret

_hardware_random:
	ld a,r
	ret
