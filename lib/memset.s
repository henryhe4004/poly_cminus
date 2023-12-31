    .text
    .align
    .global memset32
    .type   memset32, %function
    .global memset16
    .type   memset16, %function
        /*
         * Optimized memset32 and memset16 for ARM.
         *
         * void memset16(uint16_t* dst, uint16_t value, size_t size);
         * void memset32(uint32_t* dst, uint32_t value, size_t size);
         *
         */
memset16:
        .fnstart
        cmp         r2, #1
        bxle        lr
        /* expand the data to 32 bits */
        mov         r1, r1, lsl #16
        orr         r1, r1, r1, lsr #16
        /* align to 32 bits */
        tst         r0, #2
        strneh      r1, [r0], #2
        subne       r2, r2, #2
        .fnend
memset32:
        .fnstart
        .save       {lr}
        str         lr, [sp, #-4]!
        /* align the destination to a cache-line */
        mov         r12, r1
        mov         lr, r1
        rsb         r3, r0, #0
        ands        r3, r3, #0x1C
        beq         .Laligned32
        cmp         r3, r2
        andhi       r3, r2, #0x1C
        sub         r2, r2, r3
        /* conditionally writes 0 to 7 words (length in r3) */
        movs        r3, r3, lsl #28
        stmcsia     r0!, {r1, lr}
        stmcsia     r0!, {r1, lr}
        stmmiia     r0!, {r1, lr}
        movs        r3, r3, lsl #2
        strcs       r1, [r0], #4
.Laligned32:
        mov         r3, r1
1:      subs        r2, r2, #32
        stmhsia     r0!, {r1,r3,r12,lr}
        stmhsia     r0!, {r1,r3,r12,lr}
        bhs         1b
        add         r2, r2, #32
        /* conditionally stores 0 to 30 bytes */
        movs        r2, r2, lsl #28
        stmcsia     r0!, {r1,r3,r12,lr}
        stmmiia     r0!, {r1,lr}
        movs        r2, r2, lsl #2
        strcs       r1, [r0], #4
        strmih      lr, [r0], #2
        ldr         lr, [sp], #4
        bx          lr
        .fnend
