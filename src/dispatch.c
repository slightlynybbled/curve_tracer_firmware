#include "dispatch.h"
#include "frame.h"

#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum formatspecifier{
    eNONE = 0,
    eSTRING = 1,
    eU8 = 2,
    eS8 = 3,
    eU16 = 4,
    eS16 = 5,
    eU32 = 6,
    eS32 = 7
}FormatSpecifier;

typedef struct{
    uint8_t dimensions;
    uint16_t length;
    uint32_t length8bit;
    FormatSpecifier formatSpecifiers[MAX_NUM_OF_FORMAT_SPECIFIERS];
    
    uint8_t data[MAX_RECEIVE_MESSAGE_LEN];
}Message;

typedef struct {
    char topic[MAX_TOPIC_STR_LEN];
	void (*subFunctPtr)();
}Subscription;

/********** global variable declarations **********/
static Message rxMsg;
static Subscription sub[MAX_NUM_OF_SUBSCRIPTIONS];

/********** local function declarations **********/
uint16_t getCurrentRxPointerIndex(uint8_t element);
uint16_t parseTopicString(const char* topic, uint8_t dimensions);

/********** function implementations **********/
void DIS_init(void){
    uint16_t i = 0;
    
    /* clear the subscriptions */
    for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
        sub[i].subFunctPtr = 0;
        sub[i].topic[0] = 0;    // terminate the string
    }
}

