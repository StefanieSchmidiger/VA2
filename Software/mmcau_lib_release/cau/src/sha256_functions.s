#******************************************************************************
#******************************************************************************
#
# Copyright (c) Freescale Semiconductor, Inc 2011.
#
include(cau2_defines.hdr)
#*******************************************************************************
#*******************************************************************************
#
# NOTE: Code alignment for the CAU library functions in this file is carefully
# constructed to maximize processor performance. The existing alignment should
# be maintained for best results!!
#
# The individual code segments for the functions are presented first, followed
# by a common read-only data area containing the required data constants.
#
#*******************************************************************************
#*******************************************************************************


#*******************************************************************************
#*******************************************************************************
#
# SHA256: Initializes the hash output and checks the CAU hardware revision
#
#   arguments
#           *output     pointer to 256-bit message digest output
#
#   return
#           0           no error -> CAU2 hardware present
#           -1          error -> incorrect CAU hardware revision
#
#   calling convention
#   int     cau_sha256_initialize_output (unsigned int *output)

    global  _cau_sha256_initialize_output
    global  cau_sha256_initialize_output
    align   4
_cau_sha256_initialize_output:
cau_sha256_initialize_output:
    mov.l   (4,%sp),%a0                 # get argument: *output
    lea     (sha256_init_h.w,%pc),%a1   # pointer to initial data

# copy initial data into hash output buffer (32 bytes = 8 longwords)
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+

    cp0st.l %d0,&STR+CASR               # store CAUSR -> d0
    clr.b   %d0                         # clear CAUSR[7:0]
    cmp.l   %d0,&0x20000000             # verify CAU2 hardware: CASR[31:28] = 0x2
    sne     %d0                         # if not equal, then d0 = 0xff, else d0 = 0x00
    extb.l  %d0                         # return 32-bit int
    rts


#*******************************************************************************
#*******************************************************************************
#
# SHA256: Perform the hash for one or more input message blocks and generate the
#         message digest output
#
#   arguments
#           *input      pointer to start of input message data
#           num_blks    number of 512-bit blocks to process
#           *output     pointer to 256-bit message digest output
#
#   NOTE    Input message and digest output blocks must not overlap
#
#   calling convention
#   void    cau_sha256_hash_n (const unsigned int *input,
#                              const int          num_blks,
#                              unsigned int *output)

    global  _cau_sha256_hash_n
    global  cau_sha256_hash_n
    align   4
_cau_sha256_hash_n:
cau_sha256_hash_n:
    lea     (-268,%sp),%sp              # allocate stack space: 3 regs, int w[64] array

# the stack frame layout for this function is:
#   0(%sp)  saved %d7
#   4(%sp)  saved %a2
#   8(%sp)  saved %a3
#  12(%sp)  local variable w[0] - array has 64 32-bit entries, 256 bytes total
# 268(%sp)  returnPC
# 272(%sp)  input argument 1 - pointer to message input[]
# 276(%sp)  input argument 2 - number of blocks
# 280(%sp)  input argument 3 - pointer to message digest output[]

    movm.l  &0x0c80,(%sp)               # save d7/a2/a3
    mov.l   (272,%sp),%a0               # pointer to message input[]
    mov.l   (280,%sp),%a1               # pointer to message digest output[]

# initialize the CAU data registers with the current contents of output[]
    cp0ld.l (%a1)+,&LDR+CA0             # output[0] = h[0]
    cp0ld.l (%a1)+,&LDR+CA1             # output[1] = h[1]
    cp0ld.l (%a1)+,&LDR+CA2             # output[2] = h[2]
    cp0ld.l (%a1)+,&LDR+CA3             # output[3] = h[3]
    cp0ld.l (%a1)+,&LDR+CA4             # output[4] = h[4]
    cp0ld.l (%a1)+,&LDR+CA5             # output[5] = h[5]
    cp0ld.l (%a1)+,&LDR+CA6             # output[6] = h[6]
    cp0ld.l (%a1)+,&LDR+CA7             # output[7] = h[7]
    lea     (-32,%a1),%a1               # reset a1 back to output[0]

    mov.l   (276,%sp),%d7               # number of blocks
    align   4
