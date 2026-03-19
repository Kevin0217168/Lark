#ifndef __MAIN_H__
#define __MAIN_H_

typedef enum{
    DEVICE_OFFLINE,
    DEVICE_STANDBY,
    DEVICE_ON_STREAM,
    DEVICE_ERROR,
}Device_Status_t;

typedef struct
{
    char name[48];
    char uuid[48];
    bool isOnline;
    Device_Status_t status;
} Device_t;

#endif