void DIS_publish(const char* topic, ...){
    va_list arguments;
    va_start(arguments, topic);

    uint8_t dimensions = 0;
    uint8_t length = 0;
    FormatSpecifier formatSpecifiers[MAX_NUM_OF_FORMAT_SPECIFIERS];
    
    uint16_t i;
    for(i = 0; i < MAX_NUM_OF_FORMAT_SPECIFIERS; i++){
        formatSpecifiers[i] = eNONE;
    }
    
    /* get the num of args by counting the commas */
    uint8_t commaCount = 0;
    uint16_t len = strlen(topic);
    i = 0;
    while(i < len){
        if(topic[i] == ',')
            commaCount++;
        i++;
    }
    
    FRM_init();
    
    /* go through the first argument and extract the topic */
    uint16_t strIndex = 0;
    while((strIndex < len) && (topic[strIndex] != ',') && (topic[strIndex] != ':')){
        FRM_push(topic[strIndex]);  // send the topic string
        strIndex++;
    }
    FRM_push(0);    // send the \0 string terminator
    
    dimensions = commaCount;
    if(dimensions == 0){
        /* if the dimension == 0, then this is a string,
         *  simply transmit the string */
        FRM_push(1);
        
        char* data = va_arg(arguments, char*);
        
        /* send the length */
        length = strlen(data);
        FRM_push((uint8_t)(length & 0x00ff));
        FRM_push((uint8_t)((length & 0xff00) >> 8));
        
        /* send the format specifier for a string */
        FRM_push(eSTRING);
        
        /* finally, send the string */
        for(i = 0; i < length; i++){
            FRM_push(data[i]);
        }
    }else{
        /* if the code gets here, then there is definitely not a string
         * being transmitted but one or more numeric values */
        FRM_push(dimensions);

        /* determine if there is more of the string left to process and, if
         * there is, then process the index */
        if(strIndex < len){
            /* check for the ':' character to know if this 
             * is array or single-point processing */
            if(topic[strIndex] == ':'){
                strIndex++;

                /* unlimited spaces after colons */
                while(topic[strIndex] == ' '){
                    strIndex++;
                }
                
                /* find he first number to the colon command or
                 * the end of the string */
                char strNum0[8] = {0};
                i = 0;
                while((topic[strIndex] != ',') && (topic[strIndex] != 0)){
                    strNum0[i] = topic[strIndex];
                    i++;
                    strIndex++;
                }
                /* convert the ASCII number to an 
                 * integer and save it in arrIndex0 */
                length = (uint16_t)atol(strNum0);
            }
        }

        /* place a minimum on the arrIndex */
        if(length < 1)
            length = 1;
        
        FRM_push((uint8_t)(length & 0x00ff));
        FRM_push((uint8_t)((length & 0xff00) >> 8));

        /* check the format specifiers */
        i = 0;
        while((strIndex < len) && (i < MAX_NUM_OF_FORMAT_SPECIFIERS)){
            if(topic[strIndex] == ','){
                strIndex++;
                
                /* unlimited spaces after commas */
                while(topic[strIndex] == ' '){
                    strIndex++;
                }
                
                if(topic[strIndex] == 'u'){
                    strIndex++;
                    if(topic[strIndex] == '8'){
                        formatSpecifiers[i] = eU8;
                    }else if(topic[strIndex] == '1'){
                        /* if the first digit is '1', then the next digit must
                         * be 6, so there is no need to check for it */
                        formatSpecifiers[i] = eU16;
                        strIndex++;
                    }else if(topic[strIndex] == '3'){
                        /* if the first digit is '3', then the next digit must
                         * be 2, so there is no need to check for it */
                        formatSpecifiers[i] = eU32;
                        strIndex++;
                    }
                }else if(topic[strIndex] == 's'){
                    strIndex++;
                    if(topic[strIndex] == '8'){
                        formatSpecifiers[i] = eS8;
                    }else if(topic[strIndex] == '1'){
                        /* if the first digit is '1', then the next digit must
                         * be 6, so there is no need to check for it */
                        formatSpecifiers[i] = eS16;
                        strIndex++;
                    }else if(topic[strIndex] == '3'){
                        /* if the first digit is '3', then the next digit must
                         * be 2, so there is no need to check for it */
                        formatSpecifiers[i] = eS32;
                        strIndex++;
                    }else if(topic[strIndex] == 't'){
                        /* this is the case which calls for a string to be sent */
                        formatSpecifiers[i] = eSTRING;
                        strIndex++;
                    }
                }
                strIndex++;
            }else{
                /* if a comma isn't here, then abort the format specifier */
                break;
            }

            i++;
        }
        
        /* append all format specifiers */
        uint8_t fsArray[(MAX_NUM_OF_FORMAT_SPECIFIERS >> 1) + 1] = {0};
        uint8_t fsArrayIndex = 0;
        i = 0;
        while(i < dimensions){
            if((i & 1) == 0){
                fsArray[fsArrayIndex] = formatSpecifiers[i] & 0x0f;
            }else{
                fsArray[fsArrayIndex] |= ((formatSpecifiers[i] & 0x0f) << 4);
                fsArrayIndex++;
            }

            i++;
        }

        uint16_t fsArrayLength = ((i + 1) >> 1);
        for(i = 0; i < fsArrayLength; i++){
            FRM_push(fsArray[i]);
        }

        /* at this point:
         *     1. topic stored in topic[]
         *     2. array length is in msg.length
         *     3. format specifiers are in msg.formatSpecifiers[] array */
        i = 0;
        do{
            switch(formatSpecifiers[i]){
                case eU8:
                {
                    uint8_t* data = va_arg(arguments, uint8_t*);
                    
                    uint8_t j = 0;
                    while(j < length){
                        FRM_push(data[j]);
                        j++;
                    }
                    
                    break;
                }

                case eS8:
                {
                    int8_t* data = va_arg(arguments, int8_t*);
                    
                    uint8_t j = 0;
                    while(j < length){
                        FRM_push((uint8_t)data[j]);
                        j++;
                    }
                                        
                    break;
                }

                case eU16:
                {
                    uint16_t* data = va_arg(arguments, uint16_t*);
                    
                    uint8_t j = 0;
                    while(j < length){
                        FRM_push((uint8_t)(data[j] & 0x00ff));
                        FRM_push((uint8_t)((data[j] & 0xff00) >> 8));
                        j++;
                    }
                    
                    break;
                }

                case eS16:
                {
                    int16_t* data = va_arg(arguments, int16_t*);
                    
                    uint8_t j = 0;
                    while(j < length){
                        FRM_push((uint8_t)(data[j] & 0x00ff));
                        FRM_push((uint8_t)((data[j] & 0xff00) >> 8));
                        j++;
                    }
                    
                    break;
                }

                case eU32:
                {
                    uint32_t* data = va_arg(arguments, uint32_t*);
                    
                    uint8_t j = 0;
                    while(j < length){
                        FRM_push((uint8_t)(data[j] & 0x000000ff));
                        FRM_push((uint8_t)((data[j] & 0x0000ff00) >> 8));
                        FRM_push((uint8_t)((data[j] & 0x00ff0000) >> 16));
                        FRM_push((uint8_t)((data[j] & 0xff000000) >> 24));
                        j++;
                    }
                    
                    break;
                }

                case eS32:
                {
                    int32_t* data = va_arg(arguments, int32_t*);
                    
                    uint8_t j = 0;
                    while(j < length){
                        FRM_push((uint8_t)(data[j] & 0x000000ff));
                        FRM_push((uint8_t)((data[j] & 0x0000ff00) >> 8));
                        FRM_push((uint8_t)((data[j] & 0x00ff0000) >> 16));
                        FRM_push((uint8_t)((data[j] & 0xff000000) >> 24));
                        j++;
                    }
                    
                    break;
                }

                default:
                {

                }
            }

            i++;
        }while(i < dimensions);
    }
    
    FRM_finish();
}