sha256_hash_n_L%0:
    lea     (sha256_k.w,%pc),%a3        # pointer to k[]
    lea     (12,%sp),%a2                # pointer to local array w[]
    mvs.w   &16,%d0                     # set loop counter (to keep text aligned)
sha256_hash_n_L%1:
    mov.l   (%a0)+,%d1                  # load m[i] into d1
    mov.l   %d1,(%a2)+                  # store m[i] into w[i]
    cp0ld.l %d1,&LDR+CAA                # w[i] into CAA
    cp0ld.l &ADRA+CA7                   # add h
    cp0ld.l &HASH+HF2T                  # add SIGMA1(e)
    cp0ld.l &HASH+HF2C                  # add Ch(e,f,g)
    cp0ld.l (%a3)+,&ADR+CAA             # add k[i]
    cp0ld.l &MVAR+CA8                   # t1
    cp0ld.l &HASH+HF2S                  # add SIGMA0(e)
    cp0ld.l &HASH+HF2M                  # add Maj(a,b,c)
    cp0ld.l &SHS2                       # shift registers
    
    subq.l  &1,%d0
    bne.w   sha256_hash_n_L%1           # (to keep text aligned)

    movq.l  &48,%d0                     # set loop counter
sha256_hash_n_L%2:
    cp0ld.l (-64,%a2),&LDR+CAA          # w[i-16]
    cp0ld.l (-60,%a2),&LDR+CA8          # w[i-15]
    cp0ld.l &HASH+HF2U                  # add Sigma2(w[i-15])
    cp0ld.l (-28,%a2),&ADR+CAA          # add w[i-7]
    cp0ld.l (-8,%a2),&LDR+CA8           # w[i-2]
    cp0ld.l &HASH+HF2V                  # add Sigma1(w[i-2])
    cp0st.l (%a2)+,&STR+CAA             # store w[i]
    cp0ld.l &ADRA+CA7                   # add h
    cp0ld.l &HASH+HF2T                  # add SIGMA1(e)
    cp0ld.l &HASH+HF2C                  # add Ch(e,f,g)
    cp0ld.l (%a3)+,&ADR+CAA             # add k[i]
    cp0ld.l &MVAR+CA8                   # t1
    cp0ld.l &HASH+HF2S                  # add SIGMA0(e)
    cp0ld.l &HASH+HF2M                  # add Maj(a,b,c)
    cp0ld.l &SHS2                       # shift registers

    subq.l  &1,%d0
    bne.b   sha256_hash_n_L%2

    cp0ld.l (%a1)+,&ADR+CA0             # add out[0] (= h[0])
    cp0ld.l (%a1)+,&ADR+CA1             # add out[1] (= h[1])
    cp0ld.l (%a1)+,&ADR+CA2             # add out[2] (= h[2])
    cp0ld.l (%a1)+,&ADR+CA3             # add out[3] (= h[3])
    cp0ld.l (%a1)+,&ADR+CA4             # add out[4] (= h[4])
    cp0ld.l (%a1)+,&ADR+CA5             # add out[5] (= h[5])
    cp0ld.l (%a1)+,&ADR+CA6             # add out[6] (= h[6])
    cp0ld.l (%a1)+,&ADR+CA7             # add out[7] (= h[7])

    cp0st.l -(%a1),&STR+CA7             # store out[7] (= h[7])
    cp0st.l -(%a1),&STR+CA6             # store out[6] (= h[6])
    cp0st.l -(%a1),&STR+CA5             # store out[5] (= h[5])
    cp0st.l -(%a1),&STR+CA4             # store out[4] (= h[4])
    cp0st.l -(%a1),&STR+CA3             # store out[3] (= h[3])
    cp0st.l -(%a1),&STR+CA2             # store out[2] (= h[2])
    cp0st.l -(%a1),&STR+CA1             # store out[1] (= h[1])
    cp0st.l -(%a1),&STR+CA0             # store out[0] (= h[0])

    subq.l  &1,%d7
    bne.w   sha256_hash_n_L%0

    movm.l  (%sp),&0x0c80               # restore d7/a2/a3
    lea     (268,%sp),%sp               # deallocate stack space
    rts


