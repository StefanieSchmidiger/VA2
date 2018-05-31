/*
 * Golay.h
 *
 *  Created on: 20.03.2018
 *      Author: Stefanie
 */

#ifndef HEADERS_GOLAY_H_
#define HEADERS_GOLAY_H_

// -*- Mode: C; c-basic-offset: 8; -*-
//
// Copyright (c) 2012 Andrew Tridgell, All Rights Reserved
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  o Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  o Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//

///
/// @file	golay23.h
///
/// golay 23/12 error correction encoding and decoding
///
#include <stdint.h>
#include "Golay23.h"


/*!
* \fn void golay_encode(uint8_t n, uint8_t* in, uint8_t* out)
* \brief Encodes n bytes of original data into n*2 bytes of encoded data
* \param n: number of bytes to encode, must be multiple of 3
* \param in: pointer to n bytes that will be encoded
* \param out: pointer to memory location where encoded data will be stored
*/
void golay_encode(uint8_t n, uint8_t* in, uint8_t* out);


/*!
* \fn uint8_t golay_decode(uint8_t n, uint8_t* in, uint8_t* out)
* \brief Decodes n bytes of coded data into n/2 bytes of original data
* \param n: number of bytes to decode, must be multiple of 6
* \param in: pointer to n bytes that will be decoded
* \param out: pointer to memory location where decoded data will be stored
* \return number of 12bit words that required correction
*/
uint8_t golay_decode(uint8_t n, uint8_t* in, uint8_t* out);


#endif /* HEADERS_GOLAY_H_ */
