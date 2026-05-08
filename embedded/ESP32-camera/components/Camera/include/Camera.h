#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "esp_camera.h"

void CameraInit();
void CameraTakePhoto(void(PhotoHandler)(camera_fb_t *));


#endif