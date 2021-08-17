#include <unistd.h>
#include "devices/Pipeline.h"
#include "exampleBinder.h"

#define LOG_TAG	 "unity_wrapper"

using namespace std;
using namespace libeYs3D;
using namespace libeYs3D::video;

std::shared_ptr<EYS3DSystem> unity_eYs3DSystem = nullptr;
std::shared_ptr<libeYs3D::devices::CameraDevice> unity_device = nullptr;
std::shared_ptr<libeYs3D::devices::Pipeline> unity_pipeline = nullptr;
std::unique_ptr<ModeConfigOptions> modeConfigOptions;
ModeConfig::MODE_CONFIG unity_mode_config;
libeYs3D::video::DEPTH_RAW_DATA_TYPE unity_depth_raw_data_type;


CameraOpenConfig get_mode_config(int mode)
{
    CameraOpenConfig config = {
        0 , 1280 ,720 , 5 ,640 ,720, 2
    };
    bool isInterleave = false;
    auto currentUSBType = unity_device->getUsbPortType();
    unsigned short pid = unity_device->getCameraDeviceInfo().devInfo.wPID;
    modeConfigOptions = unity_device->getModeConfigOptions(currentUSBType, pid);
    for (auto& modeItem : modeConfigOptions->GetModes()) {
        if (modeItem.iMode == mode && APC_OK == modeConfigOptions->SelectCurrentIndex(mode)) {
            LOG_INFO(LOG_TAG, "Found index = %d\n", mode);
            break;
        }
    }
    LOG_INFO(LOG_TAG, "index = %d\n", modeConfigOptions->GetCurrentIndex());
    unity_mode_config = modeConfigOptions->GetCurrentModeInfo();
    LOG_INFO(LOG_TAG, "iMode=%d, iUSB_Type=%d, iInterLeaveModeFPS=%d, bRectifyMode=%d\n",
             unity_mode_config.iMode, unity_mode_config.iUSB_Type, unity_mode_config.iInterLeaveModeFPS,
             unity_mode_config.bRectifyMode);
    LOG_INFO(LOG_TAG, "eDecodeType_L=%d, L_Resolution.Width=%d ,L_Resolution.Height=%d, "
                      "D_Resolution.Width=%d, D_Resolution.Height=%d, vecDepthType=%d, vecColorFps=%d, vecDepthFps=%d\n",
                      unity_mode_config.eDecodeType_L, unity_mode_config.L_Resolution.Width, unity_mode_config.L_Resolution.Height,
                      unity_mode_config.D_Resolution.Width, unity_mode_config.D_Resolution.Height,
                      unity_mode_config.vecDepthType.at(0), unity_mode_config.vecColorFps.at(0), unity_mode_config.vecDepthFps.size());

    if(unity_mode_config.vecDepthType.empty()) {
        unity_depth_raw_data_type = libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_OFF_RAW;
    } else {
        switch(unity_mode_config.vecDepthType.at(0)){
            case 8:
                unity_depth_raw_data_type = unity_mode_config.bRectifyMode ?
                                            libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_8_BITS:
                                            libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_8_BITS_RAW;
                break;
            case 11:
                unity_depth_raw_data_type = unity_mode_config.bRectifyMode ?
                                            libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_11_BITS:
                                            libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_11_BITS_RAW;
                break;
            case 14:
                unity_depth_raw_data_type = unity_mode_config.bRectifyMode ?
                                            libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_14_BITS:
                                            libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_14_BITS_RAW;
                break;
        }
    }

    if (unity_device->getCameraDeviceInfo().devInfo.wPID == 0x120 && unity_mode_config.D_Resolution.Height == 360) {
        unity_depth_raw_data_type = static_cast<DEPTH_RAW_DATA_TYPE>(unity_depth_raw_data_type + APC_DEPTH_DATA_SCALE_DOWN_MODE_OFFSET);
    }

    if (isInterleave) {
        unity_depth_raw_data_type = static_cast<DEPTH_RAW_DATA_TYPE>(unity_depth_raw_data_type + APC_DEPTH_DATA_INTERLEAVE_MODE_OFFSET);
    }

    config.videoMode = unity_depth_raw_data_type;
    config.colorFormat = unity_mode_config.eDecodeType_L == ModeConfig::MODE_CONFIG::YUYV ?
            libeYs3D::video::COLOR_RAW_DATA_YUY2 : libeYs3D::video::COLOR_RAW_DATA_MJPG;
    config.colorWidth = unity_mode_config.L_Resolution.Width;
    config.colorHeight = unity_mode_config.L_Resolution.Height;
    config.fps = !unity_mode_config.vecColorFps.empty() ? unity_mode_config.vecColorFps.at(0) : unity_mode_config.vecDepthFps.at(0);
    config.depthWidth = unity_mode_config.D_Resolution.Width;
    config.depthHeight = unity_mode_config.D_Resolution.Height;

    LOG_INFO(LOG_TAG, "config: {%d, %d, %d, %d, %d, %d, %d} depth_raw_data_type=%d (%d, %d, %d)\n", config.colorFormat, config.colorWidth,
             config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode, unity_depth_raw_data_type,
             libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_8_BITS,
             libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_11_BITS,
             libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_14_BITS);
    return config;
}

libeYs3D::video::Frame mDepthFrame = {0};
int get_depth_frame(BYTE* dataInOut, size_t bufInSize, int type)
{
    unity_pipeline->waitForDepthFrame(&mDepthFrame);
    memcpy(dataInOut, mDepthFrame.rgbVec.data(), mDepthFrame.actualRGBBufferSize);
    return 0;
}

