#include <opencv2/opencv.hpp>

#include "utils.h"

#ifndef WIN32
#include "eSPDI.h"
#else
#include "eSPDI_Common.h"
#endif

#include "EYS3DSystem.h"
#include "devices/CameraDevice.h"
#include "devices/model/CameraDeviceProperties.h"
#include "devices/FrameSetPipeline.h"
#include "video/Frame.h"
#include "base/threads/Async.h"
#include "debug.h"
#include "PlyWriter.h"
	
#include <thread>
#ifndef WIN32
#include <unistd.h>
#else
#include <stdio.h>
#endif
#include "main.h"
#define SUPPORT_VIZ
#ifdef SUPPORT_VIZ
#include <opencv2/viz.hpp>
#include <opencv2/viz/viz3d.hpp>
#endif

#define LOG_TAG	 "opencv_wrapper"
#define DURATION 30

typedef struct {
    int colorFormat;
    int colorWidth;
    int colorHeight;
    int fps;
    int depthWidth;
    int depthHeight;
    int videoMode;
} camera_open_config;

using namespace std;
using namespace libeYs3D;
using namespace libeYs3D::video;

std::shared_ptr<EYS3DSystem> eYs3DSystem = nullptr;
std::shared_ptr<libeYs3D::devices::CameraDevice> device = nullptr;
std::shared_ptr<libeYs3D::devices::FrameSetPipeline> pipeline;
std::unique_ptr<ModeConfigOptions> mModeConfigOptions;
ModeConfig::MODE_CONFIG mode_config;
std::vector< ModeConfig::MODE_CONFIG > m_modeConfigs;
//libeYs3D::video::DEPTH_RAW_DATA_TYPE depth_raw_data_type;
int videoMode = 2;

uint64_t gColorImgSize;
uint32_t gColorSerial;
uint64_t gDepthImgSize;
uint32_t gDepthSerial;

BYTE* color_frame = nullptr;
BYTE* depth_frame = nullptr;
BYTE *color_frame_out = nullptr;
float *depth_frame_out = nullptr;
int *zdTable = nullptr;

HANDLE updatePCHandle, updateColorDepth;

int gDefaultNear, gDefaultFar;

#ifndef WIN32
camera_open_config config = {
        0 , 1280 ,720 , 60 ,1280 ,720, 2
};
#else
camera_open_config config = {
        0 , 1280 ,720 , 5 ,640 ,720, 2
};
#endif

libeYs3D::video::DEPTH_RAW_DATA_TYPE depth_raw_data_type = libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_8_BITS;

#define SUPPORT_QT_OPENGL

#ifdef SUPPORT_QT_OPENGL
    int mSupport_QT_OpenGL = 1;
#else
    int mSupport_QT_OpenGL = 0;
#endif

//void config_mode(int color_index, int depth_index, int depth_raw_index, int color_fps_index)
void config_mode(int mode_index, int depth_raw_index, int color_fps_index)
{
	int i;
	mModeConfigOptions = device->getModeConfigOptions();
	m_modeConfigs = mModeConfigOptions->GetModes();
	printf("mode count = %d\n", m_modeConfigs.size());
	for(i = 0; i < m_modeConfigs.size(); i++)
		printf("[%d]", m_modeConfigs[i].iMode);
	printf("\n");
	mode_index = (mode_index < 0 || mode_index >= m_modeConfigs.size()) ? 0 : mode_index;
	//printf("index = %d\n", mModeConfigOptions->GetCurrentIndex());
	printf("selected mode index = %d\n", m_modeConfigs[mode_index].iMode);
	mModeConfigOptions->SelectCurrentIndex(m_modeConfigs[mode_index].iMode);
	mode_config = mModeConfigOptions->GetCurrentModeInfo();
	printf("iMode=%d, iUSB_Type=%d, iInterLeaveModeFPS=%d, bRectifyMode=%d\n"
		  , mode_config.iMode, mode_config.iUSB_Type, mode_config.iInterLeaveModeFPS, mode_config.bRectifyMode);
	printf("eDecodeType_L=%d, L_Resolution.Width=%d ,L_Resolution.Height=%d, D_Resolution.Width=%d, D_Resolution.Height=%d, vecDepthType=%d, vecColorFps=%d, vecDepthFps=%d\n"
	      , mode_config.eDecodeType_L, mode_config.L_Resolution.Width, mode_config.L_Resolution.Height, mode_config.D_Resolution.Width, mode_config.D_Resolution.Height
	      , mode_config.vecDepthType.at(0), mode_config.vecColorFps.at(0), mode_config.vecDepthFps.size());
	printf("mode_config.vecDepthType.size()=%d\n", mode_config.vecDepthType.size());
	depth_raw_index = (depth_raw_index < 0 || depth_raw_index >= mode_config.vecDepthType.size()) ? 0 : depth_raw_index;
	//switch(mode_config.vecDepthType.at(0)){
	switch(mode_config.vecDepthType.at(depth_raw_index)){
		case 8: depth_raw_data_type = libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_8_BITS;
		        break;
		case 11:depth_raw_data_type = libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_11_BITS;
		        break;
		case 14:depth_raw_data_type = libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_14_BITS;
		        break;
	}
	config.colorFormat = mode_config.eDecodeType_L;
	config.colorWidth = mode_config.L_Resolution.Width;
	config.colorHeight = mode_config.L_Resolution.Height;
	config.depthWidth = mode_config.D_Resolution.Width;
	config.depthHeight = mode_config.D_Resolution.Height;
	printf("vecColorFps.size()=%d, vecDepthFps.size()=%d\n", mode_config.vecColorFps.size(), mode_config.vecDepthFps.size());
	printf("vecColorFps=");
	for(i = 0; i < mode_config.vecColorFps.size(); i++)
		printf("[%d]", mode_config.vecColorFps.at(i));
	printf("\n");
	color_fps_index = (color_fps_index < 0 || color_fps_index >= mode_config.vecColorFps.size()) ? 0 : color_fps_index;
	config.fps = mode_config.vecColorFps.at(color_fps_index);
	printf("vecDepthFps=");
	for(i = 0; i < mode_config.vecDepthFps.size(); i++)
		printf("[%d]", mode_config.vecDepthFps.at(i));
	printf("\n");
	/*std::vector<ETRONDI_STREAM_INFO> mColorStreamInfo = device->getColorStreamInfo();
	std::vector<ETRONDI_STREAM_INFO> mDepthStreamInfo = device->getDepthStreamInfo();
	printf("mColorStreamInfo.size() = %d, mDepthStreamInfo.size() = %d\n", mColorStreamInfo.size(), mDepthStreamInfo.size());
	auto it = mColorStreamInfo.begin();
	for(i = 0; it != mColorStreamInfo.end(); ++it, i++)    {
		if(i == color_index)
			break;
	}
	config.colorFormat = (*it).bFormatMJPG;
	config.colorWidth = (*it).nWidth;
	config.colorHeight = (*it).nHeight;*/
    /*it = mDepthStreamInfo.begin();
	for(i = 0; it != mDepthStreamInfo.end(); ++it, i++)    {
		if(i == depth_index)
			break;
	}
	config.depthWidth = (*it).nWidth;
	config.depthHeight = (*it).nHeight;*/

	printf("config: {%d, %d, %d, %d, %d, %d, %d} depth_raw_data_type=%d (%d, %d, %d)\n", config.colorFormat, config.colorWidth, 
		config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode, depth_raw_data_type, 
		libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_8_BITS,
		libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_11_BITS,
		libeYs3D::video::DEPTH_RAW_DATA_TYPE::DEPTH_RAW_DATA_14_BITS);
}

void config_mode()
{
	config_mode(0, 0, 0);
}

static void frameset_reader(libeYs3D::devices::FrameSetPipeline *pipeline)     {
    libeYs3D::video::FrameSet frameSet;
    char buffer[1024];
    libeYs3D::devices::FrameSetPipeline::RESULT ret;
    //static int64_t lastSerialNumber = 0, c = -1;
    printf("test\n");
    
    while(true)    {
        ret = pipeline->waitForFrameSet(&frameSet);
        if(ret < 0)    break;
        if(ret > 0)    continue;
        
#if 1
        //if(frameSet.colorFrame.serialNumber > 0XFFF0)    {
        	ResetEvent(updateColorDepth);
        	memcpy(color_frame, frameSet.colorFrame.rgbVec.data(), frameSet.colorFrame.rgbVec.size() * sizeof(uint8_t));
			memcpy(depth_frame, frameSet.depthFrame.rgbVec.data(), frameSet.depthFrame.rgbVec.size() * sizeof(uint8_t));
			memcpy(zdTable, frameSet.depthFrame.zdDepthVec.data(), frameSet.depthFrame.zdDepthVec.size() * sizeof(uint16_t));
			SetEvent(updateColorDepth);
            //LOG_INFO(LOG_TAG, "[# PC #] S/N=%" PRIu32 "", frameSet.pcFrame.serialNumber);
            //LOG_INFO(LOG_TAG, "[# COLOR #] S/N=%" PRIu32 "", frameSet.colorFrame.serialNumber);
            //LOG_INFO(LOG_TAG, "[# DEPTH #] S/N=%" PRIu32 "", frameSet.depthFrame.serialNumber);
            //frameSet.pcFrame.toStringSimple(buffer, sizeof(buffer));
            //LOG_INFO(LOG_TAG, "[# PC #] %s", buffer);
        //}
        //if(frameSet.colorFrame.serialNumber - lastSerialNumber > 1)
        //	c++;
        //LOG_INFO(LOG_TAG, "Drop frame count = %d (%d->%d)", c, lastSerialNumber, frameSet.colorFrame.serialNumber);
        //lastSerialNumber = frameSet.colorFrame.serialNumber;
#endif

#if 0
        LOG_INFO(LOG_TAG, "[# PC #] S/N=%" PRIu32 "", frameSet.pcFrame.serialNumber);
        LOG_INFO(LOG_TAG, "[# COLOR #] S/N=%" PRIu32 "", frameSet.colorFrame.serialNumber);
        LOG_INFO(LOG_TAG, "[# DEPTH #] S/N=%" PRIu32 ", roiDepth=%" PRId32 ", roiZValue=%" PRId32 "",
                 frameSet.depthFrame.serialNumber,
                 frameSet.depthFrame.roiDepth,
                 frameSet.depthFrame.roiZValue);
#endif
    }
    
    LOG_INFO(LOG_TAG, "Exiting frameset_reader...");
}

static bool color_image_callback(const libeYs3D::video::Frame* frame)    {
    char buffer[512];
    static int64_t count = 0ll;
    static int64_t time = 0ll;
    static int64_t transcodingTime = 0ll;
    static int64_t filteringTime = 0ll;

	memcpy(color_frame, frame->rgbVec.data(), frame->rgbVec.size() * sizeof(uint8_t));
    //for (int i = 0; i < frame->rgbVec.size(); i++) {
    //    color_frame[i] = frame->rgbVec[i];
    //}

#if 1
    //if(frame->serialNumber > 0XFFFC)
        //LOG_INFO(LOG_TAG, "[# COLOR #] color_image_callback: S/N=%" PRIu32 "", frame->serialNumber);
#endif
#if 0
    frame->toStringSimple(buffer, sizeof(buffer));
    LOG_INFO(LOG_TAG": color_image_callback", "Color image: %s", buffer); 
#endif

#if 0
    if((count++ % DURATION) == 0)    {
        if(count != 1)    {
            int64_t temp = 0ll;
            LOG_INFO(LOG_TAG, "Color image trancoding cost average: %" PRId64 " ms...",
                     (transcodingTime / 1000 / DURATION));

            temp = (frame->tsUs - time) / 1000 / DURATION;
            LOG_INFO(LOG_TAG, "Color image cost average: %" PRId64 " ms, %" PRId64 " fps...",
                     temp, ((int64_t)1000) / temp);    
        }

        time = frame->tsUs;
        transcodingTime = frame->rgbTranscodingTime;
    } else    {
        transcodingTime += frame->rgbTranscodingTime;
    }
#endif

    return true;
}

