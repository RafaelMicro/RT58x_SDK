/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */
/* PURPOSE: Configuration file: configuration- and platform-specific definitions,
constants etc.
*/

#ifndef ZB_CONFIG_H
#define ZB_CONFIG_H 1


#ifndef ZB_BUF_Q_SIZE
/* @note To prevent deadlocks ZB_BUF_Q_SIZE must be < ZB_IOBUF_POOL_SIZE/2 */
/**
   Size of queue for wait for free packet buffer
*/
#define ZB_BUF_Q_SIZE 8U
#endif /*ZB_BUF_Q_SIZE*/
#define ZB_TIMER_32

/**
   Size, in bytes, of the packet buffer

   @internal Be sure keep it multiple of 4 to exclude alignment problems at ARM
*/
#ifndef ZB_IO_BUF_SIZE
/* Set the ZBOSS buffer size according to the set of enabled MAC features. */
#ifdef ZB_MAC_SECURITY
#define ZB_IO_BUF_SIZE 164U
#else /* ZB_MAC_SECURITY */
#define ZB_IO_BUF_SIZE 152U
#endif /* ZB_MAC_SECURITY */
#endif  /* ZB_IO_BUF_SIZE */




/** @cond DOXYGEN_INTERNAL_DOC */
/*
  If enabled, real int24 and uint48 types will work. Else, int24 and uint48 is mapped to
  int32 and uint32.
  See include/zb_uint48.h.
*/
/** Enable support for int24 and uint48 data types */
#ifndef ZB_NO_UINT24_48_SUPPORT
#define ZB_UINT24_48_SUPPORT
#endif /* !ZB_NO_UINT24_48_SUPPORT */
/** @endcond */ /* DOXYGEN_INTERNAL_DOC */







#endif /* ZB_CONFIG_H */
