///////////////////////////////////////////////////////////////////////////////
// FILE:          EVA_NDE_PerkinElmerFPD.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   The example implementation of the EVA_NDE_PerkinElmer camera.
//                Simulates generic digital camera and associated automated
//                microscope devices and enables testing of the rest of the
//                system without the need to connect to the actual hardware. 
//                
// AUTHOR:        Nenad Amodaj, nenad@amodaj.com, 06/08/2005
//
// COPYRIGHT:     University of California, San Francisco, 2006
// LICENSE:       This file is distributed under the BSD license.
//                License text is included with the source distribution.
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.
// CVS:           $Id: EVA_NDE_PerkinElmerFPD.cpp 10842 2013-04-24 01:21:05Z mark $
//

#include "EVA_NDE_PerkinElmerFPD.h"
#include <cstdio>
#include <string>
#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
#include "../../MMCore/Error.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#define MAX_SAMPLE_LENGTH 3000000
#define WAIT_FOR_TRIGGER_FAIL_COUNT 2
using namespace std;
const double CEVA_NDE_PerkinElmerFPD::nominalPixelSizeUm_ = 1.0;
double g_IntensityFactor_ = 1.0;

// External names used used by the rest of the system
// 
const char* g_CameraDeviceName = "PerkinElmerFPD";
const char* g_HubDeviceName = "PerkinElmerHub";

// constants for naming pixel types (allowed values of the "PixelType" property)
const char* g_PixelType_8bit = "8bit";
const char* g_PixelType_16bit = "16bit";

// constants for naming acquisition mode
const char* g_syncMode_3 = "Free Running";
const char* g_syncMode_2 = "External Triggered";
const char* g_syncMode_1 = "Internal Triggered";
const char* g_syncMode_0 = "Soft Triggered";

const char* g_FrameTime_TIMING_0 = "0-134ms";
const char* g_FrameTime_TIMING_1 = "200ms";
const char* g_FrameTime_TIMING_2 = "400ms";
const char* g_FrameTime_TIMING_3 = "800ms";
const char* g_FrameTime_TIMING_4 = "1600ms";
const char* g_FrameTime_TIMING_5 = "3200ms";
const char* g_FrameTime_TIMING_6 = "6400ms";
const char* g_FrameTime_TIMING_7 = "12800ms";

const char* g_TriggerMode_0 = "DataDeliveredOnDemand";
const char* g_TriggerMode_1 = "DataDeliveredOnDemandNoClearance";
const char* g_TriggerMode_2 = "Linewise";
const char* g_TriggerMode_3 = "Framewise";

const char* g_CalibrationMode_None = "None";
const char* g_CalibrationMode_Offset = "Offset";
const char* g_CalibrationMode_Gain = "Gain";
const char* g_CalibrationMode_MultiGain = "Multi-gain";
const char* g_CalibrationMode_PixelCorrection = "Pixel Correction";

const char* g_SnapMode_0 = "Raw";
const char* g_SnapMode_1 = "Offset-corrected";
const char* g_SnapMode_2 = "Gain/Offset-corrected";
const char* g_SnapMode_3 = "Multi-Gain/Offset-corrected";


const char* g_Keyword_Progress = "AcqProgress";

// set customized error code
#define OFFSET_CALIBRATION_FAILED     100
#define GAIN_CALIBRATION_FAILED     101
#define MULTI_GAIN_CALIBRATION_FAILED     102
#define PIXEL_CORRECTION_FAILED     103

// TODO: linux entry code


// show message in a non-blcoking mode
DWORD WINAPI CreateMessageBox(LPVOID lpParam) {
	MessageBox(NULL, (char*)lpParam, "Notice", MB_OK);
	return 0;
}
#define MessageBoxNonBlocking(msg)  if(initialized_) CreateThread(NULL, 0, &CreateMessageBox, msg, 0, NULL)
// windows DLL entry code
#ifdef WIN32
BOOL APIENTRY DllMain( HANDLE /*hModule*/, 
                      DWORD  ul_reason_for_call, 
                      LPVOID /*lpReserved*/
                      )
{
   switch (ul_reason_for_call)
   {
   case DLL_PROCESS_ATTACH:
   case DLL_THREAD_ATTACH:
   case DLL_THREAD_DETACH:
   case DLL_PROCESS_DETACH:
      break;
   }
   return TRUE;
}
#endif



///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////

/**
 * List all suppoerted hardware devices here
 * Do not discover devices at runtime.  To avoid warnings about missing DLLs, Micro-Manager
 * maintains a list of supported device (MMDeviceList.txt).  This list is generated using 
 * information supplied by this function, so runtime discovery will create problems.
 */
MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_CameraDeviceName, MM::CameraDevice, "EVA_NDE_PerkinElmer Camera");
   RegisterDevice("TransposeProcessor", MM::ImageProcessorDevice, "TransposeProcessor");
   RegisterDevice("ImageFlipX", MM::ImageProcessorDevice, "ImageFlipX");
   RegisterDevice("ImageFlipY", MM::ImageProcessorDevice, "ImageFlipY");
   RegisterDevice("MedianFilter", MM::ImageProcessorDevice, "MedianFilter");
   RegisterDevice(g_HubDeviceName, MM::HubDevice, "EVA_NDE_PerkinElmer Hub");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return NULL;

   // decide which device class to create based on the deviceName parameter
   if (strcmp(deviceName, g_CameraDeviceName) == 0)
   {
      // create camera
      return new CEVA_NDE_PerkinElmerFPD();
   }
   else if(strcmp(deviceName, "TransposeProcessor") == 0)
   {
      return new TransposeProcessor();
   }
   else if(strcmp(deviceName, "ImageFlipX") == 0)
   {
      return new ImageFlipX();
   }
   else if(strcmp(deviceName, "ImageFlipY") == 0)
   {
      return new ImageFlipY();
   }
   else if(strcmp(deviceName, "MedianFilter") == 0)
   {
      return new MedianFilter();
   }
   else if (strcmp(deviceName, g_HubDeviceName) == 0)
   {
	  return new EVA_NDE_PerkinElmerHub();
   }

   // ...supplied name not recognized
   return NULL;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

///////////////////////////////////////////////////////////////////////////////
// CEVA_NDE_PerkinElmerFPD implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
CEVA_NDE_PerkinElmerFPD *CEVA_NDE_PerkinElmerFPD::activeFPDins=NULL;

/**
* CEVA_NDE_PerkinElmerFPD constructor.
* Setup default all variables and create device properties required to exist
* before intialization. In this case, no such properties were required. All
* properties will be created in the Initialize() method.
*
* As a general guideline Micro-Manager devices do not access hardware in the
* the constructor. We should do as little as possible in the constructor and
* perform most of the initialization in the Initialize() method.
*/
CEVA_NDE_PerkinElmerFPD::CEVA_NDE_PerkinElmerFPD() :
   CCameraBase<CEVA_NDE_PerkinElmerFPD> (),
   initialized_(false),
   syncMode_(2), // internal_trigger
   bitDepth_(16),
   roiX_(0),
   roiY_(0),
   sequenceStartTime_(0),
   isSequenceable_(false),
   sequenceMaxLength_(100),
   sequenceRunning_(false),
   sequenceIndex_(0),
   binMode_(1),
   exposureMs_(10),
   triggerMode_(0),
   byteDepth_(2),
   frameTiming_(0),
   image_width(512),
   image_height(512),
   pEVA_NDE_PerkinElmerResourceLock_(0),
   triggerDevice_(""),
   stopOnOverflow_(false),
   timeout_(5000),
   hAcqDesc(NULL),
   selectedModel_(0),
   modelsCount_(0),
   hOutput(NULL),
	hInput(NULL),
	dwCharsWritten(0),
	pAcqBuffer(NULL),
	pOffsetBuffer(NULL),
	pBrightocBuffer(NULL),
	pGainSeqBuffer(NULL),
	pGainSeqMedBuffer(NULL),
	pGainBuffer(NULL),
	pPixelBuffer(NULL),
	pCorrList(NULL),
    progress_(0),
	acqFrameCount_(1),
	pixelCorrection_(true),
    offsetFrameCount_(20),
    gainFrameCount_(20),
    multiGainFrameCount_(20),
    multiGainSegmentCount_(2),
	pGainBufferInUse(NULL),
	pCorrListInUse(NULL),
	pOffsetBufferInUse(NULL)
{
   // call the base class method to set-up default error codes/messages
   InitializeDefaultErrorMessages();
   pEVA_NDE_PerkinElmerResourceLock_ = new MMThreadLock();
   thd_ = new MySequenceThread(this);
   
   SetErrorText(OFFSET_CALIBRATION_FAILED, "Offset calibration failed!");
   SetErrorText(GAIN_CALIBRATION_FAILED, "Gain calibration failed!"); 
   SetErrorText(MULTI_GAIN_CALIBRATION_FAILED, "Multi-gain calibration failed!"); 
   SetErrorText(PIXEL_CORRECTION_FAILED, "Pixel correction failed!"); 

   // parent ID display
   CreateHubIDProperty();
}

/**
* CEVA_NDE_PerkinElmerFPD destructor.
* If this device used as intended within the Micro-Manager system,
* Shutdown() will be always called before the destructor. But in any case
* we need to make sure that all resources are properly released even if
* Shutdown() was not called.
*/
CEVA_NDE_PerkinElmerFPD::~CEVA_NDE_PerkinElmerFPD()
{
   StopSequenceAcquisition();
   delete thd_;
   delete pEVA_NDE_PerkinElmerResourceLock_;
}

/**
* Obtains device name.
* Required by the MM::Device API.
*/
void CEVA_NDE_PerkinElmerFPD::GetName(char* name) const
{
   // Return the name used to referr to this device adapte
   CDeviceUtils::CopyLimitedString(name, g_CameraDeviceName);
}
/**
* Intializes the hardware.
* Required by the MM::Device API.
* Typically we access and initialize hardware at this point.
* Device properties are typically created here as well, except
* the ones we need to use for defining initialization parameters.
* Such pre-initialization properties are created in the constructor.
* (This device does not have any pre-initialization properties)
*/
int CEVA_NDE_PerkinElmerFPD::Initialize()
{
   if (initialized_)
      return DEVICE_OK;

   EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   if (pHub)
   {
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   // set property list
   // -----------------

   // Name
   int nRet = CreateProperty(MM::g_Keyword_Name, g_CameraDeviceName, MM::String, true);
   if (DEVICE_OK != nRet)
      return nRet;

   // Description
   nRet = CreateProperty(MM::g_Keyword_Description, "EVA_NDE_PerkinElmer Camera Device Adapter", MM::String, true);
   if (DEVICE_OK != nRet)
      return nRet;

   // CameraName
   nRet = CreateProperty(MM::g_Keyword_CameraName, g_CameraDeviceName, MM::String, true);
   assert(nRet == DEVICE_OK);

   // CameraID
   nRet = CreateProperty(MM::g_Keyword_CameraID, "V1.0", MM::String, true);
   assert(nRet == DEVICE_OK);

   // binning
   CPropertyAction *pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnBinning);
   nRet = CreateProperty(MM::g_Keyword_Binning, "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   nRet = SetAllowedBinning();
   if (nRet != DEVICE_OK)
      return nRet;

   // exposure
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnExposure);
   nRet = CreateProperty(MM::g_Keyword_Exposure, "10.0", MM::Float, false,pAct);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits(MM::g_Keyword_Exposure, 0, 12800);  //limit to 5s

	// Selected Model
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnSelectModel);
   nRet = CreateProperty("SelectModel", g_syncMode_0, MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

	// Sync mode
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnSyncMode);
   nRet = CreateProperty("SyncMode", g_syncMode_0, MM::String, false, pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("SyncMode",g_syncMode_0);
   AddAllowedValue("SyncMode",g_syncMode_1);
   AddAllowedValue("SyncMode",g_syncMode_2);
   AddAllowedValue("SyncMode",g_syncMode_3);


	// Trigger mode
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnTriggerMode);
   nRet = CreateProperty("TriggerMode", g_TriggerMode_0, MM::String, false, pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("TriggerMode",g_TriggerMode_0);
   AddAllowedValue("TriggerMode",g_TriggerMode_1);
   AddAllowedValue("TriggerMode",g_TriggerMode_2);
   AddAllowedValue("TriggerMode",g_TriggerMode_3);

	// Frame Timing
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnFrameTiming);
   nRet = CreateProperty("FrameTiming", g_FrameTime_TIMING_0, MM::String, false, pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_0);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_1);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_2);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_3);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_4);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_5);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_6);
   AddAllowedValue("FrameTiming",g_FrameTime_TIMING_7);

   	// Calibration mode
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnCalibrationMode);
   nRet = CreateProperty("CalibrationMode", g_CalibrationMode_None, MM::String, false, pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("CalibrationMode",g_CalibrationMode_None);
   AddAllowedValue("CalibrationMode",g_CalibrationMode_Offset);
   AddAllowedValue("CalibrationMode",g_CalibrationMode_Gain);
   AddAllowedValue("CalibrationMode",g_CalibrationMode_MultiGain);
   AddAllowedValue("CalibrationMode",g_CalibrationMode_PixelCorrection);

   	// Snap mode
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnSnapMode);
   nRet = CreateProperty("SnapMode", g_SnapMode_0, MM::String, false, pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("SnapMode",g_SnapMode_0);
   AddAllowedValue("SnapMode",g_SnapMode_1);
   AddAllowedValue("SnapMode",g_SnapMode_2);
   AddAllowedValue("SnapMode",g_SnapMode_3);

   	// pixel correction   
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnPixelCorrection);
   nRet = CreateProperty("PixelCorrection", g_SnapMode_0, MM::String, false,pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("PixelCorrection","On");
   AddAllowedValue("PixelCorrection","Off");

   	// Start Calibration
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnStartCalibration);
   nRet = CreateProperty("StartCalibration", "Select mode to start...", MM::String, false, pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("StartCalibration","Select mode to start...");
   AddAllowedValue("StartCalibration",g_CalibrationMode_None);
   AddAllowedValue("StartCalibration",g_CalibrationMode_Offset);
   AddAllowedValue("StartCalibration",g_CalibrationMode_Gain);
   AddAllowedValue("StartCalibration",g_CalibrationMode_MultiGain);
   AddAllowedValue("StartCalibration",g_CalibrationMode_PixelCorrection);

   	// Progress
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnProgress);
   nRet = CreateProperty(g_Keyword_Progress, "0", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   	// acqFrameCount
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnAcqFrameCount);
   nRet = CreateProperty("AcquisitionFrameCount", "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   	// offsetCalFrameCount
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnOffsetCalFrameCount);
   nRet = CreateProperty("OffsetCalFrameCount", "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   	// GainCalFrameCount
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnGainCalFrameCount);
   nRet = CreateProperty("GainCalFrameCount", "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   	// MultiGainCalFrameCount
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnMultiGainCalFrameCount);
   nRet = CreateProperty("MultiGainCalFrameCount", "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   	// MultiGainCalSegmentCount
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnMultiGainCalSegmentCount);
   nRet = CreateProperty("MultiGainCalSegmentCount", "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   // camera gain
   nRet = CreateProperty(MM::g_Keyword_Gain, "0", MM::Integer, false);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits(MM::g_Keyword_Gain, 0, 5);

   // camera offset
   nRet = CreateProperty(MM::g_Keyword_Offset, "0", MM::Integer, false);
   assert(nRet == DEVICE_OK);

   // Whether or not to use exposure time sequencing
   pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnIsSequenceable);
   std::string propName = "UseExposureSequences";
   CreateProperty(propName.c_str(), "No", MM::String, false, pAct);
   AddAllowedValue(propName.c_str(), "Yes");
   AddAllowedValue(propName.c_str(), "No");

   // Camera Status
   //pAct = new CPropertyAction (this, &CEVA_NDE_PerkinElmerFPD::OnStatus);
   std::string statusPropName = "Status";
   CreateProperty(statusPropName.c_str(), "Idle", MM::String, false);

   // set initial mode
   calibrationMode_.assign(g_CalibrationMode_None);
   snapMode_.assign(g_SnapMode_0);
   settingsID_.assign("");

   // synchronize all properties
   // --------------------------
   nRet = UpdateStatus();
   if (nRet != DEVICE_OK)
      return nRet;


   // setup the buffer
   // ----------------
   nRet = ResizeImageBuffer();
   if (nRet != DEVICE_OK)
      return nRet;

