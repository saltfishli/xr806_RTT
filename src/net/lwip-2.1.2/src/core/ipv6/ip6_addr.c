/**
 * @file
 *
 * IPv6 addresses.
 */

/*
 * Copyright (c) 2010 Inico Technologies Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Ivan Delamer <delamer@inicotech.com>
 *
 * Functions for handling IPv6 addresses.
 *
 * Please coordinate changes and requests with Ivan Delamer
 * <delamer@inicotech.com>
 */

#include "lwip/opt.h"

#if LWIP_IPV6  /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip_addr.h"
#include "lwip/def.h"

#include <string.h>

#if LWIP_IPV4
#include "lwip/ip4_addr.h" /* for ip6addr_aton to handle IPv4-mapped addresses */
#endif /* LWIP_IPV4 */

/* used by IP6_ADDR_ANY(6) in ip6_addr.h */
const ip_addr_t ip6_addr_any = IPADDR6_INIT(0ul, 0ul, 0ul, 0ul);

#define lwip_xchar(i)        ((char)((i) < 10 ? '0' + (i) : 'A' + (i) - 10))

/**
 * Check whether "cp" is a valid ascii representation
 * of an IPv6 address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 *
 * @param cp IPv6 address in ascii representation (e.g. "FF01::1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int ip6addr_aton(const char *cp, ip6_addr_t *addr)
{
    const int ipv6_blocks = 8;
    u16_t current_block_index = 0;
    u16_t current_block_value = 0;
    u16_t addr16[ipv6_blocks];
    u16_t *a16 = (u16_t *)addr->addr;
    int squash_pos = ipv6_blocks;
    int i;
    const char *sc = cp;
    const char *ss = cp-1;

    for (; ; sc++) {
        if (current_block_index >= ipv6_blocks) {
            return 0; // address too long
        }
        if (*sc == 0) {
            if (sc - ss == 1) {
                if (squash_pos != current_block_index) {
                    return 0; // empty address or address ends with a single ':'
                } // else address ends with one valid "::"
            } else {
                addr16[current_block_index++] = current_block_value;
            }
            break;
        } else if (*sc == ':') {
            if (sc - ss == 1) {
                if (sc != cp || sc[1] != ':') {
                    return 0; // address begins with a single ':' or contains ":::"
                } // else address begins with one valid "::"
            } else {
                addr16[current_block_index++] = current_block_value;
            }
            if (sc[1] == ':') {
                if (squash_pos != ipv6_blocks) {
                    return 0; // more than one "::"
                }
                squash_pos = current_block_index;
                sc++;
            }
            ss = sc; // ss points to the recent ':' position
            current_block_value = 0;
        } else if (lwip_isxdigit(*sc) && (sc - ss) < 5) { // 4 hex-digits at most
            current_block_value = (current_block_value << 4) +
                (*sc | ('a' - 'A')) - '0' - ('a' - '9' - 1) * (*sc >= 'A');
#if LWIP_IPV4
        } else if (*sc == '.' && current_block_index < ipv6_blocks - 1) {
            ip4_addr_t ip4;
            int ret = ip4addr_aton(ss+1, &ip4);
            if (!ret) {
                return 0;
            }
            ip4.addr = lwip_ntohl(ip4.addr);
            addr16[current_block_index++] = (u16_t)(ip4.addr >> 16);
            addr16[current_block_index++] = (u16_t)(ip4.addr);
            break;
#endif /* LWIP_IPV4 */
        } else {
            return 0; // unexpected char or too many digits
        }
    }

    if (squash_pos == ipv6_blocks && current_block_index != ipv6_blocks) {
        return 0; // address too short
    }
    if (squash_pos != ipv6_blocks && current_block_index == ipv6_blocks) {
        return 0; // unexpected "::" in address
    }

    for (i = 0; i < squash_pos; ++i) {
        a16[i] = lwip_htons(addr16[i]);
    }
    for (; i < ipv6_blocks - current_block_index + squash_pos; ++i) {
        a16[i] = 0;
    }
    for (; i < ipv6_blocks; ++i) {
        a16[i] = lwip_htons(addr16[i - ipv6_blocks + current_block_index]);
    }

    return 1;
}