static bool depth_image_callback(const libeYs3D::video::Frame* frame)    {
    char buffer[1024];
    static int64_t count = 0ll;
    static int64_t time = 0ll;
    static int64_t transcodingTime = 0ll;
    static int64_t filteringTime = 0ll;
    
    //printf("depth_size = %d, frame_size=%d\n", config.depthWidth * config.depthHeight * 8, frame->rgbVec.size());
	memcpy(depth_frame, frame->rgbVec.data(), frame->rgbVec.size() * sizeof(uint8_t));
	memcpy(zdTable, frame->zdDepthVec.data(), frame->zdDepthVec.size() * sizeof(uint16_t));
	//gDepthSerial = frame->serialNumber;
    //for (int i = 0; i < frame->rgbVec.size(); i++) {
    //    depth_frame[i] = frame->rgbVec[i];
    //}
	

#if 1
    //if(frame->serialNumber > 0XFFFC)
        //LOG_INFO(LOG_TAG, "[# DEPTH #] depth_image_callback: S/N=%" PRIu32 "", frame->serialNumber);
#endif
#if 0
    frame->toStringSimple(buffer, sizeof(buffer));
    LOG_INFO(LOG_TAG": depth_image_callback", "%s", buffer);
#endif
#if 0 
    if((count++ % DURATION) == 0)    {
        if(count != 1)    {
            int64_t temp = 0ll;
            LOG_INFO(LOG_TAG, "Depth image trancoding cost average: %" PRId64 " ms...",
                     (transcodingTime / 1000 / DURATION));
            LOG_INFO(LOG_TAG, "Depth image filtering cost average: %" PRId64 " ms...",
                     (filteringTime / 1000 / DURATION));
            temp = (frame->tsUs - time) / 1000 / DURATION;
            LOG_INFO(LOG_TAG, "Depth image cost average: %" PRId64 " ms, %" PRId64 " fps...",
                     temp, ((int64_t)1000) / temp);    
        }
        
        time = frame->tsUs;
        transcodingTime = frame->rgbTranscodingTime;
        filteringTime = frame->filteringTime;
    } else    {
        transcodingTime += frame->rgbTranscodingTime;
        filteringTime += frame->filteringTime;
    }
#endif
    return true;
}

static bool pc_frame_callback(const libeYs3D::video::PCFrame *pcFrame)    {
    char buffer[2048];
    static int64_t count = 0ll;
    static int64_t time = 0ll;
    static int64_t transcodingTime = 0ll;

    //int pcl_depth_size = config.depthWidth * config.depthHeight * 3 * sizeof(float);
    //int pcl_color_size = config.colorWidth * config.colorHeight * 8 * sizeof(BYTE);
	//printf("pcl_color_size = %d/%d, pcl_depth_size = %d/%d\n", pcl_color_size, pcFrame->rgbDataVec.size(), pcl_depth_size, pcFrame->xyzDataVec.size());
	
	ResetEvent(updatePCHandle);
	//printf("pcFrame->rgbDataVec.size() = %d, ", pcFrame->rgbDataVec.size());
	//printf("pcFrame->xyzDataVec.size() = %d\n", pcFrame->xyzDataVec.size());
    for (int i = 0; i < pcFrame->rgbDataVec.size(); i+=3) {
        color_frame_out[i] = pcFrame->rgbDataVec[i+2];
        color_frame_out[i+1] = pcFrame->rgbDataVec[i+1];
        color_frame_out[i+2] = pcFrame->rgbDataVec[i];
    }
	memcpy(depth_frame_out, pcFrame->xyzDataVec.data(), pcFrame->xyzDataVec.size() * sizeof(float));    
	SetEvent(updatePCHandle);
    //for (int i = 0; i < pcFrame->xyzDataVec.size(); i++) {
    //    depth_frame_out[i] = pcFrame->xyzDataVec[i];
    //}
    
#if 1
    //if(pcFrame->serialNumber > 0XFFFC)
        //LOG_INFO(LOG_TAG, "[# PC #] pc_image_callback: S/N=%" PRIu32 "", pcFrame->serialNumber);
#endif

#if 0
    pcFrame->toStringSimple(buffer, sizeof(buffer));
    LOG_INFO(LOG_TAG": pc_frame_callback", "%s", buffer);
#endif

#if 0
    if((count++ % DURATION) == 0)    {
        if(count != 1)    {
            int64_t temp = 0ll;
            LOG_INFO(LOG_TAG, "PC image trancoding cost average: %" PRId64 " ms...",
                     (transcodingTime / 1000 / DURATION));
                     
            temp = (pcFrame->tsUs - time) / 1000 / DURATION;
            LOG_INFO(LOG_TAG, "PC image cost average: %" PRId64 " ms, %" PRId64 " fps...",
                     temp, ((int64_t)1000) / temp);
        }
        
        time = pcFrame->tsUs;
        transcodingTime = pcFrame->transcodingTime;
    } else    {
        transcodingTime += pcFrame->transcodingTime;
    }
#endif

    return true;
}

WORD get_depth_by_coordinate(int x, int y)
{
	if(x >= 0 && x < config.depthWidth && y >= 0 && y < config.depthHeight)
		return zdTable[y * config.depthWidth + x];
	else
		return 0;
}

void ir_callback(int position, void*) {
    cout << "\n\nir_callback position:"<< position << endl;
    //setupIR(position);
    device->setupIR(position);
}

void ae_callback(int position, void*) {
    cout << "\n\nae_callback position:"<< position << endl;
    if (position == 0) {
        //cout << "\n\ndisable_AE ret:"<< disable_AE() << endl;
        cout << "\n\ndisable_AE ret:"<< device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_EXPOSURE, 2) << endl;
    } else {
        //cout << "\n\nenable_AE ret:"<< enable_AE() << endl;
        cout << "\n\nenable_AE ret:"<< device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_EXPOSURE, 1) << endl;
    }
    struct libeYs3D::devices::CameraDeviceProperties::CameraPropertyItem status;
    status = device->getCameraDeviceProperty(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_EXPOSURE);
    //cout << "\n\nget ae status:"<< ((get_AE_status() == 0) ? "enable" : "disable") << endl;// 0: enable; 1: disable
    //cout << "\n\nget ae status:"<< ((status.nValue == 0) ? "enable" : "disable") << endl;// 0: enable; 1: disable
    cout << "\n\nget ae status (Support, Valid, Value): ("<< status.bSupport << "," << status.bValid << "," << status.nValue << ")" << endl;// 0: enable; 1: disable
}

void awb_callback(int position, void*) {
	struct libeYs3D::devices::CameraDeviceProperties::CameraPropertyItem status;
    cout << "\n\nawb_callback position:"<< position << endl;
    if (position == 0) {
        //cout << "\n\ndisable_AWB ret:"<< disable_AWB() << endl;
        cout << "\n\ndisable_AWB ret:"<< device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_WHITE_BLANCE, 2) << endl;
        status = device->getCameraDeviceProperty(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::WHITE_BLANCE_TEMPERATURE);
        int temperature = status.nValue;
        device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::WHITE_BLANCE_TEMPERATURE, temperature);
    } else {
        //cout << "\n\nenable_AWB ret:"<< enable_AWB() << endl;
        cout << "\n\nenable_AWB ret:"<< device->setCameraDevicePropertyValue(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_WHITE_BLANCE, 1) << endl;
    }
	status = device->getCameraDeviceProperty(libeYs3D::devices::CameraDeviceProperties::CAMERA_PROPERTY::AUTO_WHITE_BLANCE);
    //cout << "\n\nget awb status:"<< ((get_AWB_status() == 0) ? "enable" : "disable") << endl;// 0: enable; 1: disable
	//cout << "\n\nget awb status:"<< ((status.nValue == 0) ? "enable" : "disable") << endl;// 0: enable; 1: disable
	//cout << "\n\nget awb status:"<< status.nValue << endl;// 0: enable; 1: disable
	cout << "\n\nget awb status (Support, Valid, Value): ("<< status.bSupport << "," << status.bValid << "," << status.nValue << ")" << endl;// 0: enable; 1: disable
}

void mouse_callback (int event, int x, int y, int flag, void* param) {
    window_display depth_wd = *(window_display*) param;
    switch(event) {
        case cv::EVENT_LBUTTONDOWN:
            break;
        case cv::EVENT_LBUTTONUP:
            {
                cout << "\n\nx:" << x << ",y:" << y << endl;
                int value = get_depth_by_coordinate(x, y);
                cout << "\n\nmeasurement:" << value << endl;
                string title = "x:" + to_string(x) + ",y:"
                               + to_string(y) + ",depth measurement:" + to_string(value) + "(mm)";
                cout << "\n\nmeasurement_text:" << title << endl;
                cv::setWindowTitle(depth_wd.id, title);
                break;
            }
        case cv::EVENT_RBUTTONUP:
            cv::setWindowTitle(depth_wd.id, depth_wd.id);
            break;
    }
}

void color_palette_handle (char input_key, int &count) {
    if (count <= 0) {
        cout << "\n============================================"<< endl;
        cout << "\nPlease input key to choose depth color palette item:" << endl;
        cout << "\nA or a: Reset palette" << endl;
        cout << "\nB or b: Regenerate palette" << endl;
        cout << "\n============================================"<< endl;
        count++;
    }
    if (input_key == 'A' || input_key == 'a') {
        char item_input;
        cout << "\n\nReset palette (Y/N):" << endl;
        cin >> item_input;
        while(true) {
            switch (item_input) {
                case 'Y':
                case 'y':
                    //reset_palette();
				    LOG_INFO(LOG_TAG, "regenerate_palette n:%d f:%d", gDefaultNear, gDefaultFar);
					device->setDepthOfField(gDefaultNear, gDefaultFar);
                    break;
                case 'N':
                case 'n':
                    //cout << "\nGetZNear:" << GetZNear() << "GetZFar:" << GetZFar() << endl;
                    cout << "\nGetZNear:" << device->mZDTableInfo.nZDTableMaxNear << "GetZFar:" << device->mZDTableInfo.nZDTableMaxFar << endl;
                    break;
                default:
                    cout << "That's not a choice."<< endl;
                	break;
                
            }
            if (item_input != 'Y' && item_input != 'y'
                && item_input != 'N' && item_input != 'n') {
                cout << "\nreset palette(Y/N)" << endl;
                cin >> item_input;
            } else {
                count = 0;
                break;
            }
        }
        count = 0;
    } else if (input_key == 'B' || input_key == 'b') {
        unsigned short near_input;
        unsigned short far_input;
        //unsigned short zMin = getDefaultZNear();
        //unsigned short zMin = 0;
        unsigned short zMin = gDefaultNear;
        //unsigned short zFar = getDefaultZFar();
        //unsigned short zFar = 1000;
        unsigned short zFar = 16384;
        cout << "\n\nDefault ZNear:" << zMin << endl;
        cout << "\n\nDefault ZFar:" << gDefaultFar << endl;
        cout << "\n\nRegenerate palette:" << endl;
        cout << "\n\nex> 325 1000 <ZNear(mm) ZFar(mm)>" << endl;

        cin >> near_input >> far_input;
        if (near_input < zMin || near_input > zFar || far_input < zMin || far_input >= zFar) {
            cout << "\n\n!!error palette range!!" << endl;
        } else {
            //regenerate_palette(near_input, far_input);
		    LOG_INFO(LOG_TAG, "regenerate_palette n:%d f:%d", near_input, far_input);
		    if (near_input >= 0 && far_input >= near_input && far_input < 16384)
				device->setDepthOfField(near_input, far_input);
        }
        count = 0;
    }
}

#ifdef SUPPORT_QT_OPENGL

    #define GL_GLEXT_PROTOTYPES
#ifndef WIN32
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glext.h>
#else
	#define GLEW_STATIC
	#include <GL/glew.h>
	#include <GL/glut.h>
