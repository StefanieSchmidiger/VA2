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
# DES: Checks key parity
#   arguments
#           *key        pointer to 64-bit DES key with parity bits
#
#   return
#           0           no error
#           -1          parity error
#
#   calling convention
#   int     cau_des_chk_parity (const unsigned char *key)

    global  _cau_des_chk_parity
    global  cau_des_chk_parity
    align   4
_cau_des_chk_parity:
cau_des_chk_parity:
    mov.l   (4,%sp),%a0                 # get argument: *key

# load the 64-bit key into the CAU's CA0/CA1 registers
    cp0ld.l (%a0)+,&LDR+CA0             # load key[i]   -> CA0
    cp0ld.l (%a0),&LDR+CA1              # load key[i+1] -> CA1

#  perform the key schedule and check the parity bits
    cp0ld.l &DESK+CP                    # key setup + parity check

# the CAUSR[DPE] reflects the DES key parity check
    cp0st.l %d0,&STR+CASR               # store CAUSR -> d0
    btst    &1,%d0                      # test CAUSR[DPE]
    sne     %d0                         # if DPE, then d0 = 0xff, else d0 = 0x00
    extb.l  %d0                         # return 32-bit int
    rts


#*******************************************************************************
#*******************************************************************************
#
# DES: Encrypts a single 8-byte block
#   arguments
#           *in         pointer to 8-byte block of input plaintext
#           *key        pointer to 64-bit DES key with parity bits
#           *out        pointer to 8-byte block of output ciphertext
#
#   NOTE    Input and output blocks may overlap
#
#   calling convention
#   void    cau_des_encrypt (constr unsigned char *in,
#                            constr unsigned char *key,
#                            unsigned char        *out)

    global  _cau_des_encrypt
    global  cau_des_encrypt
    align   4
_cau_des_encrypt:
cau_des_encrypt:
    mov.l   (4,%sp),%a0                 # get 1st argument: *in
    mov.l   (8,%sp),%a1                 # get 2nd argument: *key

# load the 64-bit plaintext input block into CAU's CA2/CA3 registers
    cp0ld.l (%a0)+,&LDR+CA2             # load plain[i]   -> CA2
    cp0ld.l (%a0),&LDR+CA3              # load plain[i+1] -> CA3

# load the 64-bit key into the CAU's CA0/CA1 registers
    cp0ld.l (%a1)+,&LDR+CA0             # load key[i]     -> CA0
    cp0ld.l (%a1),&LDR+CA1              # load key[i+1]   -> CA1
    cp0ld.l &DESK                       # key setup

# perform the DES round operations
    cp0ld.l &DESR+IP+KSL1
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL1
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL2
    cp0ld.l &DESR+KSL1
    cp0ld.l &DESR+FP

# store the 64-bit ciphertext output block into the CPU's d0/d1 registers
    mov.l   (12,%sp),%a1                # get 3rd argument: *out
    cp0st.l %d0,&STR+CA2                # get 1st 4 bytes of result -> d0
    cp0st.l %d1,&STR+CA3                # get 2nd 4 bytes of result -> d1

# store the 64-bit ciphertext output block into memory
    mov.l   %d0,(%a1)+                  # store 1st 4 bytes of output ciphertext
    mov.l   %d1,(%a1)                   # store 2nd 4 bytes of output ciphertext
    rts


#*******************************************************************************
#*******************************************************************************
#
# DES: Decrypts a single 8-byte block
#   arguments
#           *in         pointer to 8-byte block of input ciphertext
#           *key        pointer to 64-bit DES key with parity bits
#           *out        pointer to 8-byte block of output plaintext
#
#   NOTE    Input and output blocks may overlap
#
#   calling convention
#   void    cau_des_decrypt (constr unsigned char *in,
#                            constr unsigned char *key,
#                            unsigned char        *out)

    global  _cau_des_decrypt
    global  cau_des_decrypt
    align   4
cau_des_decrypt:
_cau_des_decrypt:
    mov.l   (4,%sp),%a0                 # get 1st argument: *in
    mov.l   (8,%sp),%a1                 # get 2nd argument: *key

# load the 64-bit ciphertext input block into CAU's CA2/CA3 registers
    cp0ld.l (%a0)+,&LDR+CA2             # load cipher[i]   -> CA2
    cp0ld.l (%a0),&LDR+CA3              # load cipher[i+1] -> CA3

# load the 64-bit key into the CAU's CA0/CA1 registers
    cp0ld.l (%a1)+,&LDR+CA0             # load key[i]     -> CA0
    cp0ld.l (%a1),&LDR+CA1              # load key[i+1]   -> CA1
    cp0ld.l &DESK+DC                    # key setup

# perform the DES round operations
    cp0ld.l &DESR+IP+KSR1
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR1
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR2
    cp0ld.l &DESR+KSR1
    cp0ld.l &DESR+FP

# store the 64-bit plaintext output block into the CPU's d0/d1 registers
    mov.l   (12,%sp),%a1                # get 3rd argument: *out
    cp0st.l %d0,&STR+CA2                # get 1st 4 bytes of result -> d0
    cp0st.l %d1,&STR+CA3                # get 2nd 4 bytes of result -> d1

# store the 64-bit plaintext output block into memory
    mov.l   %d0,(%a1)+                  # store 1st 4 bytes of output plaintext
    mov.l   %d1,(%a1)                   # store 2nd 4 bytes of output plaintext
    rts