void DIS_publish_str(const char* topic, char* str){
    uint16_t length, i;
    
    FRM_init();
    
    /* load the topic into the frame */
    i = 0;
    while(topic[i] != 0){
        FRM_push(topic[i]);
        i++;
    }
    
    /* send the string termination character */
    FRM_push(0);
    
    /* if the dimension == 0, then this is a string,
     *  simply transmit the string */
    FRM_push(1);

    /* send the length */
    length = strlen(str);
    FRM_push((uint8_t)(length & 0x00ff));
    FRM_push((uint8_t)((length & 0xff00) >> 8));

    /* send the format specifier for a string */
    FRM_push(eSTRING);

    /* finally, send the string */
    for(i = 0; i < length; i++){
        FRM_push(str[i]);
    }
    
    FRM_finish();
}

void DIS_publish_u8(const char* topic, uint8_t* data){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 1);
    
    /* send the format specifier */
    FRM_push((uint8_t)eU8);
    
    for(i = 0; i < dataLength; i++){
        FRM_push(data[i]);
    }
    
    FRM_finish();
}

void DIS_publish_s8(const char* topic, int8_t* data){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 1);
    
    /* send the format specifier */
    FRM_push((uint8_t)eS8);
    
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)data[i]);
    }
    
    FRM_finish();
}

void DIS_publish_2u8(const char* topic, uint8_t* data0, uint8_t* data1){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 2);
    
    /* send the format specifiers */
    FRM_push((uint8_t)eU8 | ((uint8_t)((eU8 & 0x0f) << 4)));
    
    /* send the first array */
    for(i = 0; i < dataLength; i++){
        FRM_push(data0[i]);
    }
    
    /* send the second array */
    for(i = 0; i < dataLength; i++){
        FRM_push(data1[i]);
    }
    
    FRM_finish();
}

void DIS_publish_2s8(const char* topic, int8_t* data0, int8_t* data1){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 2);
    
    /* send the format specifiers */
    FRM_push((uint8_t)eS8 | ((uint8_t)((eS8 & 0x0f) << 4)));
    
    /* send the first array */
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)data0[i]);
    }
    
    /* send the second array */
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)data1[i]);
    }
    
    FRM_finish();
}