#endif
    #include <opencv2/core/opengl.hpp>
    #include <thread>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif
	
    GLuint gVerticesBuffer = 0;
    GLuint gColorBuffer = 0;
    GLint gPositionLocation = 0;
    GLint gColorLocation = 0;
    GLint gMVPMatrixLocation = 0;
    unsigned int point_num = 0;

    float gProjectionMatrix[16] = {0};
    float gRoatationMatrix[16] = {0};

    #define GLSL(ver, source) "#version " #ver "\n" #source

    const GLchar *VertexShader = GLSL(
                                         440,
                                         uniform mat4 mvp_matrix;
                                                 attribute vec4 a_position;
                                                 attribute mediump vec3 a_color;
                                                 varying vec4 v_color;
                                                 void main() {
                                                     gl_Position = mvp_matrix * a_position;
                                                     v_color = vec4(a_color, 1.0f);
                                                 });

    const GLchar *FragmentShader = GLSL(
                                           440,
                                           uniform bool bSingleColor;
                                                   varying vec4 v_color;
                                                   void main() {
                                                       if (bSingleColor)
                                                       {
                                                           gl_FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
                                                       }
                                                       else
                                                       {
                                                           gl_FragColor = v_color;
                                                       }
                                                   });

    void eys3dIdentity(float matrix[16]) {
        memset(matrix, 0, sizeof(float) * 16);
        matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    }

    void eys3dPerspective(float verticalAngle, float aspectRatio, float nearPlane, float farPlane, float matrix[16]) {
        float radians = (verticalAngle / 2.0f) * M_PI / 180.0f;
        float sine = std::sin(radians);
        if (sine == 0.0f)
            return;
        float cotan = std::cos(radians) / sine;
        float clip = farPlane - nearPlane;
        matrix[0] = cotan / aspectRatio;
        matrix[1] = 0.0f;
        matrix[2] = 0.0f;
        matrix[3] = 0.0f;
        matrix[4] = 0.0f;
        matrix[5] = cotan;
        matrix[6] = 0.0f;
        matrix[7] = 0.0f;
        matrix[8] = 0.0f;
        matrix[9] = 0.0f;
        matrix[10] = -(nearPlane + farPlane) / clip;
        matrix[11] = -(2.0f * nearPlane * farPlane) / clip;
        matrix[12] = 0.0f;
        matrix[13] = 0.0f;
        matrix[14] = -1.0;
        matrix[15] = 0.0f;
    }

    void eys3dTranslate(float x, float y, float z, float matrix[16]) {
        matrix[3] += matrix[0] * x + matrix[1] * y + matrix[2] * z;
        matrix[7] += matrix[4] * x + matrix[5] * y + matrix[6] * z;
        matrix[11] += matrix[8] * x + matrix[9] * y + matrix[10] * z;
        matrix[15] += matrix[12] * x + matrix[13] * y + matrix[14] * z;
    }

    void eys3dRotate(float angle, float x, float y, float z, float matrix[16]) {
        if (angle == 0.0f)
            return;
        float c, s;
        if (angle == 90.0f || angle == -270.0f)
        {
            s = 1.0f;
            c = 0.0f;
        }
        else if (angle == -90.0f || angle == 270.0f)
        {
            s = -1.0f;
            c = 0.0f;
        }
        else if (angle == 180.0f || angle == -180.0f)
        {
            s = 0.0f;
            c = -1.0f;
        }
        else
        {
            float a = angle * M_PI / 180.0f;
            c = std::cos(a);
            s = std::sin(a);
        }

        if (x == 0.0f)
        {
            if (y == 0.0f)
            {
                if (z != 0.0f)
                {
                    // Rotate around the Z axis.
                    if (z < 0)
                        s = -s;
                    float tmp;
                    matrix[0] = (tmp = matrix[0]) * c + matrix[1] * s;
                    matrix[1] = matrix[1] * c - tmp * s;
                    matrix[4] = (tmp = matrix[4]) * c + matrix[5] * s;
                    matrix[5] = matrix[5] * c - tmp * s;
                    matrix[8] = (tmp = matrix[8]) * c + matrix[13] * s;
                    matrix[9] = matrix[9] * c - tmp * s;
                    matrix[12] = (tmp = matrix[12]) * c + matrix[13] * s;
                    matrix[13] = matrix[13] * c - tmp * s;
                    return;
                }
            }
            else if (z == 0.0f)
            {
                // Rotate around the Y axis.
                if (y < 0)
                    s = -s;
                float tmp;
                matrix[2] = (tmp = matrix[2]) * c + matrix[0] * s;
                matrix[0] = matrix[0] * c - tmp * s;
                matrix[6] = (tmp = matrix[6]) * c + matrix[4] * s;
                matrix[4] = matrix[4] * c - tmp * s;
                matrix[10] = (tmp = matrix[10]) * c + matrix[8] * s;
                matrix[8] = matrix[8] * c - tmp * s;
                matrix[14] = (tmp = matrix[14]) * c + matrix[12] * s;
                matrix[12] = matrix[12] * c - tmp * s;
            }
        }
        else if (y == 0.0f && z == 0.0f)
        {
            // Rotate around the X axis.
            if (x < 0)
                s = -s;
            float tmp;
            matrix[1] = (tmp = matrix[1]) * c + matrix[2] * s;
            matrix[2] = matrix[2] * c - tmp * s;
            matrix[5] = (tmp = matrix[5]) * c + matrix[6] * s;
            matrix[6] = matrix[6] * c - tmp * s;
            matrix[9] = (tmp = matrix[9]) * c + matrix[10] * s;
            matrix[10] = matrix[10] * c - tmp * s;
            matrix[13] = (tmp = matrix[13]) * c + matrix[14] * s;
            matrix[14] = matrix[14] * c - tmp * s;
        }
    }

    void eys3dMultiplyMatrix(float matrix1[16], float matrix2[16], float matrix_out[16]) {
        float m0, m1, m2;
        m0 = matrix2[0] * matrix1[0] +
             matrix2[4] * matrix1[1] +
             matrix2[8] * matrix1[2] +
             matrix2[12] * matrix1[3];
        m1 = matrix2[0] * matrix1[4] +
             matrix2[4] * matrix1[5] +
             matrix2[8] * matrix1[6] +
             matrix2[12] * matrix1[7];
        m2 = matrix2[0] * matrix1[8] +
             matrix2[4] * matrix1[9] +
             matrix2[8] * matrix1[10] +
             matrix2[12] * matrix1[11];

        matrix_out[0] = m0;
        matrix_out[4] = m1;
        matrix_out[8] = m2;
        matrix_out[12] = matrix2[0] * matrix1[12] +
                         matrix2[4] * matrix1[13] +
                         matrix2[8] * matrix1[14] +
                         matrix2[12] * matrix1[15];

        m0 = matrix2[1] * matrix1[0] +
             matrix2[5] * matrix1[1] +
             matrix2[9] * matrix1[2] +
             matrix2[13] * matrix1[3];
        m1 = matrix2[1] * matrix1[4] +
             matrix2[5] * matrix1[5] +
             matrix2[9] * matrix1[6] +
             matrix2[13] * matrix1[7];
        m2 = matrix2[1] * matrix1[8] +
             matrix2[5] * matrix1[9] +
             matrix2[9] * matrix1[10] +
             matrix2[13] * matrix1[11];
        matrix_out[1] = m0;
        matrix_out[5] = m1;
        matrix_out[9] = m2;
        matrix_out[13] = matrix2[1] * matrix1[12] +
                         matrix2[5] * matrix1[13] +
                         matrix2[9] * matrix1[14] +
                         matrix2[13] * matrix1[15];

        m0 = matrix2[2] * matrix1[0] +
             matrix2[6] * matrix1[1] +
             matrix2[10] * matrix1[2] +
             matrix2[14] * matrix1[3];
        m1 = matrix2[2] * matrix1[4] +
             matrix2[6] * matrix1[5] +
             matrix2[10] * matrix1[6] +
             matrix2[14] * matrix1[7];
        m2 = matrix2[2] * matrix1[8] +
             matrix2[6] * matrix1[9] +
             matrix2[10] * matrix1[10] +
             matrix2[14] * matrix1[11];

        matrix_out[2] = m0;
        matrix_out[6] = m1;
        matrix_out[10] = m2;
        matrix_out[14] = matrix2[2] * matrix1[12] +
                         matrix2[6] * matrix1[13] +
                         matrix2[10] * matrix1[14] +
                         matrix2[14] * matrix1[15];

        m0 = matrix2[3] * matrix1[0] +
             matrix2[7] * matrix1[1] +
             matrix2[11] * matrix1[2] +
             matrix2[15] * matrix1[3];
        m1 = matrix2[3] * matrix1[4] +
             matrix2[7] * matrix1[5] +
             matrix2[11] * matrix1[6] +
             matrix2[15] * matrix1[7];
        m2 = matrix2[3] * matrix1[8] +
             matrix2[7] * matrix1[9] +
             matrix2[11] * matrix1[10] +
             matrix2[15] * matrix1[11];
        matrix_out[3] = m0;
        matrix_out[7] = m1;
        matrix_out[11] = m2;
        matrix_out[15] = matrix2[3] * matrix1[12] +
                         matrix2[7] * matrix1[13] +
                         matrix2[11] * matrix1[14] +
                         matrix2[15] * matrix1[15];
    }

    void initShader() {
        GLint success;
        char infoLog[512];

        //Create a shader program object
#ifdef WIN32
		glewInit();
#endif
        GLuint ProgramId = glCreateProgram();

        GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);     //Create a Vertex Shader Object
        GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER); //Create a Fragment Shader Object

        glShaderSource(vertexShaderId, 1, &VertexShader, NULL);     //Retrieves the vertex shader source code
        glShaderSource(fragmentShaderId, 1, &FragmentShader, NULL); //Retrieves the fragment shader source code

        glCompileShader(vertexShaderId); //Compile the vertex shader
        glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                      << infoLog << std::endl;
        };

        glCompileShader(fragmentShaderId); //Compile the fragment shader
        glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShaderId, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAG::COMPILATION_FAILED\n"
                      << infoLog << std::endl;
        };

        //Attaches the vertex and fragment shaders to the shader program
        glAttachShader(ProgramId, vertexShaderId);
        glAttachShader(ProgramId, fragmentShaderId);

        GLenum err = glGetError();

        glLinkProgram(ProgramId); //Links the shader program
        glGetProgramiv(ProgramId, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ProgramId, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                      << infoLog << std::endl;
        }

        glUseProgram(ProgramId); //Uses the shader program

        err = glGetError();

        gPositionLocation = glGetAttribLocation(ProgramId, "a_position");
        gColorLocation = glGetAttribLocation(ProgramId, "a_color");
        gMVPMatrixLocation = glGetUniformLocation(ProgramId, "mvp_matrix");
    }

    void initGL() {
        initShader();
        if (0 == gVerticesBuffer)
            glGenBuffers(1, &gVerticesBuffer);

        if (0 == gColorBuffer)
            glGenBuffers(1, &gColorBuffer);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        eys3dIdentity(gRoatationMatrix);
        eys3dRotate(180.0f, 0.0f, 1.0f, 0.0f, gRoatationMatrix);
        eys3dRotate(180.0f, 0.0f, 0.0f, 1.0f, gRoatationMatrix);

        eys3dIdentity(gProjectionMatrix);
        eys3dPerspective(75.0f, (float)1280 / 720, 0.01f, 16384.0f, gProjectionMatrix);
    }

    int rotx = 0, roty = 0;
    void onDraw(void *paranm) {
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        float model[16], rotate[16];
        eys3dIdentity(rotate);
        eys3dTranslate(0.0f, 0.0, -610.0f, rotate);
        eys3dRotate(rotx, 1.0f, 0.0f, 0.0f, rotate);
        eys3dRotate(roty, 0.0f, 1.0f, 0.0f, rotate);
        eys3dTranslate(0.0f, 0.0, 610.0f, rotate);
        eys3dMultiplyMatrix(rotate, gRoatationMatrix, model);

        float out[16];
        eys3dMultiplyMatrix(gProjectionMatrix, model, out);

        glUniformMatrix4fv(gMVPMatrixLocation, 1, GL_TRUE, (GLfloat *)&out[0]);

        glBindBuffer(GL_ARRAY_BUFFER, gVerticesBuffer);
        glEnableVertexAttribArray(gPositionLocation);
        glVertexAttribPointer(gPositionLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, gColorBuffer);
        glEnableVertexAttribArray(gColorLocation);
        glVertexAttribPointer(gColorLocation, 3, GL_UNSIGNED_BYTE, GL_TRUE, 0, nullptr);

        glPointSize(1);
        glDrawArrays(GL_POINTS, 0, point_num);
    }

    void on_opengl(void *param) {
        cv::ogl::Texture2D *backgroundTex = (cv::ogl::Texture2D *) param;
        glEnable(GL_TEXTURE_2D);
        backgroundTex->bind();
        cv::ogl::render(*backgroundTex);
        glDisable(GL_TEXTURE_2D);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0, 0, -0.5);
        glRotatef(rotx, 1, 0, 0);
        glRotatef(roty, 0, 1, 0);
        glRotatef(0, 0, 0, 1);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        static const int coords[6][4][3] = {
                {{+1, -1, -1}, {-1, -1, -1}, {-1, +1, -1}, {+1, +1, -1}},
                {{+1, +1, -1}, {-1, +1, -1}, {-1, +1, +1}, {+1, +1, +1}},
                {{+1, -1, +1}, {+1, -1, -1}, {+1, +1, -1}, {+1, +1, +1}},
                {{-1, -1, -1}, {-1, -1, +1}, {-1, +1, +1}, {-1, +1, -1}},
                {{+1, -1, +1}, {-1, -1, +1}, {-1, -1, -1}, {+1, -1, -1}},
                {{-1, -1, +1}, {+1, -1, +1}, {+1, +1, +1}, {-1, +1, +1}}
        };
        for (int i = 0; i < 6; ++i) {
            glColor3ub(i * 20, 100 + i * 10, i * 42);
            glBegin(GL_QUADS);
            for (int j = 0; j < 4; ++j) {
                glVertex3d(
                        0.2 * coords[i][j][0],
                        0.2 * coords[i][j][1],
                        0.2 * coords[i][j][2]
                );
            }
            glEnd();
        }
    }

    void on_trackbar(int position, void * param) {
        window_display example_wd = *(window_display*) param;
        cv::updateWindow(example_wd.id);
    }

