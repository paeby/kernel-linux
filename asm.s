# Version 2.0

.set nobreak

/**
  * Initialize the stack of a process in such a way that it can be read
  * from the transfer function.
  * First element should be the return address.
  * The last element is the interrupt switch status. We initialize it to 1.
  * A pointer to the stack pointer is returned.
  */
.global _createStack
.text
_createStack: #r4 = newSP
			  #r5 = newPC
			  #r6 = stackSize - 4
	   # pointer to the bottom of the stack
	   add r2, r4, r6
	   # init sp with r8
	   addi r8, r2, -100 # sp
	   stw  r5, 0(r8)   # sp[0] = PC
	   addi r9, r0, 1
	   stw  r9, 96(r8)  # sp[24] = status = 1
	   # store sp on the stack bottom
	   stw  r8, 0(r2)
	   # return pointer to stack address
	   ret

/**
 * Context switch called from either a normal context or from an interrupt routine.
 * For those reason every registers and interrupt switch status *must* be saved.
 * We use bret instruction instead of ret at the to do restore the status.
 * (eret instruction retores estatus into status register, while jumping at ea)
 */
.global _transfer
.text
_transfer:
	addi sp, sp, -100
	stw ra,  0(sp)
    stw fp,  4(sp)
    stw r2,  8(sp)
    stw r3,  12(sp)
    stw r4,  16(sp)
    stw r5,  20(sp)
    stw r6,  24(sp)
    stw r7,  28(sp)
    stw r8,  32(sp)
    stw r9,  36(sp)
    stw r10, 40(sp)
    stw r11, 44(sp)
    stw r12, 48(sp)
    stw r13, 52(sp)
    stw r14, 56(sp)
    stw r15, 60(sp)
    stw r16, 64(sp)
    stw r17, 68(sp)
    stw r18, 72(sp)
    stw r19, 76(sp)
    stw r20, 80(sp)
    stw r21, 84(sp)
    stw r22, 88(sp)
    stw r23, 92(sp)
	# save the current interrupt switch status
    rdctl r2, status
    stw   r2, 96(sp)
    # running->sp = sp
    ldw r2, %gprel(running)(gp)
    stw sp, (r2)
    # running = nextP
	ldw r2, %gprel(nextP)(gp)
	stw r2, %gprel(running)(gp)
	# set sp to the sp from the nextP
	ldw sp, (r2)
	# return using bret -> ba
	ldw ba,  0(sp)
    ldw fp,  4(sp)
    ldw r2,  8(sp)
    ldw r3,  12(sp)
    ldw r4,  16(sp)
    ldw r5,  20(sp)
    ldw r6,  24(sp)
    ldw r7,  28(sp)
    ldw r8,  32(sp)
    ldw r9,  36(sp)
    ldw r10, 40(sp)
    ldw r11, 44(sp)
    ldw r12, 48(sp)
    ldw r13, 52(sp)
    ldw r14, 56(sp)
    ldw r15, 60(sp)
    ldw r16, 64(sp)
    ldw r17, 68(sp)
    ldw r18, 72(sp)
    ldw r19, 76(sp)
    ldw r20, 80(sp)
    ldw r21, 84(sp)
    ldw r22, 88(sp)
	# restore interrupt switch status into bstatus
    ldw r23, 96(sp)
    wrctl bstatus, r23
    ldw r23, 92(sp)

	addi sp, sp, 100
	# bret will copy back bstatus into status and go to ba
	bret


.global maskInterrupts
.text
maskInterrupts:
	addi r9, r0, 0
	wrctl status, r9
	ret

.global allowInterrupts
.text
allowInterrupts:
	addi r9, r0, 1
	wrctl status, r9
	ret

.end


	
	
	


       
	