#ifdef TESTRESOURCELOCKING
   TestResourceLocking(true);
   LogMessage("TestResourceLocking OK",true);
#endif

   // initialize image buffer
   GenerateEmptyImage(img_);

   g_fpdLock.Lock();
   nRet = init(hAcqDesc,modelsCount_,selectedModel_);
   g_fpdLock.Unlock();
   if(nRet != HIS_ALL_OK)
		return DEVICE_ERR;
  
   SetPropertyLimits("SelectModel", 0, modelsCount_-1);

   g_fpdLock.Lock();
   nRet = openDevice(hAcqDesc);
   g_fpdLock.Unlock();
   if(nRet != HIS_ALL_OK)
		return DEVICE_ERR;

   g_fpdLock.Lock();
   refreshSettings(hAcqDesc);
   reloadCalibrationData();
   g_fpdLock.Unlock();

  initialized_ = true;
   return DEVICE_OK;
}

/**
* Shuts down (unloads) the device.
* Required by the MM::Device API.
* Ideally this method will completely unload the device and release all resources.
* Shutdown() may be called multiple times in a row.
* After Shutdown() we should be allowed to call Initialize() again to load the device
* without causing problems.
*/
int CEVA_NDE_PerkinElmerFPD::Shutdown()
{
   ////EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;
	g_fpdLock.Lock();
   closeDevice();
   g_fpdLock.Unlock();

   initialized_ = false;
   return DEVICE_OK;
}
bool CEVA_NDE_PerkinElmerFPD::WaitForExposureDone()throw()
{

  MM::MMTime startTime = GetCurrentMMTime();
   bool bRet=false;
   try
   {
      // make the time out 2 seconds plus twice the exposure
      // Added readout time, this caused troubles on very low readout speeds and large buffers, this code timeouted before the image was read out
      MM::MMTime timeout((long)(20  + 2*GetExposure() * 0.001), (long)(2*GetExposure()));
      MM::MMTime startTime = GetCurrentMMTime();
      MM::MMTime elapsed(0,0);

      do 
      {
         CDeviceUtils::SleepMs(1);
         elapsed = GetCurrentMMTime()  - startTime;
      } while(!_isReady && elapsed < timeout); 
	  // prepare for next acquisition
	  _isReady = 1;
   }
   catch(...)
   {
      LogMessage("Unknown exception while waiting for exposure to finish", false);
   }
   return bRet;
}
/**
* Performs exposure and grabs a single image.
* This function should block during the actual exposure and return immediately afterwards 
* (i.e., before readout).  This behavior is needed for proper synchronization with the shutter.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PerkinElmerFPD::SnapImage()
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;
   int ret = DEVICE_ERR;
	static int callCounter = 0;
	++callCounter;
	if(!_isReady)
   {
      LogMessage("Warning: Entering SnapImage while GetImage has not been done for previous frame", true);
      return DEVICE_CAMERA_BUSY_ACQUIRING;
   }
    MMThreadGuard g(imgPixelsLock_);
    unsigned short* pBuf = ( unsigned short*)GetImageBuffer();
	g_fpdLock.Lock();
	ret = acquireImage(hAcqDesc,pBuf);
    g_fpdLock.Unlock();
	if(ret != HIS_ALL_OK)
		return DEVICE_ERR;

   return DEVICE_OK;
}


/**
* Returns pixel data.
* Required by the MM::Camera API.
* The calling program will assume the size of the buffer based on the values
* obtained from GetImageBufferSize(), which in turn should be consistent with
* values returned by GetImageWidth(), GetImageHight() and GetImageBytesPerPixel().
* The calling program allso assumes that camera never changes the size of
* the pixel buffer on its own. In other words, the buffer can change only if
* appropriate properties are set (such as binning, pixel type, etc.)
*/
const unsigned char* CEVA_NDE_PerkinElmerFPD::GetImageBuffer()
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   MMThreadGuard g(imgPixelsLock_);
   unsigned char *pB = (unsigned char*)(img_.GetPixels());
   return pB;
}

/**
* Returns image buffer X-size in pixels.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PerkinElmerFPD::GetImageWidth() const
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   return img_.Width();
}

/**
* Returns image buffer Y-size in pixels.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PerkinElmerFPD::GetImageHeight() const
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   return img_.Height();
}

/**
* Returns image buffer pixel depth in bytes.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PerkinElmerFPD::GetImageBytesPerPixel() const
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   return img_.Depth();
} 

/**
* Returns the bit depth (dynamic range) of the pixel.
* This does not affect the buffer size, it just gives the client application
* a guideline on how to interpret pixel values.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PerkinElmerFPD::GetBitDepth() const
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   return bitDepth_;
}

/**
* Returns the size in bytes of the image buffer.
* Required by the MM::Camera API.
*/
long CEVA_NDE_PerkinElmerFPD::GetImageBufferSize() const
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   return img_.Width() * img_.Height() * GetImageBytesPerPixel();
}

/**
* Sets the camera Region Of Interest.
* Required by the MM::Camera API.
* This command will change the dimensions of the image.
* Depending on the hardware capabilities the camera may not be able to configure the
* exact dimensions requested - but should try do as close as possible.
* If the hardware does not have this capability the software should simulate the ROI by
* appropriately cropping each frame.
* This EVA_NDE_PerkinElmer implementation ignores the position coordinates and just crops the buffer.
* @param x - top-left corner coordinate
* @param y - top-left corner coordinate
* @param xSize - width
* @param ySize - height
*/
int CEVA_NDE_PerkinElmerFPD::SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
    //  return HUB_UNKNOWN_ERR;

   if (xSize == 0 && ySize == 0)
   {
      // effectively clear ROI
      ResizeImageBuffer();
      roiX_ = 0;
      roiY_ = 0;
   }
   else
   {
      // apply ROI
      img_.Resize(xSize, ySize);
      roiX_ = x;
      roiY_ = y;
   }
   return DEVICE_OK;
}

/**
* Returns the actual dimensions of the current ROI.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PerkinElmerFPD::GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   x = roiX_;
   y = roiY_;

   xSize = img_.Width();
   ySize = img_.Height();

   return DEVICE_OK;
}

/**
* Resets the Region of Interest to full frame.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PerkinElmerFPD::ClearROI()
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   ResizeImageBuffer();
   roiX_ = 0;
   roiY_ = 0;
      
   return DEVICE_OK;
}

/**
* Returns the current exposure setting in milliseconds.
* Required by the MM::Camera API.
*/
double CEVA_NDE_PerkinElmerFPD::GetExposure() const
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;
    return exposureMs_;
}

/**
 * Returns the current exposure from a sequence and increases the sequence counter
 * Used for exposure sequences
 */
double CEVA_NDE_PerkinElmerFPD::GetSequenceExposure() 
{
   if (exposureSequence_.size() == 0) 
      return this->GetExposure();

   double exposure = exposureSequence_[sequenceIndex_];

   sequenceIndex_++;
   if (sequenceIndex_ >= exposureSequence_.size())
      sequenceIndex_ = 0;

   return exposure;
}

/**
* Sets exposure in milliseconds.
* Required by the MM::Camera API.
*/
void CEVA_NDE_PerkinElmerFPD::SetExposure(double exp)
{
   SetProperty(MM::g_Keyword_Exposure, CDeviceUtils::ConvertToString(exp));

}

/**
* Returns the current binning factor.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PerkinElmerFPD::GetBinning() const
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   char buf[MM::MaxStrLength];
   int ret = GetProperty(MM::g_Keyword_Binning, buf);
   if (ret != DEVICE_OK)
      return 1;
   return atoi(buf);
}

/**
* Sets binning factor.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PerkinElmerFPD::SetBinning(int binF)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   return SetProperty(MM::g_Keyword_Binning, CDeviceUtils::ConvertToString(binF));
}

/**
 * Clears the list of exposures used in sequences
 */
int CEVA_NDE_PerkinElmerFPD::ClearExposureSequence()
{
   exposureSequence_.clear();
   return DEVICE_OK;
}

/**
 * Adds an exposure to a list of exposures used in sequences
 */
int CEVA_NDE_PerkinElmerFPD::AddToExposureSequence(double exposureTime_ms) 
{
   exposureSequence_.push_back(exposureTime_ms);
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::SetAllowedBinning() 
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   vector<string> binValues;
   binValues.push_back("1");
   //binValues.push_back("2");
   //binValues.push_back("3");
   //binValues.push_back("4");
   //binValues.push_back("5");
   LogMessage("Setting Allowed Binning settings", true);
   return SetAllowedValues(MM::g_Keyword_Binning, binValues);
}


/**
 * Required by the MM::Camera API
 * Please implement this yourself and do not rely on the base class implementation
 * The Base class implementation is deprecated and will be removed shortly
 */
int CEVA_NDE_PerkinElmerFPD::StartSequenceAcquisition(double interval) {
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   return StartSequenceAcquisition(LONG_MAX, interval, false);            
}

/**                                                                       
* Stop and wait for the Sequence thread finished                                   
*/                                                                        
int CEVA_NDE_PerkinElmerFPD::StopSequenceAcquisition()                                     
{
   if (IsCallbackRegistered())
   {
      //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
      //if (!pHub)
      //   return HUB_UNKNOWN_ERR;
   }

   if (!thd_->IsStopped()) {
      thd_->Stop();                                                       
      thd_->wait();                                                       
   }                                                                      
                                                                          
   return DEVICE_OK;                                                      
} 

/**
* Simple implementation of Sequence Acquisition
* A sequence acquisition should run on its own thread and transport new images
* coming of the camera into the MMCore circular buffer.
*/
int CEVA_NDE_PerkinElmerFPD::StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (IsCapturing())
      return DEVICE_CAMERA_BUSY_ACQUIRING;

   int ret = GetCoreCallback()->PrepareForAcq(this);
   if (ret != DEVICE_OK)
      return ret;
   sequenceStartTime_ = GetCurrentMMTime();
   imageCounter_ = 0;



   thd_->Start(numImages,interval_ms);
   stopOnOverflow_ = stopOnOverflow;
   return DEVICE_OK;
}

/*
 * Inserts Image and MetaData into MMCore circular Buffer
 */