void DIS_publish_u16(const char* topic, uint16_t* data){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 1);
    
    /* send the format specifier */
    FRM_push((uint8_t)eU16);
    
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data[i] & 0x00ff));
        FRM_push((uint8_t)((data[i] & 0xff00) >> 8));
    }
    
    FRM_finish();
}

void DIS_publish_s16(const char* topic, int16_t* data){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 1);
    
    /* send the format specifier */
    FRM_push((uint8_t)eS16);
    
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data[i] & 0x00ff));
        FRM_push((uint8_t)((data[i] & 0xff00) >> 8));
    }
    
    FRM_finish();
}

void DIS_publish_2u16(const char* topic, uint16_t* data0, uint16_t* data1){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 2);
    
    /* send the format specifiers */
    FRM_push((uint8_t)eU16 | ((uint8_t)((eU16 & 0x0f) << 4)));
    
    /* send the first array */
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data0[i] & 0x00ff));
        FRM_push((uint8_t)((data0[i] & 0xff00) >> 8));
    }
    
    /* send the second array */
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data1[i] & 0x00ff));
        FRM_push((uint8_t)((data1[i] & 0xff00) >> 8));
    }
    
    FRM_finish();
}

void DIS_publish_2s16(const char* topic, int16_t* data0, int16_t* data1){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 2);
    
    /* send the format specifiers */
    FRM_push((uint8_t)eS16 | ((uint8_t)((eS16 & 0x0f) << 4)));
    
    /* send the first array */
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data0[i] & 0x00ff));
        FRM_push((uint8_t)((data0[i] & 0xff00) >> 8));
    }
    
    /* send the second array */
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data1[i] & 0x00ff));
        FRM_push((uint8_t)((data1[i] & 0xff00) >> 8));
    }
    
    FRM_finish();
}

void DIS_publish_u32(const char* topic, uint32_t* data){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 1);
    
    /* send the format specifier */
    FRM_push((uint8_t)eU32);
    
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data[i] & 0x000000ff));
        FRM_push((uint8_t)((data[i] & 0x0000ff00) >> 8));
        FRM_push((uint8_t)((data[i] & 0x00ff0000) >> 16));
        FRM_push((uint8_t)((data[i] & 0xff000000) >> 24));
    }
    
    FRM_finish();
}

void DIS_publish_s32(const char* topic, int32_t* data){
    uint16_t i, dataLength;
    
    /* 'parseTopicString' will initialize the frame and push the topic,
     * dimensions, and headers to the framing library, returning the data
     * length */
    dataLength = parseTopicString(topic, 1);
    
    /* send the format specifier */
    FRM_push((uint8_t)eS32);
    
    for(i = 0; i < dataLength; i++){
        FRM_push((uint8_t)(data[i] & 0x000000ff));
        FRM_push((uint8_t)((data[i] & 0x0000ff00) >> 8));
        FRM_push((uint8_t)((data[i] & 0x00ff0000) >> 16));
        FRM_push((uint8_t)((data[i] & 0xff000000) >> 24));
    }
    
    FRM_finish();
}

uint16_t parseTopicString(const char* topic, uint8_t dimensions){
    uint16_t dataLength = 1, strIndex = 0;
    uint16_t i;
    
    FRM_init();
    
    /* load the topic into the frame */
    strIndex = 0;
    while((topic[strIndex] != 0)
            && (topic[strIndex] != ':')){
        FRM_push(topic[strIndex]);
        strIndex++;
    }
    
    /* send the string termination character */
    FRM_push(0);
    
    if(topic[strIndex] == ':'){
        strIndex++;
        
        /* copy the numeric part of the string into numStr */
        char numStr[8] = {0};
        i = 0;
        while(topic[strIndex] != 0){
            numStr[i] = topic[strIndex];
            i++;
            strIndex++;
        }
        
        /* convert the ASCII number into an integer */
        dataLength = (uint16_t)atol(numStr);
    }
    
    FRM_push(dimensions);

    /* send the length as two bytes */
    FRM_push((uint8_t)(dataLength & 0x00ff));
    FRM_push((uint8_t)((dataLength & 0xff00) >> 8));
    
    return dataLength;
}