/**
 * Convert numeric IPv6 address into ASCII representation.
 * returns ptr to static buffer; not reentrant!
 *
 * @param addr ip6 address in network order to convert
 * @return pointer to a global static (!) buffer that holds the ASCII
 *         representation of addr
 */
char *
ip6addr_ntoa(const ip6_addr_t *addr)
{
  static char str[40];
  return ip6addr_ntoa_r(addr, str, 40);
}

/**
 * Same as ipaddr_ntoa, but reentrant since a user-supplied buffer is used.
 *
 * @param addr ip6 address in network order to convert
 * @param buf target buffer where the string is stored
 * @param buflen length of buf
 * @return either pointer to buf which now holds the ASCII
 *         representation of addr or NULL if buf was too small
 */
char *
ip6addr_ntoa_r(const ip6_addr_t *addr, char *buf, int buflen)
{
  u32_t current_block_index, current_block_value, next_block_value;
  s32_t i;
  u8_t zero_flag, empty_block_flag;

#if LWIP_IPV4
  if (ip6_addr_isipv4mappedipv6(addr)) {
    /* This is an IPv4 mapped address */
    ip4_addr_t addr4;
    char *ret;
#define IP4MAPPED_HEADER "::FFFF:"
    char *buf_ip4 = buf + sizeof(IP4MAPPED_HEADER) - 1;
    int buflen_ip4 = buflen - sizeof(IP4MAPPED_HEADER) + 1;
    if (buflen < (int)sizeof(IP4MAPPED_HEADER)) {
      return NULL;
    }
    memcpy(buf, IP4MAPPED_HEADER, sizeof(IP4MAPPED_HEADER));
    addr4.addr = addr->addr[3];
    ret = ip4addr_ntoa_r(&addr4, buf_ip4, buflen_ip4);
    if (ret != buf_ip4) {
      return NULL;
    }
    return buf;
  }
#endif /* LWIP_IPV4 */
  i = 0;
  empty_block_flag = 0; /* used to indicate a zero chain for "::' */

  for (current_block_index = 0; current_block_index < 8; current_block_index++) {
    /* get the current 16-bit block */
    current_block_value = lwip_htonl(addr->addr[current_block_index >> 1]);
    if ((current_block_index & 0x1) == 0) {
      current_block_value = current_block_value >> 16;
    }
    current_block_value &= 0xffff;

    /* Check for empty block. */
    if (current_block_value == 0) {
      if (current_block_index == 7 && empty_block_flag == 1) {
        /* special case, we must render a ':' for the last block. */
        buf[i++] = ':';
        if (i >= buflen) {
          return NULL;
        }
        break;
      }
      if (empty_block_flag == 0) {
        /* generate empty block "::", but only if more than one contiguous zero block,
         * according to current formatting suggestions RFC 5952. */
        next_block_value = lwip_htonl(addr->addr[(current_block_index + 1) >> 1]);
        if ((current_block_index & 0x1) == 0x01) {
            next_block_value = next_block_value >> 16;
        }
        next_block_value &= 0xffff;
        if (next_block_value == 0) {
          empty_block_flag = 1;
          buf[i++] = ':';
          if (i >= buflen) {
            return NULL;
          }
          continue; /* move on to next block. */
        }
      } else if (empty_block_flag == 1) {
        /* move on to next block. */
        continue;
      }
    } else if (empty_block_flag == 1) {
      /* Set this flag value so we don't produce multiple empty blocks. */
      empty_block_flag = 2;
    }

    if (current_block_index > 0) {
      buf[i++] = ':';
      if (i >= buflen) {
        return NULL;
      }
    }

    if ((current_block_value & 0xf000) == 0) {
      zero_flag = 1;
    } else {
      buf[i++] = lwip_xchar(((current_block_value & 0xf000) >> 12));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf00) == 0) && (zero_flag)) {
      /* do nothing */
    } else {
      buf[i++] = lwip_xchar(((current_block_value & 0xf00) >> 8));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf0) == 0) && (zero_flag)) {
      /* do nothing */
    }
    else {
      buf[i++] = lwip_xchar(((current_block_value & 0xf0) >> 4));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    buf[i++] = lwip_xchar((current_block_value & 0xf));
    if (i >= buflen) {
      return NULL;
    }
  }

  buf[i] = 0;

  return buf;
}

#endif /* LWIP_IPV6 */
