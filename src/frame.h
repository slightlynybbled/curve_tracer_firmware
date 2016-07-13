#ifndef _FRAME_H
#define _FRAME_H

#include <stdint.h>
#include <stdbool.h>
#include "dispatch_config.h"

/**
 * Use to initialize a frame (usually at the start of a message)
 */
void FRM_init(void);

/**
 * Use to send data as part of a frame.  The frame must have been
 * previously initialized using FRM_init().  All data that is sent
 * is considered part of the same message until FRM_finish() is
 * executed.
 * 
 * @param data a single byte of data
 */
void FRM_push(uint8_t data);

/**
 * Use to finish a frame
 */
void FRM_finish(void);

/**
 * Use to read unframed data from the receive buffer
 * 
 * @param data a byte-aligned array pointer to which the
 * unframed data will be copied
 * @return length the length of the array
 */
uint16_t FRM_pull(uint8_t* data);

/**
 * Assigns the 'readable' function from the hardware
 * access library.
 *
 * @param functPtr a function pointer to a function which tells how
 * many bytes can be read from the channel input
 */
void FRM_assignChannelReadable(uint16_t (*functPtr)());

/**
 * Assigns the 'writeable' function from the hardware
 * access library.
 *
 * @param functPtr a function pointer to a function which tells how
 * many bytes can be written to the channel output
 */
void FRM_assignChannelWriteable(uint16_t (*functPtr)());

/**
 * Assigns the 'read' function from the hardware access library.
 *
 * @param functPtr a function pointer to a function which will read <length>
 * bytes to the channel output from <data>
 */
void FRM_assignChannelRead(void (*functPtr)(uint8_t* data, uint16_t length));

/**
 * Assigns the 'write' function from the hardware access library.
 *
 * @param functPtr a function pointer to a function which will read <length>
 * bytes from the channel input and into <data>
 */
void FRM_assignChannelWrite(void (*functPtr)(uint8_t* data, uint16_t length));

#endif