int CEVA_NDE_PerkinElmerFPD::InsertImage()
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   MM::MMTime timeStamp = this->GetCurrentMMTime();
   char label[MM::MaxStrLength];
   this->GetLabel(label);
 
   // Important:  metadata about the image are generated here:
   Metadata md;
   md.put("Camera", label);
   md.put(MM::g_Keyword_Metadata_StartTime, CDeviceUtils::ConvertToString(sequenceStartTime_.getMsec()));
   md.put(MM::g_Keyword_Elapsed_Time_ms, CDeviceUtils::ConvertToString((timeStamp - sequenceStartTime_).getMsec()));
   md.put(MM::g_Keyword_Metadata_ROI_X, CDeviceUtils::ConvertToString( (long) roiX_)); 
   md.put(MM::g_Keyword_Metadata_ROI_Y, CDeviceUtils::ConvertToString( (long) roiY_)); 

   imageCounter_++;

   char buf[MM::MaxStrLength];
   GetProperty(MM::g_Keyword_Binning, buf);
   md.put(MM::g_Keyword_Binning, buf);

   MMThreadGuard g(imgPixelsLock_);

   const unsigned char* pI;
   pI = GetImageBuffer();

   unsigned int w = GetImageWidth();
   unsigned int h = GetImageHeight();
   unsigned int b = GetImageBytesPerPixel();

   int ret = GetCoreCallback()->InsertImage(this, pI, w, h, b, md.Serialize().c_str());
   if (!stopOnOverflow_ && ret == DEVICE_BUFFER_OVERFLOW)
   {
      // do not stop on overflow - just reset the buffer
      GetCoreCallback()->ClearImageBuffer(this);
      // don't process this same image again...
      return GetCoreCallback()->InsertImage(this, pI, w, h, b, md.Serialize().c_str(), false);
   } else
      return ret;
}

/*
 * Do actual capturing
 * Called from inside the thread  
 */
int CEVA_NDE_PerkinElmerFPD::ThreadRun (MM::MMTime startTime)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   int ret=DEVICE_ERR;

   ret = SnapImage();
   if (ret != DEVICE_OK)
   {
      return ret;
   }
   ret = InsertImage();

   if (ret != DEVICE_OK)
   {
      return ret;
   }
   return ret;
};

bool CEVA_NDE_PerkinElmerFPD::IsCapturing() {
   return !thd_->IsStopped();
}

/*
 * called from the thread function before exit 
 */
void CEVA_NDE_PerkinElmerFPD::OnThreadExiting() throw()
{
   try
   {
      LogMessage(g_Msg_SEQUENCE_ACQUISITION_THREAD_EXITING);
      GetCoreCallback()?GetCoreCallback()->AcqFinished(this,0):DEVICE_OK;
   }

   catch( CMMError& e){
      std::ostringstream oss;
      oss << g_Msg_EXCEPTION_IN_ON_THREAD_EXITING << " " << e.getMsg() << " " << e.getCode();
      LogMessage(oss.str().c_str(), false);
   }
   catch(...)
   {
      LogMessage(g_Msg_EXCEPTION_IN_ON_THREAD_EXITING, false);
   }
}


MySequenceThread::MySequenceThread(CEVA_NDE_PerkinElmerFPD* pCam)
   :intervalMs_(default_intervalMS)
   ,numImages_(default_numImages)
   ,imageCounter_(0)
   ,stop_(true)
   ,suspend_(false)
   ,camera_(pCam)
   ,startTime_(0)
   ,actualDuration_(0)
   ,lastFrameTime_(0)
{};

MySequenceThread::~MySequenceThread() {};

void MySequenceThread::Stop() {
   MMThreadGuard(this->stopLock_);
   stop_=true;
}

void MySequenceThread::Start(long numImages, double intervalMs)
{
   MMThreadGuard(this->stopLock_);
   MMThreadGuard(this->suspendLock_);
   numImages_=numImages;
   intervalMs_=intervalMs;
   imageCounter_=0;
   stop_ = false;
   suspend_=false;
   activate();
   actualDuration_ = 0;
   startTime_= camera_->GetCurrentMMTime();
   lastFrameTime_ = 0;
}

bool MySequenceThread::IsStopped(){
   MMThreadGuard(this->stopLock_);
   return stop_;
}

void MySequenceThread::Suspend() {
   MMThreadGuard(this->suspendLock_);
   suspend_ = true;
}

bool MySequenceThread::IsSuspended() {
   MMThreadGuard(this->suspendLock_);
   return suspend_;
}

void MySequenceThread::Resume() {
   MMThreadGuard(this->suspendLock_);
   suspend_ = false;
}

int MySequenceThread::svc(void) throw()
{
   int ret=DEVICE_ERR;
   try 
   {
      do
      {  
         ret=camera_->ThreadRun(startTime_);
      } while (DEVICE_OK == ret && !IsStopped() && imageCounter_++ < numImages_-1);
      if (IsStopped())
         camera_->LogMessage("SeqAcquisition interrupted by the user\n");

   }catch( CMMError& e){
      camera_->LogMessage(e.getMsg(), false);
      ret = e.getCode();
   }catch(...){
      camera_->LogMessage(g_Msg_EXCEPTION_IN_THREAD, false);
   }
   stop_=true;
   actualDuration_ = camera_->GetCurrentMMTime() - startTime_;
   camera_->OnThreadExiting();
   return ret;
}


///////////////////////////////////////////////////////////////////////////////
// CEVA_NDE_PerkinElmerFPD Action handlers
///////////////////////////////////////////////////////////////////////////////



//int CEVA_NDE_PerkinElmerFPD::OnSwitch(MM::PropertyBase* pProp, MM::ActionType eAct)
//{
   // use cached values
//   return DEVICE_OK;
//}

/**
* Handles "Binning" property.
*/
int CEVA_NDE_PerkinElmerFPD::OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   int ret = DEVICE_ERR;
   switch(eAct)
   {
   case MM::AfterSet:
      {
         if(IsCapturing())
            return DEVICE_CAMERA_BUSY_ACQUIRING;

         // the user just set the new value for the property, so we have to
         // apply this value to the 'hardware'.
         pProp->Get(binMode_);
		ResizeImageBuffer();
		ret=DEVICE_OK;
      }
	  break;
   case MM::BeforeGet:
      {
         ret=DEVICE_OK;
			pProp->Set(binMode_);
      }break;
   }
   return ret; 
}
/**
* Handles "Binning" property.
*/
int CEVA_NDE_PerkinElmerFPD::OnExposure(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   int ret = DEVICE_ERR;
   switch(eAct)
   {
   case MM::AfterSet:
      {
         pProp->Get(exposureMs_);
		 GetCoreCallback()->OnExposureChanged(this, exposureMs_);
		 ret=DEVICE_OK;
		 refreshSettings(hAcqDesc);
	     reloadCalibrationData();
      }
	  break;
   case MM::BeforeGet:
      {
         ret=DEVICE_OK;
		 pProp->Set(exposureMs_);
      }break;
   }
   return ret; 
}

/*
* Handles "syncMode" property.
* Changes allowed Binning values to test whether the UI updates properly
*/
int CEVA_NDE_PerkinElmerFPD::OnSyncMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{ 
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;
   if (eAct == MM::AfterSet) {
	  std::string tmp;
      pProp->Get(tmp);
	  if(tmp.compare(g_syncMode_0)==0)
		  syncMode_ = 1;
	  else if(tmp.compare(g_syncMode_1)==0)
		  syncMode_ = 2;
	  else if(tmp.compare(g_syncMode_2)==0)
		  syncMode_ = 3;
	  else if(tmp.compare(g_syncMode_3)==0)
		  syncMode_ = 4;
	  else
		  syncMode_ = 1;
	   refreshSettings(hAcqDesc);
	   reloadCalibrationData();
      if (initialized_) {
         int ret = OnPropertiesChanged();
         if (ret != DEVICE_OK)
            return ret;
      }
   } else if (eAct == MM::BeforeGet) {
	  std::string tmp;
	  switch(syncMode_)
	  {
	  case 1:
		  tmp.assign(g_syncMode_0);
		  break;
	  case 2:
		  tmp.assign(g_syncMode_1);
		  break;
	  case 3:
		  tmp.assign(g_syncMode_2);
		  break;
	  case 4:
		  tmp.assign(g_syncMode_3);
		  break;
	  default:
		  tmp.assign("UNDEFINED");
	  }
	  pProp->Set(tmp.c_str());
   }
   return DEVICE_OK;
}