#*******************************************************************************
#*******************************************************************************
#
# SHA256: Updates SHA256 state variables for one or more input message blocks
#
#   arguments
#           *input      pointer to start of input message data
#           num_blks    number of 512-bit blocks to process
#           *output     pointer to 256-bit message digest output
#
#   NOTE    Input message and digest output blocks must not overlap
#
#   calling convention
#   void    cau_sha256_update (const unsigned int *input,
#                              const int          num_blks,
#                              unsigned int *output)

    global  _cau_sha256_update
    global  cau_sha256_update
    align   4
_cau_sha256_update:
cau_sha256_update:
    mov.l   (12,%sp),%a0                # get argument: *output
    lea     (sha256_init_h.w,%pc),%a1   # pointer to initial data

# copy initial data into hash output buffer (32 bytes = 8 longwords)
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+

    bra.w   cau_sha256_hash_n


#*******************************************************************************
#*******************************************************************************
#
# SHA256: Perform the hash for one input message block and generate the
#         message digest output
#
#   arguments
#           *input      pointer to start of input message data
#           *output     pointer to 256-bit message digest output
#
#   NOTE    Input message and digest output blocks must not overlap
#
#   calling convention
#   void    cau_sha256_hash (const unsigned int *input,
#                            unsigned int *output)

    global  _cau_sha256_hash
    global  cau_sha256_hash
    align   4
_cau_sha256_hash:
cau_sha256_hash:
    lea     (-264,%sp),%sp              # allocate stack space: 2 regs, int w[64] array

# the stack frame layout for this function is:
#   0(%sp)  saved %a2
#   4(%sp)  saved %a3
#   8(%sp)  local variable w[0] - array has 64 32-bit entries, 256 bytes total
# 264(%sp)  returnPC
# 268(%sp)  input argument 1 - pointer to message input[]
# 272(%sp)  input argument 2 - pointer to message digest output[]

    movm.l  &0x0c00,(%sp)               # save a2/a3
    lea     (sha256_k.w,%pc),%a3        # pointer to k[]
    mov.l   (268,%sp),%a0               # pointer to message input[]
    lea     (8,%sp),%a2                 # pointer to local array w[]
    mov.l   (272,%sp),%a1               # pointer to message digest output[]

# initialize the CAU data registers with the current contents of output[]
    cp0ld.l (%a1)+,&LDR+CA0             # output[0] = h[0]
    cp0ld.l (%a1)+,&LDR+CA1             # output[1] = h[1]
    cp0ld.l (%a1)+,&LDR+CA2             # output[2] = h[2]
    cp0ld.l (%a1)+,&LDR+CA3             # output[3] = h[3]
    cp0ld.l (%a1)+,&LDR+CA4             # output[4] = h[4]
    cp0ld.l (%a1)+,&LDR+CA5             # output[5] = h[5]
    cp0ld.l (%a1)+,&LDR+CA6             # output[6] = h[6]
    cp0ld.l (%a1)+,&LDR+CA7             # output[7] = h[7]
    lea     (-32,%a1),%a1               # reset a1 back to output[0]

    movq    &16,%d0                     # set loop counter
    align   4
sha256_hash_L%1:
    mov.l   (%a0)+,%d1                  # load m[i] into d1
    mov.l   %d1,(%a2)+                  # store m[i] into w[i]
    cp0ld.l %d1,&LDR+CAA                # w[i] into CAA
    cp0ld.l &ADRA+CA7                   # add h
    cp0ld.l &HASH+HF2T                  # add SIGMA1(e)
    cp0ld.l &HASH+HF2C                  # add Ch(e,f,g)
    cp0ld.l (%a3)+,&ADR+CAA             # add k[i]
    cp0ld.l &MVAR+CA8                   # t1
    cp0ld.l &HASH+HF2S                  # add SIGMA0(e)
    cp0ld.l &HASH+HF2M                  # add Maj(a,b,c)
    cp0ld.l &SHS2                       # shift registers
    
    subq.l  &1,%d0
    bne.w   sha256_hash_L%1             # (to keep text aligned)

    movq.l  &48,%d0                     # set loop counter