void DIS_subscribe(const char* topic, void (*functPtr)()){
    /* find an empty subscription slot */
    uint16_t i;
    uint16_t found = 0;
    for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
        if(sub[i].subFunctPtr == 0){
            found = 1;
            break;
        }
    }
    
    /* subscribe to the 'found' slot */
    if(found == 1){
        /* copy the function pointer and the topic */
        sub[i].subFunctPtr = functPtr;
        
        uint16_t j = 0;
        while(topic[j] != 0){
            sub[i].topic[j] = topic[j];
            j++;
        }
    }
}

void DIS_unsubscribe(void (*functPtr)()){
    /* find the required subscription slot */
    uint16_t i;
    for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
        /* copy the function pointer and the topic */
        if(sub[i].subFunctPtr == functPtr){
            sub[i].subFunctPtr = 0;
            sub[i].topic[0] = 0;
        }
    }
}

void DIS_process(void){
    /* retrieve any messages from the framing buffer 
     * and process them appropriately */
    uint8_t data[MAX_RECEIVE_MESSAGE_LEN];
    if(FRM_pull(data) > 0){
        char topic[MAX_TOPIC_STR_LEN] = {0};
        uint16_t i = 0;
        uint16_t dataIndex = 0;
        
        /* decompose the message into its constituent parts */
        while(data[i] != 0){
            topic[i] = data[i];
            i++;
        }

        dataIndex = i + 1;
        
        rxMsg.dimensions = data[dataIndex++] & 0x0f;
        rxMsg.length = (uint16_t)data[dataIndex++];
        rxMsg.length += ((uint16_t)data[dataIndex++]) << 8;
        
        rxMsg.length8bit = 0;
        
        for(i = 0; i < rxMsg.dimensions; i++){
            if((i & 1) == 0){
                rxMsg.formatSpecifiers[i] = (FormatSpecifier)(data[dataIndex] & 0x0f);
            }else{
                rxMsg.formatSpecifiers[i] = (FormatSpecifier)((data[dataIndex++] & 0xf0) >> 4);
            }
            
            if((rxMsg.formatSpecifiers[i] == eSTRING)
                    || (rxMsg.formatSpecifiers[i] == eU8)
                    || (rxMsg.formatSpecifiers[i] == eS8)){
                rxMsg.length8bit += rxMsg.length;
            }else if((rxMsg.formatSpecifiers[i] == eU16)
                    || (rxMsg.formatSpecifiers[i] == eS16)){
                rxMsg.length8bit += (rxMsg.length << 1);
            }else if((rxMsg.formatSpecifiers[i] == eU32)
                    || (rxMsg.formatSpecifiers[i] == eS32)){
                rxMsg.length8bit += (rxMsg.length << 2);
            }
        }
        
        /* ensure that the data index is incremented
         * when the dimensions are odd */
        if(i & 1){
            dataIndex++;
        }
        
        /* keep from having to re-copy the buffer */
        uint8_t* dataWithOffset = data + dataIndex;
        
        /* copy the data to the rx array */
        for(i = 0; i < rxMsg.length8bit; i++){
            rxMsg.data[i] = dataWithOffset[i];
        }
        
        /* go through the active subscriptions and execute any
         * functions that are subscribed to the received topics */
        for(i = 0; i < MAX_NUM_OF_SUBSCRIPTIONS; i++){
            if(strcmp(topic, sub[i].topic) == 0){
                /* execute the function if it isn't empty */
                if(sub[i].subFunctPtr != 0){
                    sub[i].subFunctPtr();
                }
            }
        }
    }
}