/*
* Handles "TriggerMode" property.
* Changes allowed Binning values to test whether the UI updates properly
*/
int CEVA_NDE_PerkinElmerFPD::OnTriggerMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{ 
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::AfterSet) {
	  std::string tmp;
      pProp->Get(tmp);
	  if(tmp.compare(g_TriggerMode_0)==0)
		  triggerMode_ = 0;
	  else if(tmp.compare(g_TriggerMode_1)==0)
		  triggerMode_ = 1;
	  else if(tmp.compare(g_TriggerMode_2)==0)
		  triggerMode_ = 2;
	  else if(tmp.compare(g_TriggerMode_3)==0)
		  triggerMode_ = 3;
	  else
		  triggerMode_ = 0;

   } else if (eAct == MM::BeforeGet) {
	  std::string tmp;
	  switch(triggerMode_)
	  {
	  case 0:
		  tmp.assign(g_TriggerMode_0);
		  break;
	  case 1:
		  tmp.assign(g_TriggerMode_1);
		  break;
	  case 2:
		  tmp.assign(g_TriggerMode_2);
		  break;
	  case 3:
		  tmp.assign(g_TriggerMode_3);
		  break;
	  default:
		  tmp.assign("UNDEFINED");
	  }
	  pProp->Set(tmp.c_str());
   }
   return DEVICE_OK;
}
/*
* Handles "FrameTime" property.
* Changes allowed Binning values to test whether the UI updates properly
*/
int CEVA_NDE_PerkinElmerFPD::OnFrameTiming(MM::PropertyBase* pProp, MM::ActionType eAct)
{ 
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::AfterSet) {
	  std::string tmp;
      pProp->Get(tmp);
	  if(tmp.compare(g_FrameTime_TIMING_0)==0)
	  {
		  SetExposure(100.0);
		  frameTiming_ = 0;
	  }
	  else if(tmp.compare(g_FrameTime_TIMING_1)==0)
	  {
		  SetExposure(200.0);
		  frameTiming_ = 1;
	  }
	  else if(tmp.compare(g_FrameTime_TIMING_2)==0)
	  {
		  SetExposure(400.0);
		  frameTiming_ = 2;
	  }
	  else if(tmp.compare(g_FrameTime_TIMING_3)==0)
	  {
		  SetExposure(800.0);
		  frameTiming_ = 3;
	  }
	  else if(tmp.compare(g_FrameTime_TIMING_4)==0)
	  {
		  SetExposure(1600.0);
		  frameTiming_ = 4;
	  }
	  else if(tmp.compare(g_FrameTime_TIMING_5)==0)
	  {
		  SetExposure(3200.0);
		  frameTiming_ = 5;
	  }
	  else if(tmp.compare(g_FrameTime_TIMING_6)==0)
	  {
		  SetExposure(6400.0);
		  frameTiming_ = 6;
	  }
	  else if(tmp.compare(g_FrameTime_TIMING_7)==0)
	  {
		  SetExposure(12800.0);
		  frameTiming_ = 7;
	  }
	  else
	  {
		  SetExposure(0.0);
		  frameTiming_ = 0;
	  }
	  refreshSettings(hAcqDesc);
	  reloadCalibrationData();
      if (initialized_) {
         int ret = OnPropertiesChanged();
         if (ret != DEVICE_OK)
            return ret;
      }

   } else if (eAct == MM::BeforeGet) {
	  std::string tmp;
	  switch(frameTiming_)
	  {
	  case 0:
		  tmp.assign(g_FrameTime_TIMING_0);
		  break;
	  case 1:
		  tmp.assign(g_FrameTime_TIMING_1);
		  break;
	  case 2:
		  tmp.assign(g_FrameTime_TIMING_2);
		  break;
	  case 3:
		  tmp.assign(g_FrameTime_TIMING_3);
		  break;
	  case 4:
		  tmp.assign(g_FrameTime_TIMING_4);
		  break;
	  case 5:
		  tmp.assign(g_FrameTime_TIMING_5);
		  break;
	  case 6:
		  tmp.assign(g_FrameTime_TIMING_6);
		  break;
	  case 7:
		  tmp.assign(g_FrameTime_TIMING_7);
		  break;
	  default:
		  // set to default value 0
		  frameTiming_ = 0;
		  tmp.assign(g_FrameTime_TIMING_0);
	  }
	  pProp->Set(tmp.c_str());
   }
   return DEVICE_OK;
}
/*
* Handles "FrameTime" property.
* Changes allowed Binning values to test whether the UI updates properly
*/
int CEVA_NDE_PerkinElmerFPD::OnSelectModel(MM::PropertyBase* pProp, MM::ActionType eAct)
{ 
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;
   int nRet=1;
   if (eAct == MM::AfterSet) {
	   long tmp;
      pProp->Get(tmp);
	  if(tmp!=selectedModel_)
	  {
	   selectedModel_ = tmp;
	   g_fpdLock.Lock();
	   nRet = init(hAcqDesc,modelsCount_,selectedModel_);
	   g_fpdLock.Unlock();
	   if(nRet != HIS_ALL_OK)
			return DEVICE_ERR;

	   g_fpdLock.Lock();
	   nRet = openDevice(hAcqDesc);
	   g_fpdLock.Unlock();

	   refreshSettings(hAcqDesc);
	   reloadCalibrationData();

	   if(nRet != HIS_ALL_OK)
			return DEVICE_ERR;
	  }
   } else if (eAct == MM::BeforeGet) {
	  long temp = selectedModel_;
	  pProp->Set(temp);
   }
   return DEVICE_OK;
}
int CEVA_NDE_PerkinElmerFPD::OnTriggerDevice(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(triggerDevice_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(triggerDevice_);
   }
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::OnCalibrationMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
	   pProp->Set(calibrationMode_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(calibrationMode_);
   }
   return DEVICE_OK;
}
int CEVA_NDE_PerkinElmerFPD::OnSnapMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
	   pProp->Set(snapMode_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(snapMode_);
	 //raw
	if(snapMode_.compare(g_SnapMode_0)==0)
	{
		pGainBufferInUse = NULL;
		pOffsetBufferInUse = NULL;
	}	
	//offset
	else if(snapMode_.compare(g_SnapMode_1)==0)
	{
		pGainBufferInUse = NULL;
		pOffsetBufferInUse = pOffsetBuffer;
	}
	//gain/offset
	else if(snapMode_.compare(g_SnapMode_2)==0)
	{
		pGainBufferInUse = pGainBuffer;
		pOffsetBufferInUse = pOffsetBuffer;
	}
	//multi-gain mode
	else if(snapMode_.compare(g_SnapMode_3)==0)
	{
		pGainBufferInUse = pGainBuffer;
		pOffsetBufferInUse = pOffsetBuffer;
	}
	else
	{
		pGainBufferInUse = pGainBuffer;
		pOffsetBufferInUse = pOffsetBuffer;
	}
	  if (initialized_) {
         int ret = OnPropertiesChanged();
         if (ret != DEVICE_OK)
            return ret;
      }
   }
   return DEVICE_OK;
}
int CEVA_NDE_PerkinElmerFPD::OnStartCalibration(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set("Select mode to start...");
   }
   else if (eAct == MM::AfterSet)
   {
	  std::string tmp;
      pProp->Get(tmp);
	  if(tmp.compare("Select mode to start...")!=0)
	  {	
		  calibrationMode_.assign(tmp.c_str());
	  }
	  else
		  return DEVICE_OK;
	// const char* g_CalibrationMode_None = "None";
	//const char* g_CalibrationMode_Offset = "Offset";
	//const char* g_CalibrationMode_Gain = "Gain/Offset";
	//const char* g_CalibrationMode_MultiGain = "Multi-gain";
	//const char* g_CalibrationMode_PixelCorrection
		int nRet = HIS_ERROR_UNDEFINED;
		if(calibrationMode_.compare(g_CalibrationMode_None)==0)
		{
			nRet=HIS_ALL_OK;
		}
		else if(calibrationMode_.compare(g_CalibrationMode_Offset)==0)
		{
			nRet = startOffsetCalibration(hAcqDesc);
		}
		else if(calibrationMode_.compare(g_CalibrationMode_Gain)==0)
		{
			if(pOffsetBuffer == NULL)
			{
				MessageBoxNonBlocking("You should perform OFFSET calibration firstly!");
				return GAIN_CALIBRATION_FAILED;
			}
			nRet = startGainCalibration(hAcqDesc);
		}
		else if(calibrationMode_.compare(g_CalibrationMode_MultiGain)==0)
		{
			if(pOffsetBuffer == NULL)
			{
				MessageBoxNonBlocking("You should perform OFFSET calibration firstly!");
				return MULTI_GAIN_CALIBRATION_FAILED;
			}
			nRet = startMultiGainCalibration(hAcqDesc);
		}
		else if(calibrationMode_.compare(g_CalibrationMode_PixelCorrection)==0)
		{
			if(pOffsetBuffer == NULL)
			{
				MessageBoxNonBlocking("You should perform OFFSET and GAIN calibration firstly!");
				return PIXEL_CORRECTION_FAILED;
			}
			if(pGainBuffer == NULL)
			{
				MessageBoxNonBlocking("You should perform GAIN calibration firstly!");
				return PIXEL_CORRECTION_FAILED;
			}

			nRet = startPixelCorrection(hAcqDesc);

		}
		if(nRet!=HIS_ALL_OK)
			return nRet;

		
   }
   return DEVICE_OK;
}
int CEVA_NDE_PerkinElmerFPD::OnPixelCorrection(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
	  if(pixelCorrection_)
		  pProp->Set("On");
	  else
		  pProp->Set("Off");
   }
   else if (eAct == MM::AfterSet)
   {
	  std::string tmp;
      pProp->Get(tmp);
	  if(tmp.compare("On")==0)
	  {	
		 pixelCorrection_ = true;
		 if(pCorrList)
			pCorrListInUse =(DWORD *) pCorrList;
		 else
		 {
			pixelCorrection_ = false;
			pCorrListInUse = NULL;
			if (initialized_)
			{
				MessageBoxNonBlocking("Pixel Correction data is not available!");
			}
		 }
	  }
	  else
	  {
		 pixelCorrection_ = false;
		 pCorrListInUse = NULL;
	  }


	  if (initialized_) {
         int ret = OnPropertiesChanged();
         if (ret != DEVICE_OK)
            return ret;
      }


   }
   return DEVICE_OK;
}
int CEVA_NDE_PerkinElmerFPD::OnProgress(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(progress_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(progress_);
   }
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::OnAcqFrameCount(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(acqFrameCount_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(acqFrameCount_);
   }
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::OnOffsetCalFrameCount(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(offsetFrameCount_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(offsetFrameCount_);
   }
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::OnGainCalFrameCount(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(gainFrameCount_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(gainFrameCount_);
   }
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::OnMultiGainCalFrameCount(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(multiGainFrameCount_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(multiGainFrameCount_);
   }
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::OnMultiGainCalSegmentCount(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(multiGainSegmentCount_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(multiGainSegmentCount_);
   }
   return DEVICE_OK;
}

int CEVA_NDE_PerkinElmerFPD::OnIsSequenceable(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   std::string val = "Yes";
   if (eAct == MM::BeforeGet)
   {
      if (!isSequenceable_) 
      {
         val = "No";
      }
      pProp->Set(val.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      isSequenceable_ = false;
      pProp->Get(val);
      if (val == "Yes") 
      {
         isSequenceable_ = true;
      }
   }

   return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Private CEVA_NDE_PerkinElmerFPD methods
///////////////////////////////////////////////////////////////////////////////

/**
* Sync internal image buffer size to the chosen property values.
*/
int CEVA_NDE_PerkinElmerFPD::ResizeImageBuffer()
{
	long binSizeX_;
	long binSizeY_;
	switch(binMode_)
	{
		case 1:
			binSizeX_ = 1; binSizeY_ =1;
			break;
		case 2:
			binSizeX_ = 2; binSizeY_ =2;
			break;
		case 3:
			binSizeX_ = 4; binSizeY_ =4;
			break;
		case 4:
			binSizeX_ = 1; binSizeY_ =2;
			break;
		case 5:
			binSizeX_ = 1; binSizeY_ =4;
			break;
		default:
			binSizeX_ = 1; binSizeY_ =1;
	}
	img_.Resize(image_width/binSizeX_, image_height/binSizeY_,byteDepth_);
   return DEVICE_OK;
}

void CEVA_NDE_PerkinElmerFPD::GenerateEmptyImage(ImgBuffer& img)
{
   MMThreadGuard g(imgPixelsLock_);
   if (img.Height() == 0 || img.Width() == 0 || img.Depth() == 0)
      return;
   unsigned char* pBuf = const_cast<unsigned char*>(img.GetPixels());
   memset(pBuf, 0, img.Height()*img.Width()*img.Depth());
}



//----------------------------------------------------------------------------------------------------------------------//
// The OnEndFrameCallback function is called at the end of each frame's data transfer									//
// In the end of frame callback we can place output, do image processing, etc.											//
//----------------------------------------------------------------------------------------------------------------------//
void CALLBACK CEVA_NDE_PerkinElmerFPD::OnEndFrameCallback(HACQDESC hAcqDesc)
{
	DWORD dwActFrame, dwSecFrame, dwRow=128, dwCol=128;
	UINT dwDataType, dwRows, dwColumns, dwFrames, dwSortFlags;
	DWORD dwAcqType, dwSystemID, dwAcqData, dwSyncMode, dwHwAccess;
	BOOL dwIRQFlags;
	CHwHeaderInfo Info; 
	CHwHeaderInfoEx InfoEx; // new Header 1621
	//----------------------------------------------------------------------------------------------------------------------//
	// The pointer you retrieve by Acquisition_GetAcqData(..) has been set in the main function by Acquisition_SetAcqData.	//
	// Anyway, you are free to define for instance a global variable to indicate the different acquisition modes			//
	// in the callback functions. The above mentioned approach has the advantage of a encapsulation							//
	// similar to that used in object orientated programming.																//
	//----------------------------------------------------------------------------------------------------------------------//

#ifdef __X64
	void *vpAcqData=NULL;
	Acquisition_GetAcqData(hAcqDesc, &vpAcqData);
	dwAcqData = *((DWORD*)vpAcqData);
#else
	Acquisition_GetAcqData(hAcqDesc, &dwAcqData);
#endif

	Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
		&dwColumns, &dwDataType, &dwSortFlags, &dwIRQFlags, &dwAcqType, 
		&dwSystemID, &dwSyncMode, &dwHwAccess);
	Acquisition_GetActFrame(hAcqDesc, &dwActFrame, &dwSecFrame);	

	// 1621 function demo
	Acquisition_GetLatestFrameHeader(hAcqDesc,&Info,&InfoEx);
	//if (Info.dwHeaderID==14 && InfoEx.wCameratype==1) //1621 ?)
	if (Info.dwHeaderID==14)
	{
		sprintf(activeFPDins->strBuffer, "framecount: %d frametime %d,%d millisec\t",InfoEx.wFrameCnt,InfoEx.wRealInttime_milliSec,InfoEx.wRealInttime_microSec);
		WriteConsole(activeFPDins->hOutput, activeFPDins->strBuffer, strlen(activeFPDins->strBuffer), &activeFPDins->dwCharsWritten, NULL);
	}
	//// 1621 function demo end

	//// Depending from the passed status flag it is decided how to read out the acquisition buffer:
	//if (dwAcqData & ACQ_SNAP)
	//{
	//	sprintf(activeFPDins->strBuffer, "acq buffer frame: %d, dest frame %d, row: %d, col: %d, value: %d\n", 
	//		dwActFrame, dwSecFrame,
	//		dwRow, dwCol, activeFPDins->pAcqBuffer[dwColumns*dwRow+dwCol]);
	//		//dwRow, dwCol, pAcqBuffer[((dwSecFrame-1)*(dwColumns*dwRows)) + dwColumns*dwRow+dwCol]);
	//}

	//else if (dwAcqData & ACQ_CONT) 
	//{
	//	sprintf(activeFPDins->strBuffer, "acq buffer frame: %d, dest frame %d, row: %d, col: %d, value: %d\n", 
	//		dwActFrame, dwSecFrame,
	//		dwRow, dwCol, activeFPDins->pAcqBuffer[/*((dwSecFrame-1)*(dwColumns*dwRows)) + */dwColumns*dwRow+dwCol]);


	//} else if (dwAcqData & ACQ_OFFSET)
	//{
	//	sprintf(activeFPDins->strBuffer, "offset buffer frame: %d, dest frame %d, row: %d, col: %d, value: %d\n", 
	//		dwActFrame, dwSecFrame,
	//		dwRow, dwCol, activeFPDins->pOffsetBuffer[dwColumns*dwRow+dwCol]);

	//}  else if (dwAcqData & ACQ_GAIN)
	//{
	//	sprintf(activeFPDins->strBuffer, "gain buffer frame: %d, dest frame %d, row: %d, col: %d, value: %d\n", 
	//		dwActFrame, dwSecFrame,
	//		dwRow, dwCol, activeFPDins->pGainBuffer[dwColumns*dwRow+dwCol]);

	//} else	if (dwAcqData & ACQ_Brightoc)
	//{
	//	sprintf(activeFPDins->strBuffer, "gain buffer frame: %d, dest frame %d, row: %d, col: %d, value: %d\n", 
	//		dwActFrame, dwSecFrame,
	//		dwRow, dwCol, activeFPDins->pBrightocBuffer[dwColumns*dwRow+dwCol]);

	//}
	//else
	//{
	//	//printf("endframe \n");
	//	sprintf(activeFPDins->strBuffer, "endframe image size %d\n\n",dwRows);
	//}


	 if (activeFPDins->initialized_) {
		int ret= activeFPDins->SetProperty(g_Keyword_Progress,CDeviceUtils::ConvertToString((long)dwActFrame));
        ret = activeFPDins->OnPropertiesChanged();
		sprintf(activeFPDins->strBuffer, "frame: %d\n\n",dwActFrame);
	 }
	 WriteConsole(activeFPDins->hOutput, activeFPDins->strBuffer, strlen(activeFPDins->strBuffer), &activeFPDins->dwCharsWritten, NULL);
}

//callback function that is called by XISL at end of acquisition
void CALLBACK CEVA_NDE_PerkinElmerFPD::OnEndAcqCallback(HACQDESC hAcqDesc)
{

	DWORD dwActFrame, dwSecFrame;
	UINT dwDataType, dwRows, dwColumns, dwFrames, dwSortFlags;
	DWORD dwAcqType, dwSystemID, dwAcqData, dwSyncMode, dwHwAccess;
	BOOL dwIRQFlags;

	activeFPDins->_isReady = 1;
#ifdef __X64
	void *vpAcqData=NULL;
	Acquisition_GetAcqData(hAcqDesc, &vpAcqData);
	dwAcqData = *((DWORD*)vpAcqData);
#else
	Acquisition_GetAcqData(hAcqDesc, &dwAcqData);
#endif

	//Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
	//	&dwColumns, &dwDataType, &dwSortFlags, &dwIRQFlags, &dwAcqType, 
	//	&dwSystemID, &dwSyncMode, &dwHwAccess);
	//Acquisition_GetActFrame(hAcqDesc, &dwActFrame, &dwSecFrame);
	//if (activeFPDins->initialized_) {
	//	int ret= activeFPDins->SetProperty(g_Keyword_Progress,CDeviceUtils::ConvertToString((long)dwFrames));
 //       ret = activeFPDins->OnPropertiesChanged();
	//	sprintf(activeFPDins->strBuffer, "frame: %d\n\n",dwActFrame);
	// }

 	printf("End of Acquisition\n");
	//SetEvent(hevEndAcq);

}

long CEVA_NDE_PerkinElmerFPD::DetectorInit(HACQDESC* phAcqDesc, long bGigETest, int IBIN1, int iGain,int &sensorCount,int &selectedSensorNum)
{
	int iRet;							// Return value
	int iCnt;							// 
	unsigned int uiNumSensors;			// Number of sensors
	HACQDESC hAcqDesc=NULL;
	unsigned short usTiming=0;
	unsigned short usNetworkLoadPercent=80;
	int nChannelNr;
	UINT nChannelType;

	int iSelected;						// Index of selected GigE detector
	long ulNumSensors = 0;				// nr of GigE detector in network

	GBIF_DEVICE_PARAM Params;
	GBIF_Detector_Properties Properties;

	BOOL bSelfInit = TRUE;
	long lOpenMode = HIS_GbIF_IP;
	long lPacketDelay = 256;

	int i=0;
	char* pTest = NULL;
	unsigned int dwRows=0, dwColumns=0;

	// First we tell the system to enumerate all available sensors
	// * to initialize them in polling mode, set bEnableIRQ = FALSE;
	// * otherwise, to enable interrupt support, set bEnableIRQ = TRUE;
	BOOL bEnableIRQ = TRUE;
	ACQDESCPOS Pos = 0;
	if (bGigETest)
	{	
		uiNumSensors = 0; 
	
		// find GbIF Detectors in Subnet	
		iRet = Acquisition_GbIF_GetDeviceCnt(&ulNumSensors);
		if (iRet != HIS_ALL_OK)
		{
			sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_GbIF_GetDetectorCnt",iRet);
			WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
			return iRet;
		}
		else
		{
			sprintf(strBuffer,"%s\t\t\t\tPASS!\n","Acquisition_GbIF_GetDetectorCnt");
			WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
		}
				
		if(ulNumSensors>0)
		{
			// get device params of GbIF Detectors in Subnet
			GBIF_DEVICE_PARAM* pGbIF_DEVICE_PARAM = (GBIF_DEVICE_PARAM*)malloc( sizeof(GBIF_DEVICE_PARAM)*(ulNumSensors));
			
			iRet = Acquisition_GbIF_GetDeviceList(pGbIF_DEVICE_PARAM,ulNumSensors);
			if (iRet != HIS_ALL_OK)
			{
				sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_GbIF_GetDeviceList",iRet);
				WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
				return iRet;
			}
			else
			{
				sprintf(strBuffer,"%s\t\t\t\tPASS!\n","Acquisition_GbIF_GetDeviceList");
			}
			
			WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

			sprintf(strBuffer,"Select Sensor Nr:\n");
			WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
			
			for (iCnt = 0 ; iCnt < ulNumSensors; iCnt++)
			{
				sprintf(strBuffer,"%d - %s\n",iCnt,(pGbIF_DEVICE_PARAM[iCnt]).cDeviceName);
				WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
			}
			iSelected = 0;
			//scanf("%d",&iSelected);
			sprintf(strBuffer,"%d - selected\n",iSelected);
			WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

			if (iSelected>-1 && iSelected<ulNumSensors)
			{
				//try to init detector
				uiNumSensors = 0;
			
				// convert ip to string
				pTest = (char*)malloc(256*sizeof(char));

				iRet = Acquisition_GbIF_Init(
					&hAcqDesc,
					//iSelected,							// Index to access individual detector
					0,										// here set to zero for a single detector device
					bEnableIRQ, 
					dwRows, dwColumns,						// Image dimensions
					bSelfInit,								// retrieve settings (rows,cols.. from detector
					FALSE,									// If communication port is already reserved by another process, do not open
					lOpenMode,								// here: HIS_GbIF_IP, i.e. open by IP address 
					pGbIF_DEVICE_PARAM[iSelected].ucIP		// IP address of the connection to open
					);

				if (iRet != HIS_ALL_OK)
				{
					sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_GbIF_Init",iRet);
					WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
					return iRet;
				}
				else
				{
					sprintf(strBuffer,"%s\t\t\t\tPASS!\n","Acquisition_GbIF_Init");

					// Calibrate connection
					if (Acquisition_GbIF_CheckNetworkSpeed(	hAcqDesc, &usTiming, &lPacketDelay, usNetworkLoadPercent)==HIS_ALL_OK)
					{
						sprintf(strBuffer,"%s result: suggested timing: %d packetdelay %d @%d networkload\t\t\t\n"
							,"Acquisition_GbIF_CheckNetworkSpeed",usTiming,lPacketDelay,usNetworkLoadPercent);
						WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

					}

					iRet = Acquisition_GbIF_SetPacketDelay(hAcqDesc,lPacketDelay);
					if (iRet != HIS_ALL_OK)
					{
						sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\n","Acquisition_GbIF_SetPacketDelay",iRet);
						WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
						return iRet;
					}
					else
					{
						sprintf(strBuffer,"%s %d\t\t\t\tPASS!\n","Acquisition_GbIF_SetPacketDelay",lPacketDelay);
					}
					WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

					// Get Detector Params of already opened GigE Detector
					iRet = Acquisition_GbIF_GetDeviceParams(hAcqDesc,&Params);
					if (iRet != HIS_ALL_OK)
					{
						sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\n","Acquisition_GBIF_GetDeviceParams",iRet);
						WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
						return iRet;
					}
					else
					{
						sprintf(strBuffer,"%s \t\t\t\tPASS!\n","Acquisition_GBIF_GetDeviceParams");
						sprintf(strBuffer, "Device Name: \t\t%s\nMAC: \t\t\t%s\nIP: \t\t\t%s\nSubnetMask: \t\t%s\nGateway: \t\t%s\nAdapterIP: \t\t%s\nAdapterSubnetMask: \t%s\nBootOptions Flag: \t%d\nGBIF Firmware: \t\t%s\n",
							Params.cDeviceName,
							Params.ucMacAddress,
							Params.ucIP, 
							Params.ucSubnetMask, 
							Params.ucGateway,
							Params.ucAdapterIP,
							Params.ucAdapterMask, 
							Params.dwIPCurrentBootOptions,
							Params.cGBIFFirmwareVersion
						);
						WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
					}

					// Read production data
					iRet = Acquisition_GbIF_GetDetectorProperties(hAcqDesc,&Properties);
					if (iRet != HIS_ALL_OK)
					{
						sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\n","Acquisition_GbIF_GetDetectorProperties",iRet);
						WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
						return -1;
					}
					else
					{
						sprintf(strBuffer,"%s \t\t\t\tPASS!\n","Acquisition_GbIF_GetDetectorProperties");
						sprintf(strBuffer, "Detector Type: \t\t%s\nManufDate: \t\t%s\nManufPlace: \t\t%s\n", Properties.cDetectorType,
							Properties.cManufacturingDate, Properties.cPlaceOfManufacture);
						WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
					}
					//
				}
			}
		}
		else
			return HIS_ERROR_NO_BOARD_IN_SUBNET;
	}
	else
	{
		//------------------------------------------------------------------------------------------------------------------//
		// Automatic Initialization:																						//
		// The XISL is able to recognize all sensors connected to the system automatically by its internal PROM settings.	//
		// In case of GigE connected detectors only point-to-point connected are opened with Enum-Sensors					//
		// The following code fragment shows the corresponding function call:												//
		//------------------------------------------------------------------------------------------------------------------//
		
		if ((iRet = Acquisition_EnumSensors(&uiNumSensors,	// Number of sensors
											1,				// Enable Interrupts
											FALSE			// If communication port is already reserved by another process, do not open. If this parameter is TRUE the XISL is capturing all communication port regardless if this port is already opened by other processes running on the system, which is only recommended for debug versions.
					 )
										   )!=HIS_ALL_OK)
		{
			sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_EnumSensors",iRet);
			WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
			return iRet;
		}
		else
		{
			sprintf(strBuffer,"%s\t\t\t\tPASS!\n","Acquisition_EnumSensors");
		}


		WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
		

	}

	sensorCount = uiNumSensors;
	if(selectedSensorNum>sensorCount-1)
		selectedSensorNum = sensorCount-1;
	// now we iterate through all this sensors
	// To start the loop Pos must be initialized by zero.
	do
	{
		if ((iRet = Acquisition_GetNextSensor(&Pos, &hAcqDesc))!=HIS_ALL_OK)
		{
			char szMsg[300];
			sprintf(szMsg, "Error nr.: %d", iRet);
			//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
			return HIS_ERROR_UNDEFINED;
		}
	
		//ask for communication device type and its number
		if ((iRet=Acquisition_GetCommChannel(hAcqDesc, &nChannelType, &nChannelNr))!=HIS_ALL_OK)
		{
			//error handling
			char szMsg[300];
			sprintf(szMsg, "Error nr.: %d", iRet);
			//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
			return HIS_ERROR_UNDEFINED;
		}
		sprintf(strBuffer, "channel type: %d, ChannelNr: %d\n", nChannelType, nChannelNr);
		WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

		if(i == selectedSensorNum) //select specific sensor
			*phAcqDesc = hAcqDesc;
		i++;
	}
	while (Pos!=0);

	return HIS_ALL_OK;
}
int CEVA_NDE_PerkinElmerFPD::init(HACQDESC &hAcqDesc,int &sensorCount,int &selectedSnesorNum)
{

	int nRet = HIS_ALL_OK;
	int bEnableIRQ;
	//DWORD *pPixelPtr = NULL;

	//INPUT_RECORD ir;
	//DWORD dwRead;
	//char szFileName[300];
	//FILE *pFile = NULL;
	//ACQDESCPOS Pos = 0;
	//int nChannelNr;
	//UINT nChannelType;

	//// variables for bad pixel correction
	//unsigned short* pwPixelMapData =NULL;
	//int* pCorrList = NULL;
	//int iListSize=0;

	//get an output handle to console
	hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOutput == INVALID_HANDLE_VALUE)
	{
		//error handling
		return HIS_ERROR_UNDEFINED;
	}

	hInput = GetStdHandle(STD_INPUT_HANDLE);
	if (hInput == INVALID_HANDLE_VALUE)
	{
		//error handling
		return HIS_ERROR_UNDEFINED;
	}
	//--------------------------------------------------------------------------------------------------//	
	// First we tell the system to enumerate all available sensors										//
	// * to initialize them in polling mode, set bEnableIRQ = FALSE;									//
	// * otherwise, to enable interrupt support, set bEnableIRQ = TRUE;									//
	//--------------------------------------------------------------------------------------------------//

	// This function is implemented above to give examples of the different ways to initialize your detector(s)

	nRet = DetectorInit(&hAcqDesc, 0, 0, 1,sensorCount,selectedSnesorNum);

	if (nRet!=HIS_ALL_OK)
	{
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		{closeDevice(); return HIS_ERROR_UNDEFINED;};
	}

	
	return nRet;
}
int CEVA_NDE_PerkinElmerFPD::openDevice(HACQDESC &hAcqDesc)
{
	int nRet = HIS_ALL_OK;

	int bEnableIRQ;	
	unsigned long timeoutCount;
	
	int nChannelNr;
	UINT nChannelType;

	DWORD dwDemoParam;

	//--------------------------------------------------------------------------------------------------//
	// For further description see Acquisition_GetNextSensor, Acquisition_GetCommChannel,				//
	// Acquisition_SetCallbacksAndMessages and Acquisition_GetConfiguration.							//
	// For the further steps we select the last recognized sensor to demonstrate the remaining tasks.	//
	//--------------------------------------------------------------------------------------------------//

	// We continue with one detector to show the other XISL features
	// short demo how to use the features of the 1621 if one is connected
		
// now we iterate through all this sensors, set further parameters (sensor size,
	// sorting scheme, system id, interrupt settings and so on) and extract information from every sensor.
	// To start the loop Pos must be initialized by zero.
	
	//ask for communication device type and its number
	if ((nRet=Acquisition_GetCommChannel(hAcqDesc, &nChannelType, &nChannelNr))!=HIS_ALL_OK)
	{
		//error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		return HIS_ERROR_UNDEFINED;
	}

	sprintf(strBuffer, "channel type: %d, ChannelNr: %d\n", nChannelType, nChannelNr);
	WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

	switch (nChannelType)
	{
	case HIS_BOARD_TYPE_ELTEC_XRD_FGE_Opto:
		sprintf(strBuffer,"%s%d\n","HIS_BOARD_TYPE_ELTEC_XRD_FGE_Opto:",nChannelType);
		break;
	case HIS_BOARD_TYPE_ELTEC_XRD_FGX:
		sprintf(strBuffer,"%s%d\n","HIS_BOARD_TYPE_ELTEC_XRD_FGX:",nChannelType);
		break;
	case HIS_BOARD_TYPE_ELTEC:
		sprintf(strBuffer,"%s%d\n","HIS_BOARD_TYPE_ELTEC:",nChannelType);
		break;
	case HIS_BOARD_TYPE_ELTEC_GbIF:
		sprintf(strBuffer,"%s%d\n","HIS_BOARD_TYPE_ELTEC_GbIF:",nChannelType);
		break;
	default:
		sprintf(strBuffer,"%s%d\n","Unknown ChanelType:",nChannelType);
		break;
	}
	WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

	//ask for data organization of all sensors
	if ((nRet=Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
		&dwColumns, &dwDataType, &dwSortFlags, &bEnableIRQ
			
		, &dwAcqType, 
		&dwSystemID, &dwSyncMode, &dwHwAccess))!=HIS_ALL_OK)
	{
		//error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		return HIS_ERROR_UNDEFINED;
	}
	
	sprintf(strBuffer, "frames: %d\n", dwFrames);
	WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
	sprintf(strBuffer, "rows: %d\ncolumns: %d\n", dwRows, dwColumns);
	WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);

	//set active fpd instance
	activeFPDins = this;
	// now set callbacks and messages for every sensor
	if ((nRet=Acquisition_SetCallbacksAndMessages(hAcqDesc, NULL, 0,
		0, OnEndFrameCallback, OnEndAcqCallback))!=HIS_ALL_OK)
	{
		//error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		{closeDevice(); return HIS_ERROR_UNDEFINED;};
	}		
	
	// check if optical framgrabber is used
	if (nChannelType==HIS_BOARD_TYPE_ELTEC_XRD_FGX
		||
		nChannelType==HIS_BOARD_TYPE_ELTEC_XRD_FGE_Opto) 
	{
		//create and receive FrameHeader
		CHwHeaderInfo Info; 
		CHwHeaderInfoEx InfoEx; // new Header 1621
		if ((nRet = Acquisition_GetHwHeaderInfoEx(hAcqDesc, &Info, &InfoEx))==HIS_ALL_OK)
		{
			// header could be retrieved
			
			//check if 1621 connected
			if (Info.dwHeaderID==14 && InfoEx.wCameratype>=1)
			{
				//--------------------------------------------------------------------------------------------------//
				// Acquiring Offset Images																			//
				// The sensor needs an offset correction to work properly. For this purpose the XISL provides a		//
				// special function Acquisition_Acquire_OffsetImage.												//
				// First we have to allocate a buffer for the offset data. The image size has to fit				//
				// the current image dimensions as the image data might be binned									//
				//--------------------------------------------------------------------------------------------------//
				unsigned short *pOffsetBufferBinning1=NULL,*pOffsetBufferBinning2=NULL;

				WORD wBinning=1;
				int timings = 8;
				//create lists to receive timings for different binning modes
				double* m_pTimingsListBinning1;
				double* m_pTimingsListBinning2;
				m_pTimingsListBinning1 = (double*) malloc(timings*sizeof(double));
				m_pTimingsListBinning2 = (double*) malloc(timings*sizeof(double));

				//  set detector timing and gain
				if ((nRet = Acquisition_SetCameraMode(hAcqDesc, 0))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				if ((nRet = Acquisition_SetCameraGain(hAcqDesc, 1))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				// set detector to default binning mode
				if ((nRet = Acquisition_SetCameraBinningMode(hAcqDesc,wBinning))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				// get int times for selected binning mode
				if ((nRet = Acquisition_GetIntTimes(hAcqDesc, m_pTimingsListBinning1, &timings))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				
				//ask for detector data organization to 
				if ((nRet = Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
					&dwColumns, &dwDataType, &dwSortFlags, &bEnableIRQ, &dwAcqType, 
					&dwSystemID, &dwSyncMode, &dwHwAccess))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				
				// get offsetfile
				// allocate memory for offset data for binning1
				pOffsetBufferBinning1 =(unsigned short *) malloc(dwRows*dwColumns*sizeof(short));


				// Pointers can be defined to be available in the EndFrameCallback function.
				// Let's define
				dwDemoParam = 0;
				// and pass its address to the XISL
#ifdef __X64	
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, &dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#else
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#endif				
				printf("\nAcquire offset data now!\n"); //valtest

				//hevEndAcq = CreateEvent(NULL, FALSE, FALSE, NULL);
				_isReady = 0;
				// acquire 13 dark frames and average them. The XISL automatically averages those frame to an offset image.
				if ((nRet = Acquisition_Acquire_OffsetImage(hAcqDesc, pOffsetBufferBinning1, dwRows, dwColumns, 13))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
					
				//wait for end of acquisition event
				//if (WaitForSingleObject(hevEndAcq, INFINITE)!=WAIT_OBJECT_0) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				timeoutCount = 100000;
				while(!_isReady && timeoutCount-->1) Sleep(2);
				if(timeoutCount<=1)	{closeDevice(); return HIS_ERROR_UNDEFINED;};
				// get gainimage is similar....

				// now change to other binning mode
				// set detector to default binning mode
				wBinning=2;
				// set detector to 2x2 binned mode
				if ((nRet =Acquisition_SetCameraBinningMode(hAcqDesc,wBinning))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				// get int times for selected binning mode
				if ((nRet =Acquisition_GetIntTimes(hAcqDesc, m_pTimingsListBinning2, &timings))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
								
				//ask for changed detector data organization
				if ((nRet =Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
					&dwColumns, &dwDataType, &dwSortFlags, &bEnableIRQ, &dwAcqType, 
					&dwSystemID, &dwSyncMode, &dwHwAccess))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				
				// get offsetfile
				// allocate memory for offset data for binning2 which has different numbers for rows and columns than binning 1. 
				// We retrieve the image dimensions for binning 2 by the above call of Acquisition_GetConfiguration
				pOffsetBufferBinning2 =(unsigned short *) malloc(dwRows*dwColumns*sizeof(short));

// Pointers can be defined to be available in the EndFrameCallback function.
// Let's define
				dwDemoParam = 0;
#ifdef __X64	
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, &dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#else
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#endif	
				
				printf("\nAcquire offset data now!\n"); 

				//hevEndAcq = CreateEvent(NULL, FALSE, FALSE, NULL);
				_isReady =0;
				// acquire 13 dark frames for binning2 and average them
				if ((nRet=Acquisition_Acquire_OffsetImage(hAcqDesc, pOffsetBufferBinning2, dwRows, dwColumns, 13))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				
				//wait for end of acquisition event
				//if (WaitForSingleObject(hevEndAcq, INFINITE)!=WAIT_OBJECT_0) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				// get gainimage is similar....
				timeoutCount = 100000;
				while(!_isReady && timeoutCount-->1) Sleep(2);
				if(timeoutCount<=1)	{closeDevice(); return HIS_ERROR_UNDEFINED;};

				// now back to default binning
				wBinning=1;
				if ((nRet=Acquisition_SetCameraBinningMode(hAcqDesc,wBinning))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
			
				// get offsetcorr averaged image binning1
				//ask for detector data organization
				if ((nRet=Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
					&dwColumns, &dwDataType, &dwSortFlags, &bEnableIRQ, &dwAcqType, 
					&dwSystemID, &dwSyncMode, &dwHwAccess))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				
				//	For acquisition we have to allocate an acquisition buffer with the image dimensions we received from Acquisition_GetConfiguration:
				pAcqBuffer = (WORD *)malloc(1*dwRows*dwColumns*sizeof(short));
				if (!pAcqBuffer) {closeDevice(); return HIS_ERROR_UNDEFINED;};

				dwDemoParam = ACQ_SNAP;
#ifdef __X64	
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, &dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#else
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#endif

				// After that we have to pass the address of our buffer to the XISL as well as the numbers of rows and columns:
				if ((nRet=Acquisition_DefineDestBuffers(hAcqDesc, pAcqBuffer,
					1, dwRows, dwColumns))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				_isReady = 0;
				if ((Acquisition_Acquire_Image(hAcqDesc,15,0, 
					HIS_SEQ_AVERAGE, pOffsetBufferBinning1, NULL, NULL))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};

				// wait for end of acquisition event
				// Note: In a windowed application you should post a message to you acquisition windows in your end of acquisition callback function.
				//if ( WaitForSingleObject(hevEndAcq, INFINITE)!=WAIT_OBJECT_0) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				timeoutCount = 100000;
				while(!_isReady && timeoutCount-->1) Sleep(2);
				if(timeoutCount<=1)	{closeDevice(); return HIS_ERROR_UNDEFINED;};
				free(pAcqBuffer);
				
				// get offsetcorr averaged image binning2
				// now back to binning 2
				wBinning=2;
				if ((nRet=Acquisition_SetCameraBinningMode(hAcqDesc,wBinning))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
			
				// get offsetcorr averaged image binning2
				//ask for detector data organization
				if ((nRet=Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
					&dwColumns, &dwDataType, &dwSortFlags, &bEnableIRQ, &dwAcqType, 
					&dwSystemID, &dwSyncMode, &dwHwAccess))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				
				//allocate acquisition buffer
				pAcqBuffer = (WORD *)malloc(1*dwRows*dwColumns*sizeof(short));
				if (!pAcqBuffer) {closeDevice(); return HIS_ERROR_UNDEFINED;};

				dwDemoParam = ACQ_SNAP;
#ifdef __X64
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#else
				if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
#endif
	
				if ((nRet=Acquisition_DefineDestBuffers(hAcqDesc, pAcqBuffer,
					1, dwRows, dwColumns))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};

				// Reset Framecounter to read out and display the framecnt in the endframecallback
				if ((nRet=Acquisition_ResetFrameCnt(hAcqDesc))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				_isReady = 0;
				if ((Acquisition_Acquire_Image(hAcqDesc,15,0, 
					HIS_SEQ_AVERAGE, pOffsetBufferBinning2, NULL, NULL))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};

				//wait for end of acquisition event
				//if ( WaitForSingleObject(hevEndAcq, INFINITE)!=WAIT_OBJECT_0) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				timeoutCount = 100000;
				while(!_isReady && timeoutCount-->1) Sleep(2);
				if(timeoutCount<=1)	{closeDevice(); return HIS_ERROR_UNDEFINED;};
				free(pAcqBuffer);
				free(pOffsetBufferBinning1);
				free(pOffsetBufferBinning2);

				// now back to default binning
				wBinning=1;
				if ((nRet=Acquisition_SetCameraBinningMode(hAcqDesc,wBinning))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
				//ask for detector data organization
				if ((nRet=Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
					&dwColumns, &dwDataType, &dwSortFlags, &bEnableIRQ, &dwAcqType, 
					&dwSystemID, &dwSyncMode, &dwHwAccess))!=HIS_ALL_OK) {closeDevice(); return HIS_ERROR_UNDEFINED;};
			}
		}
	}
	return nRet;
}
int CEVA_NDE_PerkinElmerFPD::refreshSettings(HACQDESC &hAcqDesc)
{
	DWORD time;
	int nRet = HIS_ALL_OK;
		// Timing 0 Internal Timer 500msec
	Acquisition_SetCameraMode(hAcqDesc,frameTiming_);
//	change of the syncmode from to internal trigger mode
	Acquisition_SetFrameSyncMode(hAcqDesc,syncMode_);
//	setting of the integration time in µs (integration time = read out time plus delay time)
	time=(DWORD)1000.0*exposureMs_;
	Acquisition_SetTimerSync(hAcqDesc,&time);
	exposureMs_ = time/1000.0;
	//set binning mode
	//if ((Acquisition_SetCameraBinningMode(hAcqDesc,binMode_))!=HIS_ALL_OK){nRet= Acquisition_SetCameraBinningMode(hAcqDesc,binMode_);}

	//comfirm settings
	nRet = Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
		&dwColumns, &dwDataType, &dwSortFlags, &dwIRQFlags, &dwAcqType, 
		&dwSystemID, &dwSyncMode, &dwHwAccess);
	if(dwRows!= image_height || dwColumns != image_width)
	{
		image_height = dwRows;
		image_width = dwColumns;
		ResizeImageBuffer();
	}
	syncMode_ = (long)dwSyncMode;
    char buf[MM::MaxStrLength];
    int ret = GetProperty("SyncMode", buf);
    string tmp1(buf);

    ret = GetProperty("FrameTiming", buf);
    string tmp2(buf);

	settingsID_ = tmp1+"_"+tmp2;

	if(nRet!=HIS_ALL_OK)
		return HIS_ERROR_UNDEFINED;

	return nRet;
}
int CEVA_NDE_PerkinElmerFPD::acquireImage(HACQDESC &hAcqDesc,unsigned short* pAcqBuffer)
{
	int nRet = HIS_ERROR_UNDEFINED;
	// -> board type is HIS_BOARD_TYPE_ELTEC or HIS_BOARD_TYPE_ELTEC_GbIF
	DWORD dwDemoParam;
	refreshSettings(hAcqDesc);

	dwFrames = acqFrameCount_;
	//allocate acquisition buffer
	//pAcqBuffer = (DWORD *)malloc(dwFrames*dwRows*dwColumns*sizeof(short));
	if (!pAcqBuffer)
	{
		//error handling
		//{closeDevice(); 
		return HIS_ERROR_UNDEFINED;//};
	}

	//--------------------------------------------------------------------------------------------------//
	// We create a scheduler event here that is used to block the main thread from execution after		//
	// starting the acquisition. In a normal windowed application you should skip this step and post a	//
	// message to your acquisition window instead of setting a scheduler event in the end of acquisition//
	// callback function.																				//
	//--------------------------------------------------------------------------------------------------//
	// create end of acquisition event
	//hevEndAcq = CreateEvent(NULL, FALSE, FALSE, NULL);
	_isReady = 0;
	//if (!hevEndAcq)
	//{
	//	//error handling
	//	{closeDevice(); return HIS_ERROR_UNDEFINED;};
	//}

	// Pointers can be defined to be available in the EndFrameCallback function.
	// Let's define
	dwDemoParam = ACQ_SNAP;
	// and pass its address to the XISL
#ifdef __X64
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam))!=HIS_ALL_OK) { return HIS_ERROR_UNDEFINED;};
#else
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) { return HIS_ERROR_UNDEFINED;};
#endif

	if ((nRet=Acquisition_DefineDestBuffers(hAcqDesc, pAcqBuffer,
		dwFrames, dwRows, dwColumns))!=HIS_ALL_OK)
	{
		//error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		//closeDevice(); 
		return HIS_ERROR_UNDEFINED;
	}

	nRet = HIS_ERROR_UNDEFINED;
	_isReady=0;

	//multi-gain mode
	if(snapMode_.compare(g_SnapMode_3)==0)
	{
		nRet=Acquisition_Acquire_Image_Ex(hAcqDesc,dwFrames,0, 
			HIS_SEQ_AVERAGE,pOffsetBufferInUse,multiGainSegmentCount_,pGainSeqBuffer,pGainSeqMedBuffer,NULL,(DWORD*)pCorrList);
	}
	else
	{
		nRet=Acquisition_Acquire_Image(hAcqDesc,dwFrames,0,
			HIS_SEQ_AVERAGE, pOffsetBufferInUse, pGainBufferInUse, (DWORD*)pCorrListInUse);
	}

	// now we acquire dwFrames (in this example dwFrames is 10) images
	if(nRet!=HIS_ALL_OK)
	{
		_isReady = 1;
		//error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		//closeDevice(); 
		return HIS_ERROR_UNDEFINED;
	}
	nRet = WaitForExposureDone();

	//abort acquisition
	Acquisition_Abort(hAcqDesc);

	if(nRet != HIS_ALL_OK)
		return HIS_ERROR_UNDEFINED;	

	sprintf(strBuffer, "I'm waiting\n");
	WriteConsole(hOutput, strBuffer, strlen(strBuffer), &dwCharsWritten, NULL);
	return nRet;
}
int CEVA_NDE_PerkinElmerFPD::reloadCalibrationData()
{
	int nRet = HIS_ALL_OK;
	bool offsetCal_ = false;
	bool gainCal_ = false;
	bool multiGainCal_ = false;
	bool pixelCal_ = false;

	//if(snapMode_.compare(g_SnapMode_0)==0)
	//	return nRet;

	char szFileName[300];
	FILE *pFile = NULL;

	//load offset map
	string prefix("Offset_");
	strcpy(szFileName, (prefix+settingsID_+".his").c_str());
	printf("Load Offset Image under filename:\n%s\n", szFileName);
	pFile = fopen(szFileName, "rb");
	if (pFile)
	{
		if(pOffsetBuffer)
			free(pOffsetBuffer);
		pOffsetBuffer = NULL;
		WinHeaderType FileHeader;
		WinImageHeaderType ImageHeader;
			
		fread(&FileHeader, sizeof(WinHeaderType), 1, pFile);
		//fseek(pFile,68,0);
		//fread(&ImageHeader, sizeof(WinImageHeaderType), 1, pFile);
		fseek(pFile,100,0);
		assert(FileHeader.BRX-FileHeader.ULX+1 == dwRows);
		assert(FileHeader.BRY-FileHeader.ULY+1 == dwColumns);
		pOffsetBuffer =(WORD *) malloc(dwRows*dwColumns*sizeof(WORD));
		fread(pOffsetBuffer, sizeof(WORD)*dwRows*dwColumns, 1, pFile);
		fclose(pFile);
		offsetCal_ = true;
	}
	else 
	{
		MessageBoxNonBlocking("Error loading Offset Calibration,you may need to perform Offset Calibration!");
		LogMessage("Error loading Offset file!\n");
		//return OFFSET_CALIBRATION_FAILED;
	}



	//load gain map
	string prefix2("Gain_");
	strcpy(szFileName, (prefix2+settingsID_+".his").c_str());
	printf("Load Gain Image under filename:\n%s\n", szFileName);
	pFile = fopen(szFileName, "rb");
	if (pFile)
	{
		if(pGainBuffer)
			free(pGainBuffer);
		pGainBuffer = NULL;
		WinHeaderType FileHeader;
		WinImageHeaderType ImageHeader;

		fread(&FileHeader, sizeof(WinHeaderType), 1, pFile);
		//fseek(pFile,68,0);
		//fread(&ImageHeader, sizeof(WinImageHeaderType), 1, pFile);
		fseek(pFile,100,0);
		assert(FileHeader.BRX-FileHeader.ULX+1 == dwRows);
		assert(FileHeader.BRY-FileHeader.ULY+1 == dwColumns);
		pGainBuffer =(DWORD *)  malloc(dwRows*dwColumns*sizeof(DWORD));
		fread(pGainBuffer, sizeof(DWORD)*dwRows*dwColumns, 1, pFile);
		fclose(pFile);
		gainCal_ = true;
	} 
	else
	{

		MessageBoxNonBlocking("Error loading Gain Calibration file,you may need to perform Gain Calibration!");
		LogMessage("Error loading Gain calibration file!\n");
		//return GAIN_CALIBRATION_FAILED;
		
	}
	//load gain map
	//--------------------------------------------------------------------------------------------------------------//
	// Pixel correction																								//
	//--------------------------------------------------------------------------------------------------------------//
	// create a defect pixel image or load the defect pixel image which is already exiting (e.g. from the PKI detector CD) 
	strcpy(szFileName, "pixel_correction.his");
	printf("Load pixel correction Image under filename:\n%s\n", szFileName);
	pFile = fopen(szFileName, "rb");
	if (pFile)
	{
		if (pPixelBuffer) 
			free(pPixelBuffer);
		pPixelBuffer = NULL;
		if (pCorrList) 
			free(pCorrList);
		pCorrList = NULL;
		int iListSize=0;
		WinHeaderType FileHeader;
		WinImageHeaderType ImageHeader;
		pPixelBuffer= (WORD *) malloc( dwRows*dwColumns*(sizeof (WORD)));
		fread(&FileHeader, sizeof(WinHeaderType), 1, pFile);
		//fseek(pFile,68,0);
		//fread(&ImageHeader, sizeof(WinImageHeaderType), 1, pFile);
		fseek(pFile,100,0);
		fread(pPixelBuffer, sizeof(WORD)*dwRows*dwColumns, 1, pFile);
		fclose(pFile);
		if(pCorrList)
			free(pCorrList);
		pCorrList = NULL;
		iListSize=0;
		// 1) retrieve the required size for the list "&iListSize"
		Acquisition_CreatePixelMap(pPixelBuffer, dwColumns, dwRows, pCorrList, &iListSize);
		// 2) Allocate the memory for the list
		pCorrList = (int *) malloc(iListSize);
		// 3) Import the data into the list
		Acquisition_CreatePixelMap(pPixelBuffer, dwColumns, dwRows, pCorrList, &iListSize);
		// 4a) Correct the already acquired data
		pixelCal_ = true;
	} 
	else
	{
		MessageBoxNonBlocking("Error loading Pixel Correction file,you may need to perform Pixel Correction!");
		LogMessage("Error loading pixel correction file!\n");
		//return GAIN_CALIBRATION_FAILED;
	}

	// refresh buffer in use
	SetProperty("SnapMode",snapMode_.c_str());
	if(pixelCorrection_)
		SetProperty("PixelCorrection","On");
	else
		SetProperty("PixelCorrection","Off");
	return HIS_ALL_OK;
}
int CEVA_NDE_PerkinElmerFPD::startOffsetCalibration(HACQDESC &hAcqDesc)
{
	char szFileName[300];
	FILE *pFile = NULL;
	int nRet = HIS_ALL_OK;
	DWORD dwDemoParam;

	refreshSettings(hAcqDesc);

	//allocate memory for offset data
	pOffsetBuffer =(WORD *) malloc(dwRows*dwColumns*sizeof(short));
	dwDemoParam = ACQ_OFFSET;
#ifdef __X64
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam))!=HIS_ALL_OK) return HIS_ERROR_UNDEFINED;
#else
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) return HIS_ERROR_UNDEFINED;
#endif
	printf("\nAcquire offset data now!\n"); //valtest

	_isReady = 0;
	//acquire 13 dark frames and average them
	if ((nRet=Acquisition_Acquire_OffsetImage(hAcqDesc, pOffsetBuffer, dwRows, dwColumns, offsetFrameCount_))!=HIS_ALL_OK)
	{
		//error handler
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		return OFFSET_CALIBRATION_FAILED;	
	}

	//wait for end of acquisition
	nRet = WaitForExposureDone();
	if(nRet != HIS_ALL_OK)
		return OFFSET_CALIBRATION_FAILED;	

	string prefix("Offset_");
	strcpy(szFileName, (prefix+settingsID_+".his").c_str());

	printf("Save Offset Image under filename:\n%s\n", szFileName);
	pFile = fopen(szFileName, "wb");
	if (pFile)
	{
		WinHeaderType FileHeader;
		byte nDummy = 0;
		memset(&FileHeader, 0, sizeof(WinHeaderType));
		FileHeader.FileType=0x7000;
		FileHeader.HeaderSize = sizeof(WinHeaderType);
		FileHeader.ImageHeaderSize = 32;
		FileHeader.FileSize = FileHeader.HeaderSize+FileHeader.ImageHeaderSize+dwRows*dwColumns*sizeof(WORD);
		FileHeader.ULX = FileHeader.ULY = 1;
		FileHeader.BRX = (WORD)dwColumns;
		FileHeader.BRY = (WORD)dwRows;
		FileHeader.NrOfFrames = 1;
		FileHeader.TypeOfNumbers = 4;//DATASHORT;
		FileHeader.IntegrationTime = 0;
		fwrite(&FileHeader, sizeof(WinHeaderType), 1, pFile);
		fwrite(&nDummy, sizeof(byte), 100-sizeof(WinHeaderType), pFile);
		fwrite(pOffsetBuffer, sizeof(WORD)*dwRows*dwColumns, 1, pFile);
		fclose(pFile);
	}
	else 
	{
		LogMessage("Error creating Offset file!\n");
		return OFFSET_CALIBRATION_FAILED;
	}

	//reloadCalibrationData();

	MessageBoxNonBlocking("Offset calibration done!");
	return HIS_ALL_OK;
}
int CEVA_NDE_PerkinElmerFPD::startGainCalibration(HACQDESC &hAcqDesc)
{
	char szFileName[300];
	FILE *pFile = NULL;
	DWORD dwDemoParam;
	int nRet = HIS_ALL_OK;

	refreshSettings(hAcqDesc);

	//--------------------------------------------------------------------------------------------------------------//
	// Acquiring Gain/Offset Data																					//
	// After we acquired offset images we are able to acquire gain images either. It isn’t possible to				//
	// acquire valid gain images without offset images. First we have to allocate the buffer for the gain			//
	// image. The XISL uses a 32 bit data format for gain images. That’s why the allocation step is a				//
	// little bit different from that above:																		//
	//--------------------------------------------------------------------------------------------------------------//
	pGainBuffer =(DWORD *) malloc(dwRows*dwColumns*sizeof(DWORD));
	dwDemoParam = ACQ_GAIN;
#ifdef __X64	
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam))!=HIS_ALL_OK) return HIS_ERROR_UNDEFINED;
#else
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) return HIS_ERROR_UNDEFINED;
#endif
	printf("\nAcquire gain data now!\n");

	_isReady = 0;
	//--------------------------------------------------------------------------------------------------------------//
	// Now the sensor must be illuminated, so that we can acquire the gain images:									//
	//--------------------------------------------------------------------------------------------------------------//
	// acquire 17 bright frames with exposed sensor and average them
	if ((nRet=Acquisition_Acquire_GainImage(hAcqDesc, pOffsetBuffer, pGainBuffer, dwRows, dwColumns, gainFrameCount_))!=HIS_ALL_OK)
	{
		//error handler
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		return GAIN_CALIBRATION_FAILED;
	}

	//wait for end of acquisition
	nRet = WaitForExposureDone();
	if(nRet != HIS_ALL_OK)
		return GAIN_CALIBRATION_FAILED;	

	string prefix("Gain_");
	strcpy(szFileName, (prefix+settingsID_+".his").c_str());
	printf("Save Gain Image under filename:\n%s\n", szFileName);
	pFile = fopen(szFileName, "wb");
	if (pFile)
	{
		WinHeaderType FileHeader;
		byte nDummy = 0;
		memset(&FileHeader, 0, sizeof(WinHeaderType));
		FileHeader.FileType=0x7000;
		FileHeader.HeaderSize = sizeof(WinHeaderType);
		FileHeader.ImageHeaderSize = 32;
		FileHeader.FileSize = FileHeader.HeaderSize+FileHeader.ImageHeaderSize+dwRows*dwColumns*sizeof(WORD);
		FileHeader.ULX = FileHeader.ULY = 1;
		FileHeader.BRX = (WORD)dwColumns;
		FileHeader.BRY = (WORD)dwRows;
		FileHeader.NrOfFrames = 1;
		FileHeader.TypeOfNumbers = 4;//DATASHORT;
		FileHeader.IntegrationTime = 0;
		fwrite(&FileHeader, sizeof(WinHeaderType), 1, pFile);
		fwrite(&nDummy, sizeof(byte), 100-sizeof(WinHeaderType), pFile);
		fwrite(pGainBuffer, sizeof(DWORD)*dwRows*dwColumns, 1, pFile);
		fclose(pFile);
	} 
	else
	{
		LogMessage("Error creating Gain file!\n");
		return GAIN_CALIBRATION_FAILED;
	}

	//reloadCalibrationData();

	MessageBoxNonBlocking("Gain calibration done!");
	return HIS_ALL_OK;
}
int CEVA_NDE_PerkinElmerFPD::startMultiGainCalibration(HACQDESC &hAcqDesc)
{

	char szFileName[300];
	FILE *pFile = NULL;
	DWORD dwDemoParam;
	int nRet = HIS_ALL_OK;

	refreshSettings(hAcqDesc);
	
	//----------------------------------------------------------------------------------------------------------------------//
	// Multi gain correction																								//
	// for the multi gain correction we have to:																			//
	// 1. create a sequence of offset-corrected bright images, each having a different intensity. In fact,					//
	// the intensity must increase with the image index in the sequence.													//
	// 2. create a list of Median values (gain map) representing the images of the above mentioned sequence					//
	// 3. Aquiere images with offset and multigain correction data linked													//
	//----------------------------------------------------------------------------------------------------------------------//

	dwFrames= multiGainSegmentCount_;
	pGainSeqBuffer =(WORD *) malloc(dwFrames*dwRows*dwColumns*sizeof(short));

	// first we acquire a fresh offset image
	dwDemoParam = ACQ_OFFSET;
#ifdef __X64	
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam))!=HIS_ALL_OK) return MULTI_GAIN_CALIBRATION_FAILED;
#else
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) return MULTI_GAIN_CALIBRATION_FAILED;
#endif

	_isReady=0;
	//acquire offset of averaged e.g. 50 dark frames
	if ((nRet=Acquisition_Acquire_OffsetImage(hAcqDesc, pOffsetBuffer, dwRows, dwColumns, offsetFrameCount_))!=HIS_ALL_OK)
	{
		//error handler
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		return MULTI_GAIN_CALIBRATION_FAILED;
	}
	//wait for end of acquisition
	nRet = WaitForExposureDone();
	if(nRet != HIS_ALL_OK)
		return GAIN_CALIBRATION_FAILED;	
	// buffer for offset corrected bright image
	pBrightocBuffer=(WORD *) malloc(dwRows*dwColumns*sizeof(short));

	dwDemoParam = ACQ_Brightoc;