#endif


int main (int argc, char** argv) {
    show_menu();
    char item_input;
    string build_info;
    int ret;
    int ir_input;
    cin >> item_input;
    while(item_input != 'Q' && item_input != 'q') {
        switch (item_input) {
            case '0':
                preview_color_depth();
                break;
            case '1':
                preview_all();
                break;
            case '2':
                face_detect();
                break;
            case '3':
                face_mask_detect();
                break;
            case '4':
                point_cloud_view();
                break;
            case '5':
                point_cloud_view_with_opengl();
                break;
            case '6': {
                cout << "OpenGL example" << endl;
#ifdef SUPPORT_QT_OPENGL
                cv::Mat img = cv::imread("asset/img.jpeg");
                if( img.empty() ) {
                    cout << "Cannot load asset/img.jpeg" << endl;
                    return -1;
                }
                window_display example_wd = {"example_wd", 640, 360, 0, 0};
                cv::namedWindow( example_wd.id, CV_WINDOW_OPENGL );
                cv::resizeWindow(example_wd.id, 640, 360);
                cv::createTrackbar( "X-rotation", example_wd.id, &rotx, 360, on_trackbar, (void*)&example_wd);
                cv::createTrackbar( "Y-rotation", example_wd.id, &roty, 360, on_trackbar, (void*)&example_wd);

                cv::ogl::Texture2D backgroundTex(img);
                cv::setOpenGlDrawCallback(example_wd.id, on_opengl, &backgroundTex);
                cv::updateWindow(example_wd.id);
#ifndef WIN32                
				while(cv::getWindowProperty(example_wd.id, cv::WND_PROP_VISIBLE) >= 1) {
                    cv::waitKey(30);
                }
#else
                while(cv::waitKey(30) != 27){
                }
#endif                
                cout << "OpenGL example exit" << endl;
                cv::setOpenGlDrawCallback(example_wd.id, 0, 0);
                cv::destroyAllWindows();
#else
                cout << "Not support ITEM6!"<< endl;
#endif
            }
                break;
            case 'I':
            case 'i':
                build_info = cv::getBuildInformation();
                cout << "\nbuild_info:"<< build_info << endl;
                break;
            default:
                cout << "That's not a choice."<< endl;
        }
        show_menu();
        cin >> item_input;
    }
    cout << "Bye!"<< endl;
    return 0;
}

void show_menu() {
    cout << "\n============================================"<< endl;
    cout << "\nSoftware VERSION:"<< VERSION << endl;
    cout << "\nPlease input key to choose test item:" << endl;
    cout << "\n0: Preview Color , Depth" << endl;
    cout << "\n1: Preview Color, Depth, Grey, Canny" << endl;
    cout << "\n2: Face detect" << endl;
    cout << "\n3: Face mask detect" << endl;
    cout << "\n4: Point cloud view" << endl;
    cout << "\n5: Point cloud view with opengl" << endl;
    cout << "\n6: OpenGL example" << endl;
    cout << "\nI or i: Show opencv build information" << endl;
    cout << "\nQ or q: Exit"<< endl;
    cout << "\n============================================"<< endl;
}

void preview_color_depth() {
    cout << "\n\npreview_color_depth item\n"<< endl;
    //prepare window
    window_display color_wd = {"color_wd", 640, 360, 0, 0};
    window_display depth_wd = {"depth_wd", 640, 360, 720, 0};
    if (mSupport_QT_OpenGL) {
        cv::namedWindow(color_wd.id, CV_GUI_NORMAL);
        cv::resizeWindow(color_wd.id, color_wd.width, color_wd.height);
        cv::moveWindow(color_wd.id, 0, 0);
        cv::namedWindow(depth_wd.id, CV_GUI_NORMAL);
        cv::resizeWindow(depth_wd.id, depth_wd.width, depth_wd.height);
        cv::moveWindow(depth_wd.id, 0, 0);
    } else {
        cv::namedWindow(color_wd.id, cv::WINDOW_NORMAL);
        cv::resizeWindow(color_wd.id, color_wd.width, color_wd.height);
        cv::moveWindow(color_wd.id, color_wd.x, color_wd.y);
        cv::namedWindow(depth_wd.id, cv::WINDOW_NORMAL);
        cv::resizeWindow(depth_wd.id, depth_wd.width, depth_wd.height);
        cv::moveWindow(depth_wd.id, depth_wd.x, depth_wd.y);
    }
	LOG_INFO(LOG_TAG, "Starting EYS3DSystem...");
	eYs3DSystem = EYS3DSystem::getEYS3DSystem();
    device = eYs3DSystem->getCameraDevice(0);
	if(!device)    {
        LOG_INFO(LOG_TAG, "Unable to find any camera devices...");
        exit(-1);
    }
	config_mode();
    printf("\ncolorFormat %d, colorWidth:%d, colorHeight:%d, fps:%d, depthWidth:%d, depthHeight:%d, videoMode:%d\n",
           config.colorFormat, config.colorWidth, config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode);
    //prepare color, depth frame
    int color_size = config.colorWidth * config.colorHeight * 3;
    BYTE* color_frame0 = new BYTE [color_size];
    //BYTE* color_frame = new BYTE [color_size];
    if(color_frame == nullptr)
    	color_frame = new BYTE [color_size];
    cout << "\ncolor_frame size: " << color_size << endl;
    //int depth_size = config.depthWidth * config.depthHeight * 8;
    int depth_size = config.depthWidth * config.depthHeight * 3;
    //BYTE* depth_frame = new BYTE [depth_size];
    if(depth_frame == nullptr)
    	depth_frame = new BYTE [depth_size];
    cout << "\nget_depth_handler size: " << depth_size << endl;
    if(zdTable == nullptr)
    	zdTable = new int [depth_size];

    //eSPDI camera open
    int ret;
    //ret = init_device();
    //cout << "\ninit_device:"<< ret << endl;
    //ret = open_device(config);
	printf("\n\nEnabling device stream...\n");
#if 0
	pipeline = device->initStreamFS((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0);
	if(pipeline != nullptr){
		base::async([&]() { frameset_reader(pipeline.get()); });
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
		gDefaultNear = device->mZDTableInfo.nZDTableMaxNear;
		gDefaultFar = device->mZDTableInfo.nZDTableMaxFar;
		updateColorDepth = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
#else
	ret = device->initStream((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0, // rectifyLogIndex
	         color_image_callback,
	         depth_image_callback,
	         nullptr);
	if(ret == ETronDI_OK){
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
		gDefaultNear = device->mZDTableInfo.nZDTableMaxNear;
		gDefaultFar = device->mZDTableInfo.nZDTableMaxFar;
	}
    cout << "\nopen_device:"<< ret << endl;
#endif
    window_display settings_wd = {"settings_wd", 640, 360, 0, 440};
    if (mSupport_QT_OpenGL) {
        //create IR Trackbar
        //int ir_min = get_IR_min_value();
        libeYs3D::devices::IRProperty property = device->getIRProperty();
        int ir_min = property.getIRMin();
        int ir_max = property.getIRMax();
        //cv::createTrackbar(SETTINGS_IR, color_wd.id, &ir_min, get_IR_max_value(), ir_callback, (void*)&color_wd);
        cv::createTrackbar(SETTINGS_IR, color_wd.id, &ir_min, ir_max, ir_callback, (void*)&color_wd);
        cv::setTrackbarPos(SETTINGS_IR, color_wd.id, 3);
        //create AE Trackbar
        int ae_min = 0;
        cv::createTrackbar(SETTINGS_AE, color_wd.id, &ae_min, 1, ae_callback, (void*)&color_wd);
        cv::setTrackbarPos(SETTINGS_AE, color_wd.id, 1);
        //create AWB Trackbar
        int awb_min = 0;
        cv::createTrackbar(SETTINGS_AWB, color_wd.id, &awb_min, 1, awb_callback, (void*)&color_wd);
        cv::setTrackbarPos(SETTINGS_AWB, color_wd.id, 1);
    } else {
        //show settings window white background
        //cv::namedWindow(settings_wd.id, cv::WINDOW_AUTOSIZE);
        cv::namedWindow(settings_wd.id, cv::WINDOW_NORMAL);
        cv::moveWindow(settings_wd.id, settings_wd.x, settings_wd.y);
        cv::Mat white_image(settings_wd.height,settings_wd.width, CV_8UC3);
        for (int r = 0; r < white_image.rows; r++) {
            for (int c = 0; c < white_image.cols; c++) {
                white_image.at<cv::Vec3b>(r, c)[0] = 255;
                white_image.at<cv::Vec3b>(r, c)[1] = 255;
                white_image.at<cv::Vec3b>(r, c)[2] = 255;
            }
        }
        show_settings_content(settings_wd);
        cv::imshow(settings_wd.id, white_image);
    }
    //mouse callback for depth measurement
    cv::setMouseCallback(depth_wd.id, mouse_callback, (void*)&depth_wd);
    int count = 0;
#ifdef SUPPORT_QT_OPENGL
#ifndef WIN32
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_VISIBLE) >= 1
        && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_VISIBLE) >= 1) {
#else
    while(cv::waitKey(30) != 27){
#endif        	
#else
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
        && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
        && cv::getWindowProperty(settings_wd.id, cv::WND_PROP_AUTOSIZE) >= 0) {
#endif

        //eSPDI camera get frame
        //ret = get_color_frame(color_frame);
		//ret = device->readColorFrame((BYTE*)color_frame, (uint64_t)2 * config.colorWidth * config.colorHeight, (uint64_t *)&gColorImgSize, (uint32_t *)&gColorSerial);
        //save_file(color_frame, color_size, config.colorWidth, config.colorHeight, 1, true);
        //int size=0, type;
        //ret = tjpeg2yuv(color_frame,2 * config.colorWidth * config.colorHeight,&color_frame0, &size,&type);
        //ret = tyuv2rgb(color_frame0,size, config.colorWidth, config.colorHeight, type,&color_frame,&size);

        //if (ret != 0) {
        //    cout << "\nget_color_frame failed:" << ret << endl;
        //    continue;
        //}
        //ret = get_depth_frame(depth_frame, depth_size, 1);
		//ret = device->readDepthFrame((BYTE*)depth_frame, (uint64_t)depth_size, (uint64_t *)&gDepthImgSize, (uint32_t *)&gDepthSerial);
        //if (ret != 0) {
        //    cout << "\nget_depth_frame failed:" << ret << endl;
        //    continue;
        //}
		WaitForSingleObject(updateColorDepth, INFINITE);
        ResetEvent(updateColorDepth);
        //cvt to MAT
        //cv::Mat color_bgr_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, (void*)color_frame, cv::Mat::AUTO_STEP);
        cv::Mat color_rgb_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, (void*)color_frame, cv::Mat::AUTO_STEP);
        //cv::Mat color_rgb_img;
        //cv::cvtColor(color_bgr_img, color_rgb_img, cv::COLOR_RGB2BGR);
        if (color_rgb_img.empty()) {
            cout << "\ncolor is empty " << endl;
            continue;
        }
        cv::Mat depth_bgr_img(cv::Size(config.depthWidth, config.depthHeight), CV_8UC3, (void *) depth_frame,
                              cv::Mat::AUTO_STEP);
        cv::Mat depth_rgb_img;
        cv::cvtColor(depth_bgr_img, depth_rgb_img, cv::COLOR_RGB2BGR);
        if (depth_rgb_img.empty()) {
            cout << "\ndepth is empty " << endl;
            continue;
        }
        //image show
        cv::imshow(color_wd.id, color_rgb_img);
        cv::imshow(depth_wd.id, depth_rgb_img);
        char input_key = cv::waitKey(30);
        color_palette_handle(input_key, count);
    }
    cout << "\nstop color streaming... " << endl;
    //destroy and release
    cv::destroyAllWindows();
    if (color_frame)
        delete color_frame;
    if (color_frame0)
        delete color_frame0;
    if (depth_frame)
        delete depth_frame;
    if (zdTable)
        delete zdTable;
    cout << "\nclose_device:"<< endl;
    //close_device();
	while (1) {
		Sleep(1);
		if (!device) {
			printf("doesn't need to CloseDevice()!\n");
			break;

		}
		device->closeStream();
		break;
	}
    cout << "\nrelease_device:"<< endl;
    //release_device();
	if (nullptr != device.get()) {
		delete device.get();
		device = nullptr;

	}
	if (nullptr != eYs3DSystem.get()) {
		delete eYs3DSystem.get();
		eYs3DSystem == nullptr;
	}
}

