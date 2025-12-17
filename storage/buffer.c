/*************************************************************************************************/
/*!
 *  \file   buffer.c
 *
 *  \brief  Circular buffer implementation for offline event storage.
 */
/*************************************************************************************************/

#include "buffer.h"
#include <string.h>
#include <stdio.h>

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Circular buffer for events */
static WorkoutEvent_t s_buffer[BUFFER_MAX_EVENTS];

/*! Buffer head (next write position) */
static uint8_t s_head = 0;

/*! Buffer tail (next read position) */
static uint8_t s_tail = 0;

/*! Number of events in buffer */
static uint8_t s_count = 0;

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
void Buffer_Init(void)
{
    s_head = 0;
    s_tail = 0;
    s_count = 0;
    memset(s_buffer, 0, sizeof(s_buffer));
    
    printf("[BUFFER] Initialized (%d event capacity)\n", BUFFER_MAX_EVENTS);
}

/*************************************************************************************************/
bool Buffer_Push(const WorkoutEvent_t *pEvent)
{
    bool overflow = false;
    
    if (pEvent == NULL)
    {
        return false;
    }
    
    /* Check if buffer is full */
    if (s_count >= BUFFER_MAX_EVENTS)
    {
        /* Overwrite oldest event (move tail forward) */
        s_tail = (s_tail + 1) % BUFFER_MAX_EVENTS;
        overflow = true;
        printf("[BUFFER] WARNING: Buffer full, oldest event overwritten\n");
    }
    else
    {
        s_count++;
    }
    
    /* Copy event to buffer */
    memcpy(&s_buffer[s_head], pEvent, sizeof(WorkoutEvent_t));
    
    /* Move head forward */
    s_head = (s_head + 1) % BUFFER_MAX_EVENTS;
    
    return !overflow;
}

/*************************************************************************************************/
bool Buffer_Pop(WorkoutEvent_t *pEvent)
{
    if (pEvent == NULL || s_count == 0)
    {
        return false;
    }
    
    /* Copy event from buffer */
    memcpy(pEvent, &s_buffer[s_tail], sizeof(WorkoutEvent_t));
    
    /* Move tail forward */
    s_tail = (s_tail + 1) % BUFFER_MAX_EVENTS;
    s_count--;
    
    return true;
}

/*************************************************************************************************/
uint8_t Buffer_GetCount(void)
{
    return s_count;
}

/*************************************************************************************************/
bool Buffer_IsEmpty(void)
{
    return (s_count == 0);
}

/*************************************************************************************************/
void Buffer_Clear(void)
{
    s_head = 0;
    s_tail = 0;
    s_count = 0;
    
    printf("[BUFFER] Cleared\n");
}