#ifdef __X64	
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam))!=HIS_ALL_OK) return MULTI_GAIN_CALIBRATION_FAILED;
#else
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, dwDemoParam))!=HIS_ALL_OK) return MULTI_GAIN_CALIBRATION_FAILED;
#endif

	if ((nRet=Acquisition_DefineDestBuffers(hAcqDesc, pBrightocBuffer,
			1, dwRows, dwColumns))!=HIS_ALL_OK)
	{
		//error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		return MULTI_GAIN_CALIBRATION_FAILED;
	}

	for(int i=0;i<multiGainSegmentCount_;i++)
	{
		_isReady =0;
		// acquire offsetcorrected-avg bright image with low intensity
		if((Acquisition_Acquire_Image(hAcqDesc,multiGainFrameCount_,0, 
				HIS_SEQ_AVERAGE, pOffsetBuffer, NULL, NULL))!=HIS_ALL_OK)
		{
			//error handling
			char szMsg[300];
			sprintf(szMsg, "Error nr.: %d", nRet);
			//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
			return MULTI_GAIN_CALIBRATION_FAILED;
		}
		//wait for end of acquisition
		nRet = WaitForExposureDone();
		if(nRet != HIS_ALL_OK)
			return GAIN_CALIBRATION_FAILED;	
		// copy acquired data to sequence buffer
		memcpy(pGainSeqBuffer+i*dwRows*dwColumns,pBrightocBuffer,dwRows*dwColumns*sizeof(short));
	}
	// build buffer for median data to be used with the multi-gain correction
	pGainSeqMedBuffer=(WORD *) malloc(multiGainSegmentCount_*sizeof(short));
	
	// create gain map
	Acquisition_CreateGainMap(pGainSeqBuffer,pGainSeqMedBuffer,dwRows*dwColumns,multiGainSegmentCount_);


	MessageBoxNonBlocking("Multi-gain calibration done!");
	return HIS_ALL_OK;
}
int CEVA_NDE_PerkinElmerFPD::startPixelCorrection(HACQDESC &hAcqDesc)
{

	int nRet = HIS_ALL_OK;
	
	MessageBoxNonBlocking("Pixel correction done!");
	return nRet;
}
int CEVA_NDE_PerkinElmerFPD::closeDevice()
{
	int nRet = HIS_ALL_OK;
	//close acquisition and clean up
	//free event object
	//if (hevEndAcq) CloseHandle(hevEndAcq);
	//hevEndAcq = NULL;
	if (pAcqBuffer) free(pAcqBuffer);
	if (pOffsetBuffer) free(pOffsetBuffer);
	if (pGainBuffer) free(pGainBuffer);
	if (pPixelBuffer) free(pPixelBuffer);
	
	if ((nRet=Acquisition_CloseAll())!=HIS_ALL_OK)
	{
		//error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		//MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		return nRet;
	}

	if (hOutput)
	{
		CloseHandle(hOutput);
	}

	if (hInput)
	{
		CloseHandle(hInput);
	}
	return HIS_ALL_OK;
}