void preview_all() {
    cout << "\n\npreview_all item\n"<< endl;
    //prepare window
    window_display color_wd = {"color_wd", 640, 360, 0, 0};
    window_display depth_wd = {"depth_wd", 640, 360, 720, 0};
    window_display grey_wd = {"grey_wd", 640, 360, 0, 440};
    window_display canny_wd = {"canny_wd", 640, 360, 720, 440};
    if (mSupport_QT_OpenGL) {
        cv::namedWindow(color_wd.id, CV_GUI_NORMAL);
        cv::resizeWindow(color_wd.id, color_wd.width, color_wd.height);
        cv::moveWindow(color_wd.id, color_wd.x, color_wd.y);
        cv::namedWindow(depth_wd.id, CV_GUI_NORMAL);
        cv::resizeWindow(depth_wd.id, depth_wd.width, depth_wd.height);
        cv::moveWindow(depth_wd.id, depth_wd.x, depth_wd.y);
        cv::namedWindow(grey_wd.id, CV_GUI_NORMAL);
        cv::resizeWindow(grey_wd.id, grey_wd.width, grey_wd.height);
        cv::moveWindow(grey_wd.id, grey_wd.x, grey_wd.y);
        cv::namedWindow(canny_wd.id, CV_GUI_NORMAL);
        cv::resizeWindow(canny_wd.id, canny_wd.width, canny_wd.height);
        cv::moveWindow(canny_wd.id, canny_wd.x, canny_wd.y);
    } else {
        cv::namedWindow(color_wd.id, cv::WINDOW_NORMAL);
        cv::resizeWindow(color_wd.id, color_wd.width, color_wd.height);
        cv::moveWindow(color_wd.id, color_wd.x, color_wd.y);
        cv::namedWindow(depth_wd.id, cv::WINDOW_NORMAL);
        cv::resizeWindow(depth_wd.id, depth_wd.width, depth_wd.height);
        cv::moveWindow(depth_wd.id, depth_wd.x, depth_wd.y);
        cv::namedWindow(grey_wd.id, cv::WINDOW_NORMAL);
        cv::resizeWindow(grey_wd.id, grey_wd.width, grey_wd.height);
        cv::moveWindow(grey_wd.id, grey_wd.x, grey_wd.y);
        cv::namedWindow(canny_wd.id, cv::WINDOW_NORMAL);
        cv::resizeWindow(canny_wd.id, canny_wd.width, canny_wd.height);
        cv::moveWindow(canny_wd.id, canny_wd.x, canny_wd.y);
    }
	LOG_INFO(LOG_TAG, "Starting EYS3DSystem...");
	eYs3DSystem = EYS3DSystem::getEYS3DSystem();
    device = eYs3DSystem->getCameraDevice(0);
	if(!device)    {
        LOG_INFO(LOG_TAG, "Unable to find any camera devices...");
        exit(-1);
    }
	//config_mode(2, 1, 0);
	config_mode();
    printf("\ncolorFormat %d, colorWidth:%d, colorHeight:%d, fps:%d, depthWidth:%d, depthHeight:%d, videoMode:%d\n",
           config.colorFormat, config.colorWidth, config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode);
    //prepare color, depth frame
    int color_size = config.colorWidth * config.colorHeight * 3;
    BYTE* color_frame0 = new BYTE [color_size];
    //BYTE* color_frame = new BYTE [color_size];
    if(color_frame == nullptr)
    	color_frame = new BYTE [color_size];
    cout << "\ncolor_frame size: " << color_size << endl;
    int depth_size = config.depthWidth * config.depthHeight * 3;
    //int depth_size = config.depthWidth * config.depthHeight * 8;
    //BYTE* depth_frame = new BYTE [depth_size];
    if(depth_frame == nullptr)
    	depth_frame = new BYTE [depth_size];
    cout << "\nget_depth_handler size: " << depth_size << endl;
    if(zdTable == nullptr)
    	zdTable = new int [depth_size];
    
    //eSPDI camera open
    int ret;
    //ret = init_device();
    //cout << "\ninit_device:"<< ret << endl;
    //ret = open_device(config);
	printf("\n\nEnabling device stream...\n");
#if 0
	pipeline = device->initStreamFS((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0);
	if(pipeline != nullptr){
		base::async([&]() { frameset_reader(pipeline.get()); });
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
		gDefaultNear = device->mZDTableInfo.nZDTableMaxNear;
		gDefaultFar = device->mZDTableInfo.nZDTableMaxFar;
		updateColorDepth = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
#else
	ret = device->initStream((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0, // rectifyLogIndex
	         color_image_callback,
	         depth_image_callback,
	         nullptr);
	if(ret == ETronDI_OK){
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
		gDefaultNear = device->mZDTableInfo.nZDTableMaxNear;
		gDefaultFar = device->mZDTableInfo.nZDTableMaxFar;
	}
    cout << "\nopen_device:"<< ret << endl;
#endif

    //show settings window white background
    window_display settings_wd = {"settings_wd", 320, 180, 1400, 0};
    if (mSupport_QT_OpenGL) {
        //create IR Trackbar
        //int ir_min = get_IR_min_value();
        libeYs3D::devices::IRProperty property = device->getIRProperty();
        int ir_min = property.getIRMin();
        int ir_max = property.getIRMax();
        //cv::createTrackbar(SETTINGS_IR, color_wd.id, &ir_min, get_IR_max_value(), ir_callback, (void*)&color_wd);
        cv::createTrackbar(SETTINGS_IR, color_wd.id, &ir_min, ir_max, ir_callback, (void*)&color_wd);
        cv::setTrackbarPos(SETTINGS_IR, color_wd.id, 3);
        //create AE Trackbar
        int ae_min = 0;
        cv::createTrackbar(SETTINGS_AE, color_wd.id, &ae_min, 1, ae_callback, (void*)&color_wd);
        cv::setTrackbarPos(SETTINGS_AE, color_wd.id, 1);
        //create AWB Trackbar
        int awb_min = 0;
        cv::createTrackbar(SETTINGS_AWB, color_wd.id, &awb_min, 1, awb_callback, (void*)&color_wd);
        cv::setTrackbarPos(SETTINGS_AWB, color_wd.id, 1);
    } else {
        //show settings window white background
        //cv::namedWindow(settings_wd.id, cv::WINDOW_AUTOSIZE);
        cv::namedWindow(settings_wd.id, cv::WINDOW_NORMAL);
        cv::moveWindow(settings_wd.id, settings_wd.x, settings_wd.y);
        cv::Mat white_image(settings_wd.height,settings_wd.width, CV_8UC3);
        for (int r = 0; r < white_image.rows; r++) {
            for (int c = 0; c < white_image.cols; c++) {
                white_image.at<cv::Vec3b>(r, c)[0] = 255;
                white_image.at<cv::Vec3b>(r, c)[1] = 255;
                white_image.at<cv::Vec3b>(r, c)[2] = 255;
            }
        }
        show_settings_content(settings_wd);
        cv::imshow(settings_wd.id, white_image);
    }
    //mouse callback for depth measurement
    cv::setMouseCallback(depth_wd.id, mouse_callback, (void*)&depth_wd);
    int count = 0;
#ifdef SUPPORT_QT_OPENGL
#ifndef WIN32
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_VISIBLE) >= 1
        && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_VISIBLE) >= 1
        && cv::getWindowProperty(grey_wd.id, cv::WND_PROP_VISIBLE) >= 1
        && cv::getWindowProperty(canny_wd.id, cv::WND_PROP_VISIBLE) >= 1) {
#else
    while(cv::waitKey(30) != 27){
#endif        	
#else
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
          && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
          && cv::getWindowProperty(grey_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
          && cv::getWindowProperty(canny_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
          && cv::getWindowProperty(settings_wd.id, cv::WND_PROP_AUTOSIZE) >= 0) {
#endif
        //eSPDI camera get frame
        //get_color_frame(color_frame);
		//ret = device->readColorFrame((BYTE*)color_frame, (uint64_t)2 * config.colorWidth * config.colorHeight, (uint64_t *)&gColorImgSize, (uint32_t *)&gColorSerial);
        //get_depth_frame(depth_frame, depth_size, 0);
		//ret = device->readDepthFrame((BYTE*)depth_frame, (uint64_t)depth_size, (uint64_t *)&gDepthImgSize, (uint32_t *)&gDepthSerial);        
        //int size=0, type;
        //ret = tjpeg2yuv(color_frame,2 * config.colorWidth * config.colorHeight,&color_frame0, &size,&type);
        //ret = tyuv2rgb(color_frame0,size, config.colorWidth, config.colorHeight, type,&color_frame,&size);
		WaitForSingleObject(updateColorDepth, INFINITE);
        ResetEvent(updateColorDepth);
        //cvt to MAT
        //cv::Mat color_bgr_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, (void*)color_frame, cv::Mat::AUTO_STEP);
        //cv::Mat color_bgr_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, color_frame, cv::Mat::AUTO_STEP);
        cv::Mat color_rgb_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, color_frame, cv::Mat::AUTO_STEP);
        //cv::Mat color_rgb_img;
        //cv::cvtColor(color_bgr_img, color_rgb_img, cv::COLOR_RGB2BGR);
        if (color_rgb_img.empty()) {
            cout << "\ncolor is empty " << endl;
            continue;
        }
        cv::Mat depth_bgr_img(cv::Size(config.depthWidth, config.depthHeight), CV_8UC3, (void *) depth_frame,
                              cv::Mat::AUTO_STEP);
        cv::Mat depth_rgb_img;
        cv::cvtColor(depth_bgr_img, depth_rgb_img, cv::COLOR_RGB2BGR);
        if (depth_rgb_img.empty()) {
            cout << "\ndepth is empty " << endl;
            continue;
        }

        cv::Mat color_gry_img, color_cny_img;
        cv::cvtColor(color_rgb_img, color_gry_img, cv::COLOR_RGB2GRAY);
        cv::Canny(color_gry_img, color_cny_img, 10, 100, 3, true);

        if (color_gry_img.empty()) {
            cout << "\ngrey is empty " << endl;
            continue;
        }

        if (color_cny_img.empty()) {
            cout << "\ncanny is empty " << endl;
            continue;
        }
        //image show
        cv::imshow(color_wd.id, color_rgb_img);
        cv::imshow(depth_wd.id, depth_rgb_img);
        cv::imshow(grey_wd.id, color_gry_img);
        cv::imshow(canny_wd.id, color_cny_img);

        char input_key = cv::waitKey(30);
        color_palette_handle(input_key, count);
    }
    cout << "\nstop color streaming... " << endl;
    //destroy and release
    cv::destroyAllWindows();
    if (color_frame)
        delete color_frame;
    if (color_frame0)
        delete color_frame0;
    if (depth_frame)
        delete depth_frame;
    if (zdTable)
        delete zdTable;
    cout << "\nclose_device:"<< endl;
    //close_device();
	while (1) {
		Sleep(1);
		if (!device) {
			printf("doesn't need to CloseDevice()!\n");
			break;

		}
		device->closeStream();
		break;
	}
    cout << "\nrelease_device:"<< endl;
    //release_device();
	if (nullptr != device.get()) {
		delete device.get();
		device = nullptr;

	}
	if (nullptr != eYs3DSystem.get()) {
		delete eYs3DSystem.get();
		eYs3DSystem == nullptr;
	}
}

void face_detect() {
    cout << "\n\nface_detect item\n"<< endl;
    cv::CascadeClassifier face_cascade;
    cv::CascadeClassifier eyes_cascade;
    //Load the cascades
    if(!face_cascade.load(FACE_CASCADE)) {
        cout << "\nCannot load FACE_CASCADE!"<< endl;
        return ;
    }
    cout << "\n\nload face_cascade\n"<< endl;    
    if(!eyes_cascade.load(EYES_CASCADE)) {
        cout << "\nCannot load EYES_CASCADE!"<< endl;
        return ;
    }
    cout << "\n\nload eyes_cascade\n"<< endl;    
    
    //prepare window
    window_display color_wd = {"color_wd", 640, 360, 0, 0};
    window_display depth_wd = {"depth_wd", 640, 360, 720, 0};

    cv::namedWindow(color_wd.id, cv::WINDOW_NORMAL);
    cv::resizeWindow(color_wd.id, color_wd.width, color_wd.height);
    cv::moveWindow(color_wd.id, color_wd.x, color_wd.y);
    cv::namedWindow(depth_wd.id, cv::WINDOW_NORMAL);
    cv::resizeWindow(depth_wd.id, depth_wd.width, depth_wd.height);
    cv::moveWindow(depth_wd.id, depth_wd.x, depth_wd.y);

	LOG_INFO(LOG_TAG, "Starting EYS3DSystem...");
	eYs3DSystem = EYS3DSystem::getEYS3DSystem();
    device = eYs3DSystem->getCameraDevice(0);
	if(!device)    {
        LOG_INFO(LOG_TAG, "Unable to find any camera devices...");
        exit(-1);
    }
	config_mode();
    printf("\ncolorFormat %d, colorWidth:%d, colorHeight:%d, fps:%d, depthWidth:%d, depthHeight:%d, videoMode:%d\n",
           config.colorFormat, config.colorWidth, config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode);
    //prepare color, depth frame
    int color_size = config.colorWidth * config.colorHeight * 3;
    BYTE* color_frame0 = new BYTE [color_size];
    //BYTE* color_frame = new BYTE [color_size];
    if(color_frame == nullptr)
    	color_frame = new BYTE [color_size];
    cout << "\ncolor_frame size: " << color_size << endl;
    //int depth_size = config.depthWidth * config.depthHeight * 8;
    int depth_size = config.depthWidth * config.depthHeight * 3;
    //BYTE* depth_frame = new BYTE [depth_size];
    if(depth_frame == nullptr)
    	depth_frame = new BYTE [depth_size];
    cout << "\nget_depth_handler size: " << depth_size << endl;
    if(zdTable == nullptr)
    	zdTable = new int [depth_size];

    //eSPDI camera open
    int ret;
    //ret = init_device();
    //cout << "\ninit_device:"<< ret << endl;
    //ret = open_device(config);
	printf("\n\nEnabling device stream...\n");
#if 0
	pipeline = device->initStreamFS((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0);
	if(pipeline != nullptr){
		base::async([&]() { frameset_reader(pipeline.get()); });
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
		updateColorDepth = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
#else
	ret = device->initStream((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0, // rectifyLogIndex
	         color_image_callback,
	         depth_image_callback,
	         nullptr);
	if(ret == ETronDI_OK){
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
	}
    cout << "\nopen_device:"<< ret << endl;
#endif

    //ret = setupIR(0);
    device->setupIR(0);
    cout << "\nsetupIR:"<< ret << endl;

    cout << "\ninput Esc key to stop stream"<< endl;
    bool draw_rectangle = false;
    bool draw_eye = false;
#ifdef SUPPORT_QT_OPENGL
#ifndef WIN32
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_VISIBLE) >= 1
        && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_VISIBLE) >= 1) {
#else
    while(cv::waitKey(30) != 27){
#endif        
#else
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
          && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_AUTOSIZE) >= 0) {
#endif

        //eSPDI camera get frame
        //get_color_frame(color_frame);
		//ret = device->readColorFrame((BYTE*)color_frame, (uint64_t)2 * config.colorWidth * config.colorHeight, (uint64_t *)&gColorImgSize, (uint32_t *)&gColorSerial);
        //int size=0, type;
        //ret = tjpeg2yuv(color_frame,2 * config.colorWidth * config.colorHeight,&color_frame0, &size,&type);
        //ret = tyuv2rgb(color_frame0,size, config.colorWidth, config.colorHeight, type,&color_frame,&size);
        //get_depth_frame(depth_frame, depth_size, 0);
		//ret = device->readDepthFrame((BYTE*)depth_frame, (uint64_t)depth_size, (uint64_t *)&gDepthImgSize, (uint32_t *)&gDepthSerial);
		WaitForSingleObject(updateColorDepth, INFINITE);
        ResetEvent(updateColorDepth);
        //cvt to MAT
        //cv::Mat color_bgr_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, (void*)color_frame, cv::Mat::AUTO_STEP);
        cv::Mat color_rgb_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, (void*)color_frame, cv::Mat::AUTO_STEP);
        //cv::Mat color_rgb_img;
        //cv::cvtColor(color_bgr_img, color_rgb_img, cv::COLOR_RGB2BGR);
        if (color_rgb_img.empty()) {
            cout << "\ncolor is empty " << endl;
            continue;
        }
        cv::Mat depth_bgr_img(cv::Size(config.depthWidth, config.depthHeight), CV_8UC3, (void *) depth_frame,
                              cv::Mat::AUTO_STEP);
        cv::Mat depth_rgb_img;
        cv::cvtColor(depth_bgr_img, depth_rgb_img, cv::COLOR_RGB2BGR);
        if (depth_rgb_img.empty()) {
            cout << "\ndepth is empty " << endl;
            continue;
        }

        std::vector<cv::Rect> faces;
        cv::Mat color_gry_img;
        cv::cvtColor(color_rgb_img, color_gry_img, cv::COLOR_RGB2GRAY);
        equalizeHist(color_gry_img, color_gry_img);

        if (color_gry_img.empty()) {
            cout << "\ngrey is empty " << endl;
            continue;
        }

        //-- Detect faces
        face_cascade.detectMultiScale(color_gry_img, faces, 1.2, 5,
            0 | cv::CASCADE_SCALE_IMAGE, cv::Size(50, 50) );

        for ( size_t i = 0; i < faces.size(); i++ ) {
            //find face center
            if (!draw_rectangle) {
                cv::Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
                //draw ellipse to show face
                ellipse(color_rgb_img, center, cv::Size(faces[i].width / 2, faces[i].height / 2),
                        0, 0, 360, cv::Scalar(255, 0, 0),
                        4, 8, 0);
            } else {
                rectangle(color_rgb_img, faces[i], cv::Scalar(255, 0, 0), 4, 8, 2);
            }


            cv::Mat faceROI = color_gry_img(faces[i]);
            std::vector<cv::Rect> eyes;

            //-- In each face, detect eyes
            eyes_cascade.detectMultiScale( faceROI, eyes, 1.2, 5,
                0 | cv::CASCADE_SCALE_IMAGE, cv::Size(50, 50) );

            for (size_t j = 0; j < eyes.size(); j++ ) {
                if (draw_eye) {
                    //find eye center
                    cv::Point eye_center(faces[i].x + eyes[j].x + eyes[j].width / 2,
                                         faces[i].y + eyes[j].y + eyes[j].height / 2);
                    //draw circle to show eye
                    int radius = cvRound((eyes[j].width + eyes[j].height) * 0.25);
                    circle(color_rgb_img, eye_center, radius, cv::Scalar(255, 0, 0),
                           4, 8, 0);
                }
            }
        }

        //image show
        cv::imshow(color_wd.id, color_rgb_img);
        cv::imshow(depth_wd.id, depth_rgb_img);
        //if(faces.size() > 0){
        //	cv::imwrite("out_img/face_detect.jpg", color_rgb_img);
        //}

        char c = cv::waitKey(30);
        if (c == 27) {
            break;
        }
    }
    cout << "\nstop color streaming... " << endl;
    //destroy and release
    cv::destroyAllWindows();
    if (color_frame)
        delete color_frame;
    if (color_frame0)
        delete color_frame0;
    if (depth_frame)
        delete depth_frame;
    if (zdTable)
        delete zdTable;
    cout << "\nclose_device:"<< endl;
    //close_device();
	while (1) {
		Sleep(1);
		if (!device) {
			printf("doesn't need to CloseDevice()!\n");
			break;

		}
		device->closeStream();
		break;
	}
    cout << "\nrelease_device:"<< endl;
    //release_device();
	if (nullptr != device.get()) {
		delete device.get();
		device = nullptr;

	}
	if (nullptr != eYs3DSystem.get()) {
		delete eYs3DSystem.get();
		eYs3DSystem == nullptr;
	}
}

void face_mask_detect() {
    cout << "\n\nface_mask_detect item\n"<< endl;
    cv::CascadeClassifier face_cascade;
    cv::CascadeClassifier nose_cascade;

    //Load the cascades
    if(!face_cascade.load(FACE_CASCADE)) {
        cout << "\nCannot load FACE_CASCADE!"<< endl;
        return ;
    }
    
    if(!nose_cascade.load(NOSE_CASCADE)) {
        cout << "\nCannot load NOSE_CASCADE!"<< endl;
        return ;
    }

    //prepare window
    window_display color_wd = {"color_wd", 640, 360, 0, 0};
    window_display depth_wd = {"depth_wd", 640, 360, 720, 0};

    cv::namedWindow(color_wd.id, cv::WINDOW_NORMAL);
    cv::resizeWindow(color_wd.id, color_wd.width, color_wd.height);
    cv::moveWindow(color_wd.id, color_wd.x, color_wd.y);
    cv::namedWindow(depth_wd.id, cv::WINDOW_NORMAL);
    cv::resizeWindow(depth_wd.id, depth_wd.width, depth_wd.height);
    cv::moveWindow(depth_wd.id, depth_wd.x, depth_wd.y);

	LOG_INFO(LOG_TAG, "Starting EYS3DSystem...");
	eYs3DSystem = EYS3DSystem::getEYS3DSystem();
    device = eYs3DSystem->getCameraDevice(0);
	if(!device)    {
        LOG_INFO(LOG_TAG, "Unable to find any camera devices...");
        exit(-1);
    }
	config_mode();
    printf("\ncolorFormat %d, colorWidth:%d, colorHeight:%d, fps:%d, depthWidth:%d, depthHeight:%d, videoMode:%d\n",
           config.colorFormat, config.colorWidth, config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode);
    //prepare color, depth frame
    int color_size = config.colorWidth * config.colorHeight * 3;
    BYTE* color_frame0 = new BYTE [color_size];
    //BYTE* color_frame = new BYTE [color_size];
    if(color_frame == nullptr)
    	color_frame = new BYTE [color_size];
    cout << "\ncolor_frame size: " << color_size << endl;
    //int depth_size = config.depthWidth * config.depthHeight * 8;
    int depth_size = config.depthWidth * config.depthHeight * 3;
    //BYTE* depth_frame = new BYTE [depth_size];
    if(depth_frame == nullptr)
    	depth_frame = new BYTE [depth_size];
    cout << "\nget_depth_handler size: " << depth_size << endl;
    if(zdTable == nullptr)
    	zdTable = new int [depth_size];

    //eSPDI camera open
    int ret;
    //ret = init_device();
    //cout << "\ninit_device:"<< ret << endl;
    //ret = open_device(config);
	printf("\n\nEnabling device stream...\n");
#if 0
	pipeline = device->initStreamFS((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0);
	if(pipeline != nullptr){
		base::async([&]() { frameset_reader(pipeline.get()); });
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
		updateColorDepth = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
#else
	ret = device->initStream((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0, // rectifyLogIndex
	         color_image_callback,
	         depth_image_callback,
	         nullptr);
	if(ret == ETronDI_OK){
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
	}
    cout << "\nopen_device:"<< ret << endl;
#endif

    //ret = setupIR(0);
    device->setupIR(0);
    cout << "\nsetupIR:"<< ret << endl;

    cout << "\ninput Esc key to stop stream"<< endl;
    bool draw_rectangle = false;
    bool draw_nose = false;
#ifdef SUPPORT_QT_OPENGL
#ifndef WIN32
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_VISIBLE) >= 1
        && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_VISIBLE) >= 1) {
#else
    while(cv::waitKey(30) != 27){
#endif
#else
    while(cv::getWindowProperty(color_wd.id, cv::WND_PROP_AUTOSIZE) >= 0
          && cv::getWindowProperty(depth_wd.id, cv::WND_PROP_AUTOSIZE) >= 0) {
#endif
        //eSPDI camera get frame
        //get_color_frame(color_frame);
		//ret = device->readColorFrame((BYTE*)color_frame, (uint64_t)2 * config.colorWidth * config.colorHeight, (uint64_t *)&gColorImgSize, (uint32_t *)&gColorSerial);
        //int size=0, type;
        //ret = tjpeg2yuv(color_frame,2 * config.colorWidth * config.colorHeight,&color_frame0, &size,&type);
        //ret = tyuv2rgb(color_frame0,size, config.colorWidth, config.colorHeight, type,&color_frame,&size);
        //get_depth_frame(depth_frame, depth_size, 0);
		//ret = device->readDepthFrame((BYTE*)depth_frame, (uint64_t)depth_size, (uint64_t *)&gDepthImgSize, (uint32_t *)&gDepthSerial);
		WaitForSingleObject(updateColorDepth, INFINITE);
        ResetEvent(updateColorDepth);
        //cvt to MAT
        //cv::Mat color_bgr_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, (void*)color_frame, cv::Mat::AUTO_STEP);
        cv::Mat color_rgb_img(cv::Size(config.colorWidth, config.colorHeight), CV_8UC3, (void*)color_frame, cv::Mat::AUTO_STEP);
        //cv::Mat color_rgb_img;
        //cv::cvtColor(color_bgr_img, color_rgb_img, cv::COLOR_RGB2BGR);
        if (color_rgb_img.empty()) {
            cout << "\ncolor is empty " << endl;
            continue;
        }
        cv::Mat depth_bgr_img(cv::Size(config.depthWidth, config.depthHeight), CV_8UC3, (void *) depth_frame,
                              cv::Mat::AUTO_STEP);
        cv::Mat depth_rgb_img;
        cv::cvtColor(depth_bgr_img, depth_rgb_img, cv::COLOR_RGB2BGR);
        if (depth_rgb_img.empty()) {
            cout << "\ndepth is empty " << endl;
            continue;
        }

        std::vector<cv::Rect> faces;
        cv::Mat color_gry_img;
        cv::cvtColor(color_rgb_img, color_gry_img, cv::COLOR_RGB2GRAY);
        equalizeHist(color_gry_img, color_gry_img);

        if (color_gry_img.empty()) {
            cout << "\ngrey is empty " << endl;
            continue;
        }

        //-- Detect faces
        face_cascade.detectMultiScale(color_gry_img, faces, 1.3, 5,
                                      0 | cv::CASCADE_SCALE_IMAGE, cv::Size(50, 50) );

        for ( size_t i = 0; i < faces.size(); i++ ) {

            cv::Mat faceROI = color_gry_img(faces[i]);
            std::vector<cv::Rect> nose;
            std::vector<cv::Rect> mouth;


            //-- In each face, detect nose
            nose_cascade.detectMultiScale( faceROI, nose, 1.1, 5,
                                           0 | cv::CASCADE_SCALE_IMAGE, cv::Size(50, 50) );

            for (size_t j = 0; j < nose.size(); j++ ) {
                if (draw_nose) {
                    //find nose center
                    cv::Point nose_center(faces[i].x + nose[j].x + nose[j].width / 2,
                                          faces[i].y + nose[j].y + nose[j].height / 2);
                    //draw circle to show nose
                    int radius = cvRound((nose[j].width + nose[j].height) * 0.25);
                    circle(color_rgb_img, nose_center, radius, cv::Scalar(0, 0, 255),
                           4, 8, 0);
                }
            }

            cv::Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
            cv::Point center_text(faces[i].x + 20, faces[i].y  - 20);

            if (nose.size() > 0) {
                //find face center
                if (!draw_rectangle) {
                    //draw ellipse to show face
                    ellipse(color_rgb_img, center, cv::Size(faces[i].width / 2, faces[i].height / 2),
                        0, 0, 360, cv::Scalar(0, 0, 255),
                            4, 8, 0);
                    cv::putText(color_rgb_img, "no_mask", center_text, cv::FONT_HERSHEY_SIMPLEX,
                        1, cv::Scalar(0, 0, 255), 2);

                } else {
                    cv::rectangle(color_rgb_img, center,
                              cv::Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height),
                              cv::Scalar(0, 0, 255), 4, 8, 2);
                }
            } else {
                if (!draw_rectangle) {
                    //draw ellipse to show face
                    ellipse(color_rgb_img, center, cv::Size(faces[i].width / 2, faces[i].height / 2),
                            0, 0, 360, cv::Scalar(0, 255, 0),
                            4, 8, 0);
                    cv::putText(color_rgb_img, "have_mask", center_text, cv::FONT_HERSHEY_SIMPLEX,
                                1, cv::Scalar(0, 255, 0), 2);
                } else {
                    cv::rectangle(color_rgb_img, center,
                         cv::Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height),
                        cv::Scalar(0, 255, 0), 4, 8, 2);
                }
            }
        }

        //image show
        cv::imshow(color_wd.id, color_rgb_img);
        cv::imshow(depth_wd.id, depth_rgb_img);
        //if(faces.size() > 0){
        //	cv::imwrite("out_img/face_mask_detect.jpg", color_rgb_img);
        //}

        char c = cv::waitKey(30);
        if (c == 27) {
            break;
        }
    }
    cout << "\nstop color streaming... " << endl;
    //destroy and release
    cv::destroyAllWindows();
    if (color_frame)
        delete color_frame;
    if (color_frame0)
        delete color_frame0;
    if (depth_frame)
        delete depth_frame;
    if (zdTable)
        delete zdTable;
    cout << "\nclose_device:"<< endl;
    //close_device();
	while (1) {
		Sleep(1);
		if (!device) {
			printf("doesn't need to CloseDevice()!\n");
			break;

		}
		device->closeStream();
		break;
	}
    cout << "\nrelease_device:"<< endl;
    //release_device();
	if (nullptr != device.get()) {
		delete device.get();
		device = nullptr;

	}
	if (nullptr != eYs3DSystem.get()) {
		delete eYs3DSystem.get();
		eYs3DSystem == nullptr;
	}
}

void show_settings_content(window_display &settings_wd) {
    cout << "\n\nshow_settings_content:"<< endl;
    //create IR Trackbar
    //int ir_value= get_IR_min_value();
    libeYs3D::devices::IRProperty property = device->getIRProperty();
    int ir_value = property.getIRMin();
    int ir_max = property.getIRMax();
    //cv::createTrackbar(SETTINGS_IR, settings_wd.id, &ir_value, get_IR_max_value(), ir_callback);
    cv::createTrackbar(SETTINGS_IR, settings_wd.id, &ir_value, ir_max, ir_callback);
    cv::setTrackbarPos(SETTINGS_IR, settings_wd.id, 3);
    //create AE Trackbar
    cv::createTrackbar(SETTINGS_AE, settings_wd.id, 0, 1, ae_callback);
    cv::setTrackbarPos(SETTINGS_AE, settings_wd.id, 1);
    //create AWB Trackbar
    cv::createTrackbar(SETTINGS_AWB, settings_wd.id, 0, 1, awb_callback);
    cv::setTrackbarPos(SETTINGS_AWB, settings_wd.id, 1);
}

void point_cloud_view() {
    cout << "\n\npoint_cloud_view item\n"<< endl;
	LOG_INFO(LOG_TAG, "Starting EYS3DSystem...");
	eYs3DSystem = EYS3DSystem::getEYS3DSystem();
    device = eYs3DSystem->getCameraDevice(0);
	if(!device)    {
        LOG_INFO(LOG_TAG, "Unable to find any camera devices...");
        exit(-1);
    }
	config_mode();
    printf("\ncolorFormat %d, colorWidth:%d, colorHeight:%d, fps:%d, depthWidth:%d, depthHeight:%d, videoMode:%d\n",
           config.colorFormat, config.colorWidth, config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode);
    //prepare color, depth frame
    int color_size = config.colorWidth * config.colorHeight * 3;
    BYTE* color_frame0 = new BYTE [color_size];
    //BYTE* color_frame = new BYTE [color_size];
    if(color_frame == nullptr)
    	color_frame = new BYTE [color_size];
    cout << "\ncolor_frame size: " << color_size << endl;
    int depth_size = config.depthWidth * config.depthHeight * 2; // raw data
    //BYTE* depth_frame = new BYTE [depth_size];
    if(depth_frame == nullptr)
    	depth_frame = new BYTE [depth_size];
    cout << "\nget_depth_handler size: " << depth_size << endl;
    int pcl_depth_size = config.depthWidth * config.depthHeight * 3 * sizeof(float);
    //int pcl_color_size = config.colorWidth * config.colorHeight << 2 * sizeof(BYTE);
    int pcl_color_size = config.colorWidth * config.colorHeight * 3 * sizeof(BYTE); 
	//BYTE *color_frame_out = new BYTE[pcl_color_size];
	if(color_frame_out == nullptr)
		color_frame_out = new BYTE[pcl_color_size];
	//float *depth_frame_out = new float[pcl_depth_size];
	if(depth_frame_out == nullptr)
		depth_frame_out = new float[pcl_depth_size];
    if(zdTable == nullptr)
    	zdTable = new int [depth_size];
	updatePCHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    //eSPDI camera open
    int ret;
    //ret = init_device();
    //cout << "\ninit_device:"<< ret << endl;
    //ret = open_device(config);
	printf("\n\nEnabling device stream...\n");
	ret = device->initStream((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0, // rectifyLogIndex
	         nullptr,
	         nullptr,
	         pc_frame_callback);
	if(ret == ETronDI_OK){
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
	}
    cout << "\nopen_device:"<< ret << endl;

    char item_input;
    cout << "\nTurn on IR (Y/N):" << endl;
    cin >> item_input;
    while(true) {
        switch (item_input) {
            case 'Y':
            case 'y':
                //setupIR(3);
    			device->setupIR(3);
                break;
            case 'N':
            case 'n':
                //setupIR(0);
    			device->setupIR(0);
                break;
            default:
                cout << "That's not a choice."<< endl;
        }
        if (item_input != 'Y' && item_input != 'y'
            && item_input != 'N' && item_input != 'n') {
            cout << "\nturn on IR (Y/N):" << endl;
            cin >> item_input;
        } else
            break;
    }

#ifdef SUPPORT_VIZ
    cv::viz::Viz3d window("window");
#endif
    bool pcl_gpu = true;
#ifdef SUPPORT_VIZ
    while(!window.wasStopped())
#else
	while(1)
#endif
    {
        //eSPDI camera get frame
        //ret = get_color_frame(color_frame);
		//ret = device->readColorFrame((BYTE*)color_frame, (uint64_t)2 * config.colorWidth * config.colorHeight, (uint64_t *)&gColorImgSize, (uint32_t *)&gColorSerial);
        //if (ret != 0) {
        //    cout << "\nget_color_frame failed:" << ret << endl;
        //    continue;
        //}
        //int size=0, type;
        //ret = tjpeg2yuv(color_frame,2 * config.colorWidth * config.colorHeight,&color_frame0, &size,&type);
        //ret = tyuv2rgb(color_frame0,size, config.colorWidth, config.colorHeight, type,&color_frame,&size);
        
        //ret = get_depth_frame(depth_frame, depth_size, 1); // raw data
		//ret = device->readDepthFrame((BYTE*)depth_frame, (uint64_t)depth_size, (uint64_t *)&gDepthImgSize, (uint32_t *)&gDepthSerial);
        //if (ret != 0) {
        //    cout << "\nget_depth_frame failed:" << ret << endl;
        //    continue;
        //}
        int count = 0;
        vector<CloudPoint> cloudPoint;
        if (ret == 0) {
#if 1
            /*if (pcl_gpu) {
                cloudPoint = PyGeneratePointCloud_GPU(depth_frame, config.depthWidth, config.depthHeight,
                                                                         color_frame, config.colorWidth, config.colorHeight);
            } else {
                cloudPoint = PyGeneratePointCloud(depth_frame, config.depthWidth, config.depthHeight,
                                                                         color_frame, config.colorWidth, config.colorHeight,
                                                                         false);
            }*/
            /*ret = generate_point_cloud_gpu(color_frame, depth_frame,
                                           color_frame_out, &pcl_color_size,
                                           depth_frame_out, &pcl_depth_size);*/
			std::vector<float> m_pointCloudDepth;
		    std::vector<BYTE> m_pointCloudColor;
		    if (m_pointCloudDepth.size() != (size_t)(config.colorWidth * config.colorHeight * 3)) m_pointCloudDepth.resize(config.colorWidth * config.colorHeight * 3);
		    else                                                          std::fill(m_pointCloudDepth.begin(), m_pointCloudDepth.end(), 0.0f);

		    if (m_pointCloudColor.size() != (size_t)(config.colorWidth * config.colorHeight * 3)) m_pointCloudColor.resize(config.colorWidth * config.colorHeight * 3);
		    else                                                          std::fill(m_pointCloudColor.begin(), m_pointCloudColor.end(), 0);

			//ret = device->readPCFrame(color_frame, depth_frame, color_frame_out, depth_frame_out);
			WaitForSingleObject(updatePCHandle, INFINITE);
			ResetEvent(updatePCHandle);
            // prevent assertion fail
            struct CloudPoint cloudpoint = {0.0, 0.0, 0.0, 0, 0, 0};
            cloudPoint.push_back(cloudpoint);
            for (int i = 0 ; i < pcl_depth_size; i+=3){
		        if (depth_frame_out[i] != 0 && depth_frame_out[i+1] != 0 && depth_frame_out[i+2] !=0){
		        	struct CloudPoint cloudpoint = { depth_frame_out[i], depth_frame_out[i+ 1], depth_frame_out[i+2], color_frame_out[i], color_frame_out[i+ 1], color_frame_out[i+2]};
		        	cloudPoint.push_back(cloudpoint);
		        }
			}
            if (count <= 0) {
                PlyWriter::writePly(cloudPoint, "123.ply");
                count++;
            }
#endif
            vector<cv::Point3f> m3Dpoints;
            vector<cv::Vec3b> m3Dbgr;
            for (int i = 0; i < cloudPoint.size() ; i++) {
                cv::Point3f point3f;
                point3f.x = cloudPoint[i].x;
                point3f.y = (-cloudPoint[i].y);
                point3f.z = (-cloudPoint[i].z);
                m3Dpoints.push_back(point3f);

                cv::Vec3b pix_rgb;
                pix_rgb[0] = cloudPoint[i].b;
                pix_rgb[1] = cloudPoint[i].g;
                pix_rgb[2] = cloudPoint[i].r;
                m3Dbgr.push_back(pix_rgb);

            }
            cv::Mat point_cloud { m3Dpoints };
            cv::Mat color_cloud { m3Dbgr };
#ifdef SUPPORT_VIZ
            cv::viz::WCloud cloud_widget(point_cloud, color_cloud);
            window.showWidget( "window", cloud_widget );
            window.spinOnce();
#endif
        }
    }
    cout << "\nstop color streaming... " << endl;
    //destroy and release
    cv::destroyAllWindows();
    if (color_frame)
        delete color_frame;
    if (color_frame0)
        delete color_frame0;
    if (depth_frame)
        delete depth_frame;
    if (zdTable)
        delete zdTable;
    cout << "\nclose_device:"<< endl;
    //close_device();
	while (1) {
		Sleep(1);
		if (!device) {
			printf("doesn't need to CloseDevice()!\n");
			break;

		}
		device->closeStream();
		break;
	}
    cout << "\nrelease_device:"<< endl;
    //release_device();
	if (nullptr != device.get()) {
		delete device.get();
		device = nullptr;

	}
	if (nullptr != eYs3DSystem.get()) {
		delete eYs3DSystem.get();
		eYs3DSystem == nullptr;
	}
}

void point_cloud_view_with_opengl() {
    cout << "\n\npoint cloud view with opengl item\n"<< endl;
#ifdef SUPPORT_QT_OPENGL

	LOG_INFO(LOG_TAG, "Starting EYS3DSystem...");
	eYs3DSystem = EYS3DSystem::getEYS3DSystem();
    device = eYs3DSystem->getCameraDevice(0);
	if(!device)    {
        LOG_INFO(LOG_TAG, "Unable to find any camera devices...");
        exit(-1);
    }
	config_mode();
    printf("\ncolorFormat %d, colorWidth:%d, colorHeight:%d, fps:%d, depthWidth:%d, depthHeight:%d, videoMode:%d\n",
           config.colorFormat, config.colorWidth, config.colorHeight, config.fps, config.depthWidth, config.depthHeight, config.videoMode);
    //prepare color, depth frame
    int color_size = config.colorWidth * config.colorHeight * 3;
    BYTE* color_frame0 = new BYTE [color_size];
    //BYTE *color_frame = new BYTE[color_size];
    if(color_frame == nullptr)
    	color_frame = new BYTE[color_size];
    //BYTE *color_frame_out = new BYTE[color_size];
    cout << "\ncolor_frame size: " << color_size << endl;
    //int depth_size = config.depthWidth * config.depthHeight * 8; // raw data
    int depth_size = config.depthWidth * config.depthHeight * 2; // raw data
    //BYTE *depth_frame = new BYTE[depth_size];
    if(depth_frame == nullptr)
    	depth_frame = new BYTE[depth_size];
    //float *depth_frame_out = new float[depth_size * sizeof(float)];
    cout << "\nget_depth_handler size: " << depth_size << endl;
    int pcl_depth_size = config.depthWidth * config.depthHeight * 3 * sizeof(float);
    //int pcl_color_size = config.colorWidth * config.colorHeight << 2 * sizeof(BYTE);
    int pcl_color_size = config.colorWidth * config.colorHeight * 3 * sizeof(BYTE);
	//BYTE *color_frame_out = new BYTE[pcl_color_size];
	if(color_frame_out == nullptr)
		color_frame_out = new BYTE[pcl_color_size];
	//float *depth_frame_out = new float[pcl_depth_size];
	if(depth_frame_out == nullptr)
		depth_frame_out = new float[pcl_depth_size];
    if(zdTable == nullptr)
    	zdTable = new int [depth_size];
	updatePCHandle = CreateEvent(NULL, TRUE, FALSE, NULL);

    //eSPDI camera open
    int ret;
    //ret = init_device();
    //cout << "\ninit_device:"<< ret << endl;
    //ret = open_device(config);
	printf("\n\nEnabling device stream...\n");
	ret = device->initStream((libeYs3D::video::COLOR_RAW_DATA_TYPE)config.colorFormat,
	         config.colorWidth, config.colorHeight, config.fps, 
	         depth_raw_data_type,
	         config.depthWidth, config.depthHeight,
	         (DEPTH_TRANSFER_CTRL)config.videoMode,
	         IMAGE_SN_SYNC,
	         0, // rectifyLogIndex
	         nullptr,
	         nullptr,
	         pc_frame_callback);
	if(ret == ETronDI_OK){
		device->enableStream();
		printf("\n\nDevice stream enabled\n");
	}
    cout << "\nopen_device:" << ret << endl;

    char item_input;
    cout << "\nTurn on IR (Y/N):" << endl;
    cin >> item_input;
    while (true)
    {
        switch (item_input)
        {
        case 'Y':
        case 'y':
            //setupIR(3);
    		device->setupIR(3);
            break;
        case 'N':
        case 'n':
            //setupIR(0);
    		device->setupIR(0);
            break;
        default:
            cout << "That's not a choice." << endl;
        }
        if (item_input != 'Y' && item_input != 'y' && item_input != 'N' && item_input != 'n')
        {
            cout << "\nturn on IR (Y/N):" << endl;
            cin >> item_input;
        }
        else
            break;
    }
    window_display point_cloud_wd = {"Point Cloud with OpenGL", 1280, 720, 0, 0};
    cv::namedWindow(point_cloud_wd.id, cv::WINDOW_OPENGL);
    cv::resizeWindow(point_cloud_wd.id, 1280, 720);
    cv::setOpenGlContext(point_cloud_wd.id);
    cv::setOpenGlDrawCallback(point_cloud_wd.id, onDraw, NULL);

    char k;
    bool continueStream = true;

    initGL();
    cv::createTrackbar( "X-rotation", point_cloud_wd.id, &rotx, 360, on_trackbar, (void*)&point_cloud_wd);
    cv::createTrackbar( "Y-rotation", point_cloud_wd.id, &roty, 360, on_trackbar, (void*)&point_cloud_wd);
#ifndef WIN32
    while (cv::getWindowProperty(point_cloud_wd.id, cv::WND_PROP_VISIBLE) >= 1)
    {
#else
    while(cv::waitKey(30) != 27){
#endif
    	
        //eSPDI camera get frame
        //ret = get_color_frame(color_frame);
		//ret = device->readColorFrame((BYTE*)color_frame, (uint64_t)2 * config.colorWidth * config.colorHeight, (uint64_t *)&gColorImgSize, (uint32_t *)&gColorSerial);
        //int size=0, type;
        //ret = tjpeg2yuv(color_frame,2 * config.colorWidth * config.colorHeight,&color_frame0, &size,&type);
        //ret = tyuv2rgb(color_frame0,size, config.colorWidth, config.colorHeight, type,&color_frame,&size);
        //if (ret != 0)
        //{
        //    cout << "\nget_color_frame failed:" << ret << endl;
        //    continue;
        //}
        //ret = get_depth_frame(depth_frame, depth_size, 1); // raw data
		//ret = device->readDepthFrame((BYTE*)depth_frame, (uint64_t)depth_size, (uint64_t *)&gDepthImgSize, (uint32_t *)&gDepthSerial);
        //if (ret != 0)
        //{
        //    cout << "\nget_depth_frame failed:" << ret << endl;
        //    continue;
        //}
        int count = 0;
        vector<CloudPoint> cloudPoint;
        if (ret == 0)
        {
            /*if (1)
            {
                cloudPoint = PyGeneratePointCloud_GPU(depth_frame, config.depthWidth, config.depthHeight,
                                                      color_frame, config.colorWidth, config.colorHeight);
            }
            else
            {
                cloudPoint = PyGeneratePointCloud(depth_frame, config.depthWidth, config.depthHeight,
                                                  color_frame, config.colorWidth, config.colorHeight,
                                                  false);
            }

            if (count <= 0)
            {
                PlyWriter::writePly(cloudPoint, "123.ply");
                count++;
            }*/

            /*ret = generate_point_cloud_gpu(color_frame, depth_frame,
                                           color_frame_out, &pcl_color_size,
                                           depth_frame_out, &pcl_depth_size);*/
			std::vector<float> m_pointCloudDepth;
		    std::vector<BYTE> m_pointCloudColor;
		    if (m_pointCloudDepth.size() != (size_t)(config.colorWidth * config.colorHeight * 3)) m_pointCloudDepth.resize(config.colorWidth * config.colorHeight * 3);
		    else                                                          std::fill(m_pointCloudDepth.begin(), m_pointCloudDepth.end(), 0.0f);

		    if (m_pointCloudColor.size() != (size_t)(config.colorWidth * config.colorHeight * 3)) m_pointCloudColor.resize(config.colorWidth * config.colorHeight * 3);
		    else                                                          std::fill(m_pointCloudColor.begin(), m_pointCloudColor.end(), 0);

			//ret = device->readPCFrame(color_frame, depth_frame, color_frame_out, depth_frame_out);

			WaitForSingleObject(updatePCHandle, INFINITE);
			ResetEvent(updatePCHandle);
            point_num = (pcl_depth_size / sizeof(float)) / 3;
            glBindBuffer(GL_ARRAY_BUFFER, gVerticesBuffer);
            glBufferData(GL_ARRAY_BUFFER, point_num * 3 * sizeof(float), &depth_frame_out[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, gColorBuffer);
            glBufferData(GL_ARRAY_BUFFER, point_num * 3 * sizeof(uchar), &color_frame_out[0], GL_STATIC_DRAW);
        }

        cv::updateWindow(point_cloud_wd.id);
        k = cv::waitKey(1);

        switch (k)
        {
        case 0x1b: //ESC key
            printf("Closing stream.\n");
            continueStream = false;
            break;
        }
    }

    //destroy and release
    cv::destroyAllWindows();

    if (color_frame) delete [] color_frame;
    if (color_frame_out) delete [] color_frame_out;

    if (depth_frame) delete [] depth_frame;
    if (depth_frame_out) delete [] depth_frame_out;

    if (zdTable)
        delete zdTable;

    cout << "\nclose_device:" << endl;
    //close_device();
	while (1) {
		Sleep(1);
		if (!device) {
			printf("doesn't need to CloseDevice()!\n");
			break;

		}
		device->closeStream();
		break;
	}
    cout << "\nrelease_device:" << endl;
    //release_device();
	if (nullptr != device.get()) {
		delete device.get();
		device = nullptr;

	}
	if (nullptr != eYs3DSystem.get()) {
		delete eYs3DSystem.get();
		eYs3DSystem == nullptr;
	}
#else
    cout << "Not support OpenGL!"<< endl;
#endif
}