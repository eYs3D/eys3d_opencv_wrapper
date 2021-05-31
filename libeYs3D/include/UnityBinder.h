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
//CameraOpenConfig get_mode_config_with_index(int color_index, int depth_index, int depth_raw_index, int color_fps_index);
CameraOpenConfig get_mode_config_with_index(int mode_index, int depth_raw_index, int color_fps_index);
int get_depth_frame(BYTE* dataInOut, size_t bufInSize, int type);
bool regenerate_palette(unsigned short zMin, unsigned short zFar);
void reset_palette();
int get_default_near();
int get_default_far();
int setupIR(unsigned short value);
int init_device(void);
int open_device(CameraOpenConfig config);
int get_color_frame(BYTE* imageInOut);
void close_device(void);
void release_device(void);
WORD get_depth_by_coordinate(int x, int y);
int generate_point_cloud_gpu(unsigned char *colorData, unsigned char *depthData,
                             unsigned char *colorOut, int* colorCapacity, float* depthOut, int* depthCapacity);
}