uint16_t DIS_getElements(uint16_t element, void* destArray){
    uint16_t i;
    
    uint16_t currentIndex = getCurrentRxPointerIndex(element);
    
    switch(rxMsg.formatSpecifiers[element]){
        case eNONE:
        case eSTRING:
        {
            // typecast to a char
            char* data = (char*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                data[i] = rxMsg.data[i];
                i++;
            }
            
            break;
        }
        
        case eU8:
        {
            // typecast to a char
            uint8_t* data = (uint8_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                data[i] = (uint8_t)rxMsg.data[currentIndex + i];
                i++;
            }
            
            break;
        }
        
        case eS8:
        {
            // typecast to a char
            int8_t* data = (int8_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                data[i] = (int8_t)rxMsg.data[currentIndex + i];
                i++;
            }
            
            break;
        }
        
        case eU16:
        {
            // typecast to a unsigned int
            uint16_t* data = (uint16_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 1;
                data[dataIndex] = (uint16_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (uint16_t)rxMsg.data[currentIndex + i] << 8;
                i++;
            }
            
            break;
        }
        
        case eS16:
        {
            // typecast to a int
            int16_t* data = (int16_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 1;
                data[dataIndex] = (int16_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (int16_t)rxMsg.data[currentIndex + i] << 8;
                i++;
            }
            
            break;
        }
        
        case eU32:
        {
            // typecast to a int
            uint32_t* data = (uint32_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 2;
                data[dataIndex] = (uint32_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (uint32_t)rxMsg.data[currentIndex + i] << 8;
                i++;
                data[dataIndex] |= (uint32_t)rxMsg.data[currentIndex + i] << 16;
                i++;
                data[dataIndex] |= (uint32_t)rxMsg.data[currentIndex + i] << 24;
                i++;
            }
            
            break;
        }
        
        case eS32:
        {
            // typecast to a int
            int32_t* data = (int32_t*)destArray;
            
            // copy data to destination array
            i = 0;
            while(i < rxMsg.length8bit){
                uint16_t dataIndex = i >> 2;
                data[dataIndex] = (int32_t)rxMsg.data[currentIndex + i];
                i++;
                data[dataIndex] |= (int32_t)rxMsg.data[currentIndex + i] << 8;
                i++;
                data[dataIndex] |= (int32_t)rxMsg.data[currentIndex + i] << 16;
                i++;
                data[dataIndex] |= (int32_t)rxMsg.data[currentIndex + i] << 24;
                i++;
            }
            break;
        }
    }
    
    return rxMsg.length;
}

uint16_t getCurrentRxPointerIndex(uint8_t element){
    uint16_t i = 0;
    uint16_t currentIndex = 0;
    
    for(i = 0; i < element; i++){
        uint16_t widthInBytes = 0;
        if((rxMsg.formatSpecifiers[i] == eNONE)
                || (rxMsg.formatSpecifiers[i] == eU8)
                || (rxMsg.formatSpecifiers[i] == eS8)){
            widthInBytes = 1;
        }else if((rxMsg.formatSpecifiers[i] == eU16)
                || (rxMsg.formatSpecifiers[i] == eS16)){
            widthInBytes = 2;
        }else{
            widthInBytes = 4;
        }
        
        currentIndex += rxMsg.length * widthInBytes;
    }
    
    return currentIndex;
}

void DIS_assignChannelReadable(uint16_t (*functPtr)()){
    FRM_assignChannelReadable(functPtr);
}

void DIS_assignChannelWriteable(uint16_t (*functPtr)()){
    FRM_assignChannelWriteable(functPtr);
}

void DIS_assignChannelRead(void (*functPtr)(uint8_t* data, uint16_t length)){
    FRM_assignChannelRead(functPtr);
}

void DIS_assignChannelWrite(void (*functPtr)(uint8_t* data, uint16_t length)){
    FRM_assignChannelWrite(functPtr);
}