void CEVA_NDE_PerkinElmerFPD::TestResourceLocking(const bool recurse)
{
   MMThreadGuard g(*pEVA_NDE_PerkinElmerResourceLock_);
   if(recurse)
      TestResourceLocking(false);
}

int TransposeProcessor::Initialize()
{
   EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   if (pHub)
   {
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if( NULL != this->pTemp_)
   {
      free(pTemp_);
      pTemp_ = NULL;
      this->tempSize_ = 0;
   }
    CPropertyAction* pAct = new CPropertyAction (this, &TransposeProcessor::OnInPlaceAlgorithm);
   (void)CreateProperty("InPlaceAlgorithm", "0", MM::Integer, false, pAct); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int TransposeProcessor::OnInPlaceAlgorithm(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
    //  return HUB_UNKNOWN_ERR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(this->inPlace_?1L:0L);
   }
   else if (eAct == MM::AfterSet)
   {
      long ltmp;
      pProp->Get(ltmp);
      inPlace_ = (0==ltmp?false:true);
   }

   return DEVICE_OK;
}


int TransposeProcessor::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

   int ret = DEVICE_OK;
   // 
   if( width != height)
      return DEVICE_NOT_SUPPORTED; // problem with tranposing non-square images is that the image buffer
   // will need to be modified by the image processor.
   if(busy_)
      return DEVICE_ERR;
 
   busy_ = true;

   if( inPlace_)
   {
      if(  sizeof(unsigned char) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned char*)pBuffer, width);
      }
      else if( sizeof(unsigned short) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned short*)pBuffer, width);
      }
      else if( sizeof(unsigned long) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned long*)pBuffer, width);
      }
      else if( sizeof(unsigned long long) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned long long*)pBuffer, width);
      }
      else 
      {
         ret = DEVICE_NOT_SUPPORTED;
      }
   }
   else
   {
      if( sizeof(unsigned char) == byteDepth)
      {
         ret = TransposeRectangleOutOfPlace( (unsigned char*)pBuffer, width, height);
      }
      else if( sizeof(unsigned short) == byteDepth)
      {
         ret = TransposeRectangleOutOfPlace( (unsigned short*)pBuffer, width, height);
      }
      else if( sizeof(unsigned long) == byteDepth)
      {
         ret = TransposeRectangleOutOfPlace( (unsigned long*)pBuffer, width, height);
      }
      else if( sizeof(unsigned long long) == byteDepth)
      {
         ret =  TransposeRectangleOutOfPlace( (unsigned long long*)pBuffer, width, height);
      }
      else
      {
         ret =  DEVICE_NOT_SUPPORTED;
      }
   }
   busy_ = false;

   return ret;
}




