#ifndef _DISPATCH_H
#define _DISPATCH_H

#include <stdint.h>
#include "dispatch_config.h"

/**
 * Initializes the PUB library elements, must be called before
 * any other PUB functions
 */
void DIS_init(void);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic, length,
 * and format specifiers
 * 
 * @param ... one or more pointers to the data arrays
 */
void DIS_publish(const char* topic, ...);

/**
 * Publish a string to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param str string pointer
 */
void DIS_publish_str(const char* topic, char* str);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data pointer to the first element in the array
 */
void DIS_publish_u8(const char* topic, uint8_t* data);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data pointer to the first element in the array
 */
void DIS_publish_s8(const char* topic, int8_t* data);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data0 pointer to the first element in the first array
 * @param data1 pointer to the first element in the second array
 */
void DIS_publish_2u8(const char* topic, uint8_t* data0, uint8_t* data1);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data0 pointer to the first element in the first array
 * @param data1 pointer to the first element in the second array
 */
void DIS_publish_2s8(const char* topic, int8_t* data0, int8_t* data1);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data pointer to the first element in the array
 */
void DIS_publish_u16(const char* topic, uint16_t* data);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data pointer to the first element in the array
 */
void DIS_publish_s16(const char* topic, int16_t* data);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data0 pointer to the first element in the first array
 * @param data1 pointer to the first element in the second array
 */
void DIS_publish_2u16(const char* topic, uint16_t* data0, uint16_t* data1);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data0 pointer to the first element in the first array
 * @param data1 pointer to the first element in the second array
 */
void DIS_publish_2s16(const char* topic, int16_t* data0, int16_t* data1);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data pointer to the first element in the array
 */
void DIS_publish_u32(const char* topic, uint32_t* data);

/**
 * Publish data to a particular topic
 * 
 * @param topic a text string that contains the topic and length
 * 
 * @param data pointer to the first element in the array
 */
void DIS_publish_s32(const char* topic, int32_t* data);

/**
 * Subscribe to a particular topic
 * 
 * @param topic a text string that contains the topic only
 * 
 * @param functPtr a function pointer to the function that should
 * be executed when the particular topic is received.
 */
void DIS_subscribe(const char* topic, void (*functPtr)());

/**
 * Unsubscribe the function from all topics
 * 
 * @param functPtr a function pointer to the function that should
 * be removed from all topics
 */
void DIS_unsubscribe(void (*functPtr)());

/**
 * This function must be called periodically in order to properly
 * call the subscribed function callbacks.  The minimum rate that 
 * the function should be called is dependant on expected topic
 * reception frequency.  This function should be called at a rate
 * at which a particular topic cannot be received twice.  For
 * instance, if your max-rate topic is sent every 100ms, then this
 * function should be called - at minimum - every 100ms.  You should
 * strive for twice this frequency, or 50ms, where possible.
 */
void DIS_process(void);

/**
 * The subscribing function(s) will use this function in order to
 * extract the received data from the publish library.  Note that
 * the subscribing functions must already be aware of the data type
 * that it is expecting.
 * 
 * @param element the element number
 * @param destArray the destination array for the data to be copied
 * into
 * @return the length of the received data
 */
uint16_t DIS_getElements(uint16_t element, void* destArray);

/** 
 * Use this function to assign the 'readable' function.  The 
 * 'readable' function must return a uint16_t and takes a
 * void.
 * 
 * @param *functPtr a function pointer for a function that will
 * return the number of bytes waiting to be read from the
 * communication channel */
void DIS_assignChannelReadable(uint16_t (*functPtr)());

/** 
 * Use this function to assign the 'writeable' funciton.  The
 * 'writeable' function must return a uint16_t and take a void.
 * 
 * @param *functPtr a function pointer for a function that will
 * return the number of bytes that are able to be written to
 * the communication channel */
void DIS_assignChannelWriteable(uint16_t (*functPtr)());

/** 
 * Use this function to assign the 'read' function.  The 'read'
 * function must take a data pointer and a length.  This allows
 * the function to read <length> amount of data from the incoming
 * channel buffer into the <data> array.
 * 
 * @param *functPtr a function pointer for a function that will
 * read <length> data into the array <data> */
void DIS_assignChannelRead(void (*functPtr)(uint8_t* data, uint16_t length));

/** 
 * Use this function to assign the 'write' function.  The 'write'
 * function must take a data pointer and a length.  This allows
 * the function to write <length> amount of data to the outgoing
 * channel buffer from the <data> array.
 * 
 * @param *functPtr a function pointer for a function that will
 * write <length> data from <data> to the outgoing buffer */
void DIS_assignChannelWrite(void (*functPtr)(uint8_t* data, uint16_t length));

#endif
