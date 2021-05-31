//
// Created by Alan Lin on 2021/1/27.
//

// Magic here is for some definition which linux only.
// TODO: windows impl rename it and place to a suitable place

#ifndef EYS3DPY_MAGIC_H
#define EYS3DPY_MAGIC_H

typedef enum{
    IMAGE_SN_NONSYNC = 0,
    IMAGE_SN_SYNC
} CONTROL_MODE;

typedef enum{
    DEPTH_IMG_NON_TRANSFER,
    DEPTH_IMG_GRAY_TRANSFER,
    DEPTH_IMG_COLORFUL_TRANSFER
}DEPTH_TRANSFER_CTRL;

#define ETronDI_ZD_TABLE_FILE_SIZE_11_BITS		4096
#define ETronDI_ZD_TABLE_FILE_SIZE_8_BITS		512
#define PATH_MAX 260
// for Sensor mode +
typedef enum
{
    SENSOR_A = 0,
    SENSOR_B,
    SENSOR_BOTH,
    SENSOR_C,
    SENSOR_D
} SENSORMODE_INFO;
// for Sensor mode +

#endif //EYS3DPY_MAGIC_H