int ImageFlipY::Initialize()
{
    CPropertyAction* pAct = new CPropertyAction (this, &ImageFlipY::OnPerformanceTiming);
    (void)CreateProperty("PeformanceTiming (microseconds)", "0", MM::Float, true, pAct); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int ImageFlipY::OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
      pProp->Set( performanceTiming_.getUsec());
   }
   else if (eAct == MM::AfterSet)
   {
      // -- it's ready only!
   }

   return DEVICE_OK;
}


int ImageFlipY::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   if(busy_)
      return DEVICE_ERR;

   int ret = DEVICE_OK;
 
   busy_ = true;
   performanceTiming_ = MM::MMTime(0.);
   MM::MMTime  s0 = GetCurrentMMTime();


   if( sizeof(unsigned char) == byteDepth)
   {
      ret = Flip( (unsigned char*)pBuffer, width, height);
   }
   else if( sizeof(unsigned short) == byteDepth)
   {
      ret = Flip( (unsigned short*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long) == byteDepth)
   {
      ret = Flip( (unsigned long*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long long) == byteDepth)
   {
      ret =  Flip( (unsigned long long*)pBuffer, width, height);
   }
   else
   {
      ret =  DEVICE_NOT_SUPPORTED;
   }

   performanceTiming_ = GetCurrentMMTime() - s0;
   busy_ = false;

   return ret;
}







///
int ImageFlipX::Initialize()
{
    CPropertyAction* pAct = new CPropertyAction (this, &ImageFlipX::OnPerformanceTiming);
    (void)CreateProperty("PeformanceTiming (microseconds)", "0", MM::Float, true, pAct); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int ImageFlipX::OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
      pProp->Set( performanceTiming_.getUsec());
   }
   else if (eAct == MM::AfterSet)
   {
      // -- it's ready only!
   }

   return DEVICE_OK;
}


