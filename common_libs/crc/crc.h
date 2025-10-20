/*==============================================================================
File    crc.h

Author  Daniel Zorychta

Brief   CRC library.

        MIT License

        Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

        Permission is hereby granted, free of charge, to any person obtaining
        a copy of this software and associated documentation files (the
        "Software"), to deal in the Software without restriction, including
        without limitation the rights to use, copy, modify, merge, publish,
        distribute, sublicense, and/or sell copies of the Software, and to
        permit persons to whom the Software is furnished to do so, subject to
        the following conditions:

        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
        OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
        THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
        OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
        ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
        OTHER DEALINGS IN THE SOFTWARE.

==============================================================================*/

/**
@defgroup _CRC_H_ _CRC_H_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/
extern const uint8_t crc8_1wire_init;
extern const uint16_t crc16_ccitt_init;
extern const uint32_t crc32_ethernet_init;

/*==============================================================================
  Exported functions
==============================================================================*/

//==============================================================================
/**
 * @brief  Calculate 1-wire CRC8.
 *
 * @param  buf          input buffer
 * @param  len          buffer size
 * @param  init         init value or last calculated CRC
 *
 * @return CRC8.
 */
//==============================================================================
extern uint8_t crc8_update_1wire(const void *buf, size_t len, uint8_t init);

//==============================================================================
/**
 * @brief  Calculate CCITT CRC16.
 *
 * @param  buf          buffer to calculate
 * @param  len          buffer length
 * @param  crc_in       init value or last calculated CRC
 *
 * @return CRC16.
 */
//==============================================================================
extern uint16_t crc16_udpate_ccitt(const void *buf, uint16_t len, uint16_t crc_in);

//==============================================================================
/**
 * @brief  Calculate CRC32.
 *
 * @param  buf          buffer
 * @param  buflen       buffer length
 * @param  init         initial value
 *
 * @return CRC32 result.
 */
//==============================================================================
extern uint32_t crc32_update(const void *buf, uint32_t buflen, uint32_t init);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
