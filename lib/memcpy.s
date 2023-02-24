 .syntax unified

 .arm
 .arch armv7-a
 .fpu vfpv3

 .macro cpy_line_vfp vreg, base
 vstr \vreg, [ip, #\base]
 vldr \vreg, [r1, #\base]
 vstr d0, [ip, #\base + 8]
 vldr d0, [r1, #\base + 8]
 vstr d1, [ip, #\base + 16]
 vldr d1, [r1, #\base + 16]
 vstr d2, [ip, #\base + 24]
 vldr d2, [r1, #\base + 24]
 vstr \vreg, [ip, #\base + 32]
 vldr \vreg, [r1, #\base + 5 * 64 - 32]
 vstr d0, [ip, #\base + 40]
 vldr d0, [r1, #\base + 40]
 vstr d1, [ip, #\base + 48]
 vldr d1, [r1, #\base + 48]
 vstr d2, [ip, #\base + 56]
 vldr d2, [r1, #\base + 56]
 .endm

 .macro cpy_tail_vfp vreg, base
 vstr \vreg, [ip, #\base]
 vldr \vreg, [r1, #\base]
 vstr d0, [ip, #\base + 8]
 vldr d0, [r1, #\base + 8]
 vstr d1, [ip, #\base + 16]
 vldr d1, [r1, #\base + 16]
 vstr d2, [ip, #\base + 24]
 vldr d2, [r1, #\base + 24]
 vstr \vreg, [ip, #\base + 32]
 vstr d0, [ip, #\base + 40]
 vldr d0, [r1, #\base + 40]
 vstr d1, [ip, #\base + 48]
 vldr d1, [r1, #\base + 48]
 vstr d2, [ip, #\base + 56]
 vldr d2, [r1, #\base + 56]
 .endm


.global memcpy_arm; .type memcpy_arm,%function; .align 6; memcpy_arm: .fnstart; .cfi_startproc;

 mov ip, r0
 cmp r2, #64
 bhs .Lcpy_not_short



.Ltail63unaligned:

 and r3, r2, #0x3c
 add ip, ip, r3
 add r1, r1, r3
 rsb r3, r3, #(60 - 8/2 + 4/2)

 add pc, pc, r3, lsl #1

 ldr r3, [r1, #-60]
 str r3, [ip, #-60]

 ldr r3, [r1, #-56]
 str r3, [ip, #-56]
 ldr r3, [r1, #-52]
 str r3, [ip, #-52]

 ldr r3, [r1, #-48]
 str r3, [ip, #-48]
 ldr r3, [r1, #-44]
 str r3, [ip, #-44]

 ldr r3, [r1, #-40]
 str r3, [ip, #-40]
 ldr r3, [r1, #-36]
 str r3, [ip, #-36]

 ldr r3, [r1, #-32]
 str r3, [ip, #-32]
 ldr r3, [r1, #-28]
 str r3, [ip, #-28]

 ldr r3, [r1, #-24]
 str r3, [ip, #-24]
 ldr r3, [r1, #-20]
 str r3, [ip, #-20]

 ldr r3, [r1, #-16]
 str r3, [ip, #-16]
 ldr r3, [r1, #-12]
 str r3, [ip, #-12]

 ldr r3, [r1, #-8]
 str r3, [ip, #-8]
 ldr r3, [r1, #-4]
 str r3, [ip, #-4]


 lsls r2, r2, #31
 ldrhcs r3, [r1], #2
 ldrbne r1, [r1]
 strhcs r3, [ip], #2
 strbne r1, [ip]
 bx lr

.Lcpy_not_short:

 str r10, [sp, #-32]!
 and r10, r1, #7
 and r3, ip, #7
 cmp r3, r10
 bne .Lcpy_notaligned





 vmov.f32 s0, s0





 lsls r10, ip, #29
 beq 1f
 rsbs r10, r10, #0
 sub r2, r2, r10, lsr #29
 ldrmi r3, [r1], #4
 strmi r3, [ip], #4
 lsls r10, r10, #2
 ldrhcs r3, [r1], #2
 ldrbne r10, [r1], #1
 strhcs r3, [ip], #2
 strbne r10, [ip], #1

1:
 subs r10, r2, #64
 blo .Ltail63aligned

 cmp r10, #512
 bhs .Lcpy_body_long

.Lcpy_body_medium:

1:
 vldr d0, [r1, #0]
 subs r10, r10, #64
 vldr d1, [r1, #8]
 vstr d0, [ip, #0]
 vldr d0, [r1, #16]
 vstr d1, [ip, #8]
 vldr d1, [r1, #24]
 vstr d0, [ip, #16]
 vldr d0, [r1, #32]
 vstr d1, [ip, #24]
 vldr d1, [r1, #40]
 vstr d0, [ip, #32]
 vldr d0, [r1, #48]
 vstr d1, [ip, #40]
 vldr d1, [r1, #56]
 vstr d0, [ip, #48]
 add r1, r1, #64
 vstr d1, [ip, #56]
 add ip, ip, #64
 bhs 1b
 tst r10, #0x3f
 beq .Ldone

.Ltail63aligned:
 and r3, r10, #0x38
 add ip, ip, r3
 add r1, r1, r3
 rsb r3, r3, #(56 - 8 + 4)
 add pc, pc, r3

 vldr d0, [r1, #-56]
 vstr d0, [ip, #-56]
 vldr d0, [r1, #-48]
 vstr d0, [ip, #-48]
 vldr d0, [r1, #-40]
 vstr d0, [ip, #-40]
 vldr d0, [r1, #-32]
 vstr d0, [ip, #-32]
 vldr d0, [r1, #-24]
 vstr d0, [ip, #-24]
 vldr d0, [r1, #-16]
 vstr d0, [ip, #-16]
 vldr d0, [r1, #-8]
 vstr d0, [ip, #-8]

 tst r10, #4
 ldrne r3, [r1], #4
 strne r3, [ip], #4
 lsls r10, r10, #31
 ldrhcs r3, [r1], #2
 ldrbne r10, [r1]
 strhcs r3, [ip], #2
 strbne r10, [ip]

.Ldone:
 ldr r10, [sp], #32
 bx lr

.Lcpy_body_long:

 vldr d3, [r1, #0]
 vldr d4, [r1, #64]
 vldr d5, [r1, #128]
 vldr d6, [r1, #192]
 vldr d7, [r1, #256]

 vldr d0, [r1, #8]
 vldr d1, [r1, #16]
 vldr d2, [r1, #24]
 add r1, r1, #32

 subs r10, r10, #5 * 64 * 2
 blo 2f
1:
 cpy_line_vfp d3, 0
 cpy_line_vfp d4, 64
 cpy_line_vfp d5, 128
 add ip, ip, #3 * 64
 add r1, r1, #3 * 64
 cpy_line_vfp d6, 0
 cpy_line_vfp d7, 64
 add ip, ip, #2 * 64
 add r1, r1, #2 * 64
 subs r10, r10, #5 * 64
 bhs 1b

2:
 cpy_tail_vfp d3, 0
 cpy_tail_vfp d4, 64
 cpy_tail_vfp d5, 128
 add r1, r1, #3 * 64
 add ip, ip, #3 * 64
 cpy_tail_vfp d6, 0
 vstr d7, [ip, #64]
 vldr d7, [r1, #64]
 vstr d0, [ip, #64 + 8]
 vldr d0, [r1, #64 + 8]
 vstr d1, [ip, #64 + 16]
 vldr d1, [r1, #64 + 16]
 vstr d2, [ip, #64 + 24]
 vldr d2, [r1, #64 + 24]
 vstr d7, [ip, #64 + 32]
 add r1, r1, #96
 vstr d0, [ip, #64 + 40]
 vstr d1, [ip, #64 + 48]
 vstr d2, [ip, #64 + 56]
 add ip, ip, #128
 add r10, r10, #5 * 64
 b .Lcpy_body_medium

.Lcpy_notaligned:
 pld [r1]
 pld [r1, #64]



 lsls r10, ip, #29
 pld [r1, #(2 * 64)]
 beq 1f
 rsbs r10, r10, #0
 sub r2, r2, r10, lsr #29
 ldrmi r3, [r1], #4
 strmi r3, [ip], #4
 lsls r10, r10, #2
 ldrbne r3, [r1], #1
 ldrhcs r10, [r1], #2
 strbne r3, [ip], #1
 strhcs r10, [ip], #2
1:
 pld [r1, #(3 * 64)]
 subs r2, r2, #64
 ldrlo r10, [sp], #32
 blo .Ltail63unaligned
 pld [r1, #(4 * 64)]

 sub r1, r1, #4
 sub ip, ip, #8
 subs r10, r2, #64
 ldr r2, [r1, #4]
 ldr r3, [r1, #8]
 strd r4, r5, [sp, #8]
 ldr r4, [r1, #12]
 ldr r5, [r1, #16]
 strd r6, r7, [sp, #16]
 ldr r6, [r1, #20]
 ldr r7, [r1, #24]
 strd r8, r9, [sp, #24]
 ldr r8, [r1, #28]
 ldr r9, [r1, #32]!
 b 1f
 .p2align 6
2:
 pld [r1, #(5 * 64) - (32 - 4)]
 strd r2, r3, [ip, #40]
 ldr r2, [r1, #36]
 ldr r3, [r1, #40]
 strd r4, r5, [ip, #48]
 ldr r4, [r1, #44]
 ldr r5, [r1, #48]
 strd r6, r7, [ip, #56]
 ldr r6, [r1, #52]
 ldr r7, [r1, #56]
 strd r8, r9, [ip, #64]!
 ldr r8, [r1, #60]
 ldr r9, [r1, #64]!
 subs r10, r10, #64
1:
 strd r2, r3, [ip, #8]
 ldr r2, [r1, #4]
 ldr r3, [r1, #8]
 strd r4, r5, [ip, #16]
 ldr r4, [r1, #12]
 ldr r5, [r1, #16]
 strd r6, r7, [ip, #24]
 ldr r6, [r1, #20]
 ldr r7, [r1, #24]
 strd r8, r9, [ip, #32]
 ldr r8, [r1, #28]
 ldr r9, [r1, #32]
 bcs 2b


 strd r2, r3, [ip, #40]
 add r1, r1, #36
 strd r4, r5, [ip, #48]
 ldrd r4, r5, [sp, #8]
 strd r6, r7, [ip, #56]
 ldrd r6, r7, [sp, #16]
 strd r8, r9, [ip, #64]
 ldrd r8, r9, [sp, #24]
 add ip, ip, #72
 ands r2, r10, #0x3f

 ldr r10, [sp], #32
 bne .Ltail63unaligned
 bx lr

.cfi_endproc; .fnend; .size memcpy_arm, .-memcpy_arm;