bool regenerate_palette(unsigned short zMin, unsigned short zFar)
{
    LOG_INFO(LOG_TAG, "regenerate_palette n:%d f:%d", zMin, zFar);
    if (zMin < 0) return false;
    if (zFar < zMin) return false;
    if (zFar >= 16384) return false;
    unity_device->setDepthOfField(zMin, zFar);
    return true;
}

void reset_palette()
{
    if (unity_device == nullptr) return;

    regenerate_palette(unity_device->mZDTableInfo.nZDTableMaxNear, 1000);
}

int setupIR(unsigned short value)
{
    return unity_device->setupIR(value);
}

int init_device(void)
{
    LOG_INFO(LOG_TAG, "Starting EYS3DSystem...");
    unity_eYs3DSystem = EYS3DSystem::getEYS3DSystem();
    unity_device = unity_eYs3DSystem->getCameraDevice(0);
    if(!unity_device)    {
        LOG_INFO(LOG_TAG, "Unable to find any camera devices...");
        exit(-1);
    }
    return 0;
}

int open_device(CameraOpenConfig config)
{
    printf("\n\nEnabling device stream...\n");
    unity_pipeline = unity_device->initStream((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
                                              config.colorWidth, config.colorHeight, config.fps,
                                              static_cast<DEPTH_RAW_DATA_TYPE>(config.videoMode),
                                              config.depthWidth, config.depthHeight,
                                              DEPTH_IMG_COLORFUL_TRANSFER,
                                              IMAGE_SN_SYNC,
                                              0);
    unity_device->enableStream();
    return (unity_pipeline != nullptr) ? APC_OK : APC_Init_Fail;
}


libeYs3D::video::Frame mColorFrame = {0};
int get_color_frame(BYTE* imageInOut)
{
    unity_pipeline->waitForColorFrame(&mColorFrame);
    memcpy(imageInOut, mColorFrame.rgbVec.data(), mColorFrame.actualRGBBufferSize);
    return 0;
}

void close_device(void)
{
    unity_device->closeStream();
    while (1) {
        sleep(1);
        if (!unity_device) {
            printf("doesn't need to CloseDevice()!\n");
            break;
        }
        unity_device->closeStream();
        break;
    }
}

void release_device(void)
{
}

int set_fw_register(unsigned short address, unsigned short value)
{
    if (unity_device == nullptr) return APC_Init_Fail;

    return unity_device->setFWRegister(address, value);
}

WORD get_depth_by_coordinate(int x, int y)
{
    if (x < 0 || y < 0) return 0;

    return mDepthFrame.zdDepthVec[y * mDepthFrame.width + x];
}

PCFrame mPcFrame = {0};
/**
 *
 * @param colorOut array contain more than W * H * sizeOfRGBPixel(3)
 * @param depthOut array space more than  W * H * sizeOfRGBPixel(3) * sizeOfFloat(4)
 * @return
 */
int generate_point_cloud_gpu(unsigned char *colorData, unsigned char *depthData,
                             unsigned char *colorOut, int* colorCapacity, float* depthOut, int* depthCapacity)
{
    if (!colorData || !depthData || !colorOut || !colorCapacity || !depthOut|| !depthCapacity) return APC_NullPtr;

    int ret = unity_pipeline->waitForPCFrame(&mPcFrame, 0 /* Forever */);

    *colorCapacity = mPcFrame.width * mPcFrame.height * 3 /*RGB*/ * sizeof(uint8_t);
    *depthCapacity = mPcFrame.width * mPcFrame.height * 3 /*XYZ*/ * sizeof(float);
    memcpy(colorOut, mPcFrame.rgbDataVec.data(), *colorCapacity);
    memcpy(depthOut, mPcFrame.xyzDataVec.data(), *depthCapacity);

    return ret;
}


void disable_AE() {
    if (unity_device == nullptr) return;
    unity_device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_EXPOSURE, 0);
}

void enable_AE() {
    if (unity_device == nullptr) return;
    unity_device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_EXPOSURE, 1);
}

int get_AE_status() {
     if (unity_device == nullptr) return APC_Init_Fail;
     auto property = unity_device->getCameraDeviceProperty(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_EXPOSURE);
     auto nValue = property.nValue;
     return nValue;
}

void disable_AWB() {
    if (unity_device == nullptr) return;
    int ret = unity_device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_WHITE_BLANCE, 0);
}

void enable_AWB() {
    if (unity_device == nullptr) return;
    int ret = unity_device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_WHITE_BLANCE, 1);
}

int get_AWB_status() {
    if (unity_device == nullptr) return APC_Init_Fail;
    auto property = unity_device->getCameraDeviceProperty(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_WHITE_BLANCE);
    auto nValue = property.nValue;
    return nValue;
}

void getDepthOfField(uint16_t *GetZNear, uint16_t *GetZFar) {
    if (unity_device == nullptr) return;
    unity_device->getDepthOfField(GetZNear, GetZFar);
}

int get_IR_min_value() {
    if (unity_device == nullptr) return APC_Init_Fail;
    libeYs3D::devices::IRProperty property = unity_device->getIRProperty();
    int ir_min = property.getIRMin();
    return ir_min;
}

int get_IR_max_value() {
    if (unity_device == nullptr) return APC_Init_Fail;
    libeYs3D::devices::IRProperty property = unity_device->getIRProperty();
    int ir_max = property.getIRMax();
    return ir_max;
}

int getDefaultZNear() {
    return 0;
}

int getDefaultZFar() {
    return 1000;
}