sha256_hash_L%2:
    cp0ld.l (-64,%a2),&LDR+CAA          # w[i-16]
    cp0ld.l (-60,%a2),&LDR+CA8          # w[i-15]
    cp0ld.l &HASH+HF2U                  # add Sigma2(w[i-15])
    cp0ld.l (-28,%a2),&ADR+CAA          # add w[i-7]
    cp0ld.l (-8,%a2),&LDR+CA8           # w[i-2]
    cp0ld.l &HASH+HF2V                  # add Sigma1(w[i-2])
    cp0st.l (%a2)+,&STR+CAA             # store w[i]
    cp0ld.l &ADRA+CA7                   # add h
    cp0ld.l &HASH+HF2T                  # add SIGMA1(e)
    cp0ld.l &HASH+HF2C                  # add Ch(e,f,g)
    cp0ld.l (%a3)+,&ADR+CAA             # add k[i]
    cp0ld.l &MVAR+CA8                   # t1
    cp0ld.l &HASH+HF2S                  # add SIGMA0(e)
    cp0ld.l &HASH+HF2M                  # add Maj(a,b,c)
    cp0ld.l &SHS2                       # shift registers

    subq.l  &1,%d0
    bne.b   sha256_hash_L%2

    cp0ld.l (%a1)+,&ADR+CA0             # add out[0] (= h[0])
    cp0ld.l (%a1)+,&ADR+CA1             # add out[1] (= h[1])
    cp0ld.l (%a1)+,&ADR+CA2             # add out[2] (= h[2])
    cp0ld.l (%a1)+,&ADR+CA3             # add out[3] (= h[3])
    cp0ld.l (%a1)+,&ADR+CA4             # add out[4] (= h[4])
    cp0ld.l (%a1)+,&ADR+CA5             # add out[5] (= h[5])
    cp0ld.l (%a1)+,&ADR+CA6             # add out[6] (= h[6])
    cp0ld.l (%a1)+,&ADR+CA7             # add out[7] (= h[7])

    cp0st.l -(%a1),&STR+CA7             # store out[7] (= h[7])
    cp0st.l -(%a1),&STR+CA6             # store out[6] (= h[6])
    cp0st.l -(%a1),&STR+CA5             # store out[5] (= h[5])
    cp0st.l -(%a1),&STR+CA4             # store out[4] (= h[4])
    cp0st.l -(%a1),&STR+CA3             # store out[3] (= h[3])
    cp0st.l -(%a1),&STR+CA2             # store out[2] (= h[2])
    cp0st.l -(%a1),&STR+CA1             # store out[1] (= h[1])
    cp0st.l -(%a1),&STR+CA0             # store out[0] (= h[0])

    movm.l  (%sp),&0x0c00               # restore a2/a3
    lea     (264,%sp),%sp               # deallocate stack space
    rts


#*******************************************************************************
#*******************************************************************************
#
# CAU Constant Data

    align   16
sha256_init_h:
    long    0x6a09e667                  # initialize h[0] = output[0]
    long    0xbb67ae85                  # initialize h[1] = output[1]
    long    0x3c6ef372                  # initialize h[2] = output[2]
    long    0xa54ff53a                  # initialize h[3] = output[3]
    long    0x510e527f                  # initialize h[4] = output[4]
    long    0x9b05688c                  # initialize h[5] = output[5]
    long    0x1f83d9ab                  # initialize h[6] = output[6]
    long    0x5be0cd19                  # initialize h[7] = output[7]

sha256_k:
    long    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5
    long    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5
    long    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3
    long    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174
    long    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc
    long    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da
    long    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7
    long    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967
    long    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13
    long    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85
    long    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3
    long    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070
    long    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5
    long    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3
    long    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208
    long    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
