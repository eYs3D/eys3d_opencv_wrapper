#include "utils.h"

#ifndef WIN32
#include "eSPDI.h"
#else
#include "eSPDI_Common.h"
#endif

#include "EYS3DSystem.h"
#include "devices/CameraDevice.h"
#include "devices/model/CameraDeviceProperties.h"
#include "video/Frame.h"
#include "debug.h"
#include "PlyWriter.h"

typedef struct {
    int colorFormat;
    int colorWidth;
    int colorHeight;
    int fps;
    int depthWidth;
    int depthHeight;
    int videoMode;
} CameraOpenConfig;

extern "C" {
    CameraOpenConfig get_mode_config();
    int get_depth_frame(BYTE* dataInOut, size_t bufInSize, int type);
    bool regenerate_palette(unsigned short zMin, unsigned short zFar);
    void reset_palette();
    int setupIR(unsigned short value);
    int init_device(void);
    int open_device(CameraOpenConfig config);
    int get_color_frame(BYTE* imageInOut);
    void close_device(void);
    void release_device(void);
    WORD get_depth_by_coordinate(int x, int y);
    int set_fw_register(unsigned short address, unsigned short value);
    int generate_point_cloud_gpu(unsigned char *colorData, unsigned char *depthData,
                                 unsigned char *colorOut, int* colorCapacity, float* depthOut, int* depthCapacity);
    void disable_AE();
    void enable_AE();
    int get_AE_status();
    void disable_AWB();
    void enable_AWB();
    int get_AWB_status();
    void getDepthOfField(uint16_t *GetZNear, uint16_t *GetZFar);
    int get_IR_min_value();
    int get_IR_max_value();
    int getDefaultZNear();
    int getDefaultZFar();

}