int ImageFlipX::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   if(busy_)
      return DEVICE_ERR;

   int ret = DEVICE_OK;
 
   busy_ = true;
   performanceTiming_ = MM::MMTime(0.);
   MM::MMTime  s0 = GetCurrentMMTime();


   if( sizeof(unsigned char) == byteDepth)
   {
      ret = Flip( (unsigned char*)pBuffer, width, height);
   }
   else if( sizeof(unsigned short) == byteDepth)
   {
      ret = Flip( (unsigned short*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long) == byteDepth)
   {
      ret = Flip( (unsigned long*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long long) == byteDepth)
   {
      ret =  Flip( (unsigned long long*)pBuffer, width, height);
   }
   else
   {
      ret =  DEVICE_NOT_SUPPORTED;
   }

   performanceTiming_ = GetCurrentMMTime() - s0;
   busy_ = false;

   return ret;
}

///
int MedianFilter::Initialize()
{
    CPropertyAction* pAct = new CPropertyAction (this, &MedianFilter::OnPerformanceTiming);
    (void)CreateProperty("PeformanceTiming (microseconds)", "0", MM::Float, true, pAct); 
    (void)CreateProperty("BEWARE", "THIS FILTER MODIFIES DATA, EACH PIXEL IS REPLACED BY 3X3 NEIGHBORHOOD MEDIAN", MM::String, true); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int MedianFilter::OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
      pProp->Set( performanceTiming_.getUsec());
   }
   else if (eAct == MM::AfterSet)
   {
      // -- it's ready only!
   }

   return DEVICE_OK;
}


int MedianFilter::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   if(busy_)
      return DEVICE_ERR;

   int ret = DEVICE_OK;
 
   busy_ = true;
   performanceTiming_ = MM::MMTime(0.);
   MM::MMTime  s0 = GetCurrentMMTime();


   if( sizeof(unsigned char) == byteDepth)
   {
      ret = Filter( (unsigned char*)pBuffer, width, height);
   }
   else if( sizeof(unsigned short) == byteDepth)
   {
      ret = Filter( (unsigned short*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long) == byteDepth)
   {
      ret = Filter( (unsigned long*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long long) == byteDepth)
   {
      ret =  Filter( (unsigned long long*)pBuffer, width, height);
   }
   else
   {
      ret =  DEVICE_NOT_SUPPORTED;
   }

   performanceTiming_ = GetCurrentMMTime() - s0;
   busy_ = false;

   return ret;
}


int EVA_NDE_PerkinElmerHub::Initialize()
{
   //EVA_NDE_PerkinElmerHub* pHub = static_cast<EVA_NDE_PerkinElmerHub*>(GetParentHub());
   //if (!pHub)
   //   return HUB_UNKNOWN_ERR;

  	initialized_ = true;
 
	return DEVICE_OK;
}

int EVA_NDE_PerkinElmerHub::DetectInstalledDevices()
{  
   ClearInstalledDevices();

   // make sure this method is called before we look for available devices
   InitializeModuleData();

   char hubName[MM::MaxStrLength];
   GetName(hubName); // this device name
   for (unsigned i=0; i<GetNumberOfDevices(); i++)
   { 
      char deviceName[MM::MaxStrLength];
      bool success = GetDeviceName(i, deviceName, MM::MaxStrLength);
      if (success && (strcmp(hubName, deviceName) != 0))
      {
         MM::Device* pDev = CreateDevice(deviceName);
         AddInstalledDevice(pDev);
      }
   }
   return DEVICE_OK; 
}

MM::Device* EVA_NDE_PerkinElmerHub::CreatePeripheralDevice(const char* adapterName)
{
   for (unsigned i=0; i<GetNumberOfInstalledDevices(); i++)
   {
      MM::Device* d = GetInstalledDevice(i);
      char name[MM::MaxStrLength];
      d->GetName(name);
      if (strcmp(adapterName, name) == 0)
         return CreateDevice(adapterName);

   }
   return DEVICE_OK; // adapter name not found
}


void EVA_NDE_PerkinElmerHub::GetName(char* pName) const
{
   CDeviceUtils::CopyLimitedString(pName, g_HubDeviceName);
}

int EVA_NDE_PerkinElmerHub::OnErrorRate(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   // Don't simulate an error here!!!!

   if (eAct == MM::AfterSet)
   {
      pProp->Get(errorRate_);

   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(errorRate_);
   }
   return DEVICE_OK;
}

int EVA_NDE_PerkinElmerHub::OnDivideOneByMe(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   // Don't simulate an error here!!!!

   if (eAct == MM::AfterSet)
   {
      pProp->Get(divideOneByMe_);
      static long result = 0;
      bool crashtest = CDeviceUtils::CheckEnvironment("MICROMANAGERCRASHTEST");
      if((0 != divideOneByMe_) || crashtest)
         result = 1/divideOneByMe_;
      result = result;

   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(divideOneByMe_);
   }
   return DEVICE_OK;
}


