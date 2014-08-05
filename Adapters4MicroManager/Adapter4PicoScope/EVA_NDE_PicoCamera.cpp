///////////////////////////////////////////////////////////////////////////////
// FILE:          EVA_NDE_PicoCamera.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   The example implementation of the EVA_NDE_Pico camera.
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
// CVS:           $Id: EVA_NDE_PicoCamera.cpp 10842 2013-04-24 01:21:05Z mark $
//

#include "EVA_NDE_PicoCamera.h"
#include <cstdio>
#include <string>
#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
#include "../../MMCore/Error.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include "pico\PS3000Acon.c"
#define MAX_SAMPLE_LENGTH 3000000
#define WAIT_FOR_TRIGGER_FAIL_COUNT 2
using namespace std;
const double CEVA_NDE_PicoCamera::nominalPixelSizeUm_ = 1.0;
double g_IntensityFactor_ = 1.0;

// External names used used by the rest of the system
// to load particular device from the "EVA_NDE_PicoCamera.dll" library
const char* g_CameraDeviceName = "PicoCam";
const char* g_HubDeviceName = "PicoHub";
const char* g_DADeviceName = "PicoScopeGen";

// constants for naming pixel types (allowed values of the "PixelType" property)
const char* g_PixelType_8bit = "8bit";
const char* g_PixelType_16bit = "16bit";

// TODO: linux entry code

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
   RegisterDevice(g_CameraDeviceName, MM::CameraDevice, "EVA_NDE_Pico camera");
   RegisterDevice("MedianFilter", MM::ImageProcessorDevice, "MedianFilter");
   RegisterDevice(g_DADeviceName, MM::SignalIODevice, "EVA_NDE_Pico DA");
   RegisterDevice(g_HubDeviceName, MM::HubDevice, "EVA_NDE_Pico hub");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;

   // decide which device class to create based on the deviceName parameter
   if (strcmp(deviceName, g_CameraDeviceName) == 0)
   {
      // create camera
      return new CEVA_NDE_PicoCamera();
   }
   else if (strcmp(deviceName, g_DADeviceName) == 0)
   {
      // create DA
      return new EVA_NDE_PicoDA();
   }
   else if(strcmp(deviceName, "MedianFilter") == 0)
   {
      return new MedianFilter();
   }
   else if (strcmp(deviceName, g_HubDeviceName) == 0)
   {
	  return new EVA_NDE_PicoHub();
   }

   // ...supplied name not recognized
   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

///////////////////////////////////////////////////////////////////////////////
// CEVA_NDE_PicoCamera implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
* CEVA_NDE_PicoCamera constructor.
* Setup default all variables and create device properties required to exist
* before intialization. In this case, no such properties were required. All
* properties will be created in the Initialize() method.
*
* As a general guideline Micro-Manager devices do not access hardware in the
* the constructor. We should do as little as possible in the constructor and
* perform most of the initialization in the Initialize() method.
*/
CEVA_NDE_PicoCamera::CEVA_NDE_PicoCamera() :
   CCameraBase<CEVA_NDE_PicoCamera> (),
   dPhase_(0),
   initialized_(false),
   roiX_(0),
   roiY_(0),
   sequenceStartTime_(0),
   isSequenceable_(false),
   sequenceMaxLength_(100),
   sequenceRunning_(false),
   sequenceIndex_(0),
   binSize_(1),
   image_width(512),
   image_height(1),
   ccdT_ (0.0),
   nComponents_(1),
   pEVA_NDE_PicoResourceLock_(0),
   isExternalTriggerOn_(1),
   stopOnOverflow_(false),
   sampleOffset_(0),
   timeout_(5000)
{

   // call the base class method to set-up default error codes/messages
   InitializeDefaultErrorMessages();
   pEVA_NDE_PicoResourceLock_ = new MMThreadLock();
   thd_ = new MySequenceThread(this);
   // parent ID display
   CreateHubIDProperty();
}

/**
* CEVA_NDE_PicoCamera destructor.
* If this device used as intended within the Micro-Manager system,
* Shutdown() will be always called before the destructor. But in any case
* we need to make sure that all resources are properly released even if
* Shutdown() was not called.
*/
CEVA_NDE_PicoCamera::~CEVA_NDE_PicoCamera()
{
   StopSequenceAcquisition();
   delete thd_;
   delete pEVA_NDE_PicoResourceLock_;
}

/**
* Obtains device name.
* Required by the MM::Device API.
*/
void CEVA_NDE_PicoCamera::GetName(char* name) const
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
int CEVA_NDE_PicoCamera::Initialize()
{
   if (initialized_)
      return DEVICE_OK;

   EVA_NDE_PicoHub* pHub = static_cast<EVA_NDE_PicoHub*>(GetParentHub());
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
   nRet = CreateProperty(MM::g_Keyword_Description, "EVA_NDE_Pico Camera Device Adapter", MM::String, true);
   if (DEVICE_OK != nRet)
      return nRet;

   // CameraName
   nRet = CreateProperty(MM::g_Keyword_CameraName, "EVA_NDE_PicoCamera-MultiMode", MM::String, true);
   assert(nRet == DEVICE_OK);

   // CameraID
   nRet = CreateProperty(MM::g_Keyword_CameraID, "V1.0", MM::String, true);
   assert(nRet == DEVICE_OK);

   // binning
   CPropertyAction *pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnBinning);
   nRet = CreateProperty(MM::g_Keyword_Binning, "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   //RowCount
   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnRowCount);
   nRet = CreateProperty("RowCount", "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   nRet = SetAllowedBinning();
   if (nRet != DEVICE_OK)
      return nRet;

   // pixel type
   nRet = CreateProperty(MM::g_Keyword_PixelType, g_PixelType_16bit, MM::String, false);
   assert(nRet == DEVICE_OK);
   vector<string> pixelTypeValues;
   pixelTypeValues.push_back(g_PixelType_8bit);
   pixelTypeValues.push_back(g_PixelType_16bit); 
   SetProperty(MM::g_Keyword_PixelType, g_PixelType_16bit);
   nRet = SetAllowedValues(MM::g_Keyword_PixelType, pixelTypeValues);
   if (nRet != DEVICE_OK)
      return nRet;

   // exposure
   nRet = CreateProperty(MM::g_Keyword_Exposure, "10.0", MM::Float, false);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits(MM::g_Keyword_Exposure, 0, 10000);

   // timeout
   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnTimeoutMs);
   nRet = CreateProperty("timeoutMs", "500", MM::Integer, false,pAct);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits("timeoutMs", 10, 100000);

    // sample offset
   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnSampleOffset);
   nRet = CreateProperty("SampleOffset", "0", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);
   //SetPropertyLimits("SampleOffset", 0, MAX_SAMPLE_LENGTH-1000);

   //sample length
   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnSampleLength);
   nRet = CreateProperty("SampleLength", "10.0", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);
   //SetPropertyLimits("SampleLength", 0, MAX_SAMPLE_LENGTH);

   //timebase
   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnTimebase);
   nRet = CreateProperty("Timebase", "10", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   //timeInterval
   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnTimeInterval);
   nRet = CreateProperty("TimeIntervalNs", "0", MM::Integer, true, pAct);
   assert(nRet == DEVICE_OK);

   // camera gain
   nRet = CreateProperty(MM::g_Keyword_Gain, "0", MM::Integer, false);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits(MM::g_Keyword_Gain, -5, 8);

   // camera offset
   nRet = CreateProperty(MM::g_Keyword_Offset, "0", MM::Integer, false);
   assert(nRet == DEVICE_OK);

   // Whether or not to use exposure time sequencing
   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnIsSequenceable);
   std::string propName = "UseExposureSequences";
   CreateProperty(propName.c_str(), "No", MM::String, false, pAct);
   AddAllowedValue(propName.c_str(), "Yes");
   AddAllowedValue(propName.c_str(), "No");

   // Camera Status
   //pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnStatus);
   std::string statusPropName = "Status";
   CreateProperty(statusPropName.c_str(), "Idle", MM::String, false);

   // synchronize all properties
   // --------------------------
   nRet = UpdateStatus();
   if (nRet != DEVICE_OK)
      return nRet;

#ifdef TESTRESOURCELOCKING
   TestResourceLocking(true);
   LogMessage("TestResourceLocking OK",true);
#endif

   	status = OpenDevice(&unit);
	if(PICO_OK != status)
	return DEVICE_NOT_CONNECTED;
	picoInitBlock(&unit,sampleOffset_);

   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnInputRange);
   CreateProperty("InputRange", "0", MM::Integer, false, pAct);
   //add input range
   	for (int i = unit.firstRange; i <= unit.lastRange; i++) 
	{
		AddAllowedValue("InputRange",  CDeviceUtils::ConvertToString(inputRanges[i]));
	}
	short ch;
	for (ch = 0; ch < unit.channelCount ; ch++)
	{
	   char propNameBuf[30]= "Channel A";
	   propNameBuf[8] +=ch;

	   char propValBufON[30]= "A--ON";
	   propValBufON[0] +=ch;

	   char propValBufOFF[30]= "A--OFF";
	   propValBufOFF[0] +=ch;

	   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnChannelEnable);
	   CreateProperty(propNameBuf, propValBufON, MM::String, false, pAct);


	   AddAllowedValue(propNameBuf, propValBufON);
	   AddAllowedValue(propNameBuf, propValBufOFF);
	}

   pAct = new CPropertyAction (this, &CEVA_NDE_PicoCamera::OnExternalTrigger);
   propName = "ExternalTrigger";
   CreateProperty(propName.c_str(), "ON", MM::String, false, pAct);
   AddAllowedValue(propName.c_str(), "ON");
   AddAllowedValue(propName.c_str(), "OFF");

		   // setup the buffer
   // ----------------
   nRet = ResizeImageBuffer();
   if (nRet != DEVICE_OK)
      return nRet;
   // initialize image buffer
   GenerateEmptyImage(img_);

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
int CEVA_NDE_PicoCamera::Shutdown()
{

   CloseDevice(&unit);

   initialized_ = false;
   return DEVICE_OK;
}

/**
* Performs exposure and grabs a single image.
* This function should block during the actual exposure and return immediately afterwards 
* (i.e., before readout).  This behavior is needed for proper synchronization with the shutter.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PicoCamera::SnapImage()
{
   int ret = DEVICE_ERR;
	static int callCounter = 0;
	++callCounter;

 //  MM::MMTime readoutTime(PICO_RUM_TIME_OUT_US+10*img_.Height());
 //  //picoInitBlock(&unit);
 // // CollectBlockImmediate(&unit); 

   //rapid block mode
   try
   {
	   picoInitRapidBlock(&unit,sampleOffset_,timeout_,isExternalTriggerOn_);
   }
   catch( CMMError& e){
	   return DEVICE_ERR;
   }
   uint32_t nCompletedSamples;
   uint32_t nCompletedCaptures;
    try
   {
	    MMThreadGuard g(imgPixelsLock_);
		short* pBuf = (short*) const_cast<unsigned char*>(img_.GetPixels());
		ret = picoRunRapidBlock(&unit,img_.Height(),img_.Width() ,&nCompletedSamples,&nCompletedCaptures,pBuf);
   }
   catch( CMMError& e){
	   return DEVICE_ERR;
   }

   if(ret != PICO_OK)
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
const unsigned char* CEVA_NDE_PicoCamera::GetImageBuffer()
{

   MMThreadGuard g(imgPixelsLock_);	
   unsigned char *pB = (unsigned char*)(img_.GetPixels());
   return pB;
}
/**
* Returns pixel data for cameras with multiple channels.
* See description for GetImageBuffer() for details.
* Use this overloaded version for cameras with multiple channels
* When calling this function for a single channel camera, this function
* should return the content of the imagebuffer as returned by the function
* GetImageBuffer().  This behavior is implemented in the DeviceBase.
* When GetImageBuffer() is called for a multi-channel camera, the 
* camera adapter should return the ImageBuffer for the first channel
* @param channelNr Number of the channel for which the image data are requested.
*/
 const unsigned char* CEVA_NDE_PicoCamera::GetImageBuffer(unsigned channelNr)
 {
	MMThreadGuard g(imgPixelsLock_);	
   unsigned char *pB = (unsigned char*)(img_.GetPixels()+GetImageBufferSize()*channelNr);
   return pB;
 }
/**
* Returns image buffer X-size in pixels.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PicoCamera::GetImageWidth() const
{
   return img_.Width();
}

/**
* Returns image buffer Y-size in pixels.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PicoCamera::GetImageHeight() const
{
   return img_.Height();
}

/**
* Returns image buffer pixel depth in bytes.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PicoCamera::GetImageBytesPerPixel() const
{
	int byteDepth = 0;
	char buf[MM::MaxStrLength];
	int  ret = GetProperty(MM::g_Keyword_PixelType, buf);
    if (ret != DEVICE_OK)
      return ret;

	std::string pixelType(buf);
   if (pixelType.compare(g_PixelType_8bit) == 0)
   {
      byteDepth = 1;
   }
   else if (pixelType.compare(g_PixelType_16bit) == 0)
   {
      byteDepth = 2;
   }
   return byteDepth;//img_.Depth();
} 

/**
* Returns the bit depth (dynamic range) of the pixel.
* This does not affect the buffer size, it just gives the client application
* a guideline on how to interpret pixel values.
* Required by the MM::Camera API.
*/
unsigned CEVA_NDE_PicoCamera::GetBitDepth() const
{
   return GetImageBytesPerPixel()*8;
}

/**
* Returns the size in bytes of the image buffer.
* Required by the MM::Camera API.
*/
long CEVA_NDE_PicoCamera::GetImageBufferSize() const
{

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
* This EVA_NDE_Pico implementation ignores the position coordinates and just crops the buffer.
* @param x - top-left corner coordinate
* @param y - top-left corner coordinate
* @param xSize - width
* @param ySize - height
*/
int CEVA_NDE_PicoCamera::SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize)
{

   if (xSize == 0 && ySize == 0)
   {
      // effectively clear ROI
      //ResizeImageBuffer();
      roiX_ = 0;
      roiY_ = 0;
   }
   else
   {
      // apply ROI
      //img_.Resize(xSize, ySize);
	  //imgTmp_.Resize(xSize, ySize);
      roiX_ = x;
      roiY_ = y;
   }
   return DEVICE_OK;
}

/**
* Returns the actual dimensions of the current ROI.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PicoCamera::GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize)
{

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
int CEVA_NDE_PicoCamera::ClearROI()
{

   ResizeImageBuffer();
   roiX_ = 0;
   roiY_ = 0;
      
   return DEVICE_OK;
}
   /**
    * Multi-Channel cameras use this function to indicate how many channels they 
    * provide.  Single channel cameras do not need to override this
    */
   unsigned CEVA_NDE_PicoCamera::GetNumberOfChannels() const 
   {
	   unsigned count=0;
	   	for (int ch = 0; ch < unit.channelCount ; ch++)
		{
			if(unit.channelSettings[ch].enabled)
				count++;
		}
      return count;//(unsigned)unit.channelCount;
   }

   /**
    * Multi-channel cameras should provide names for their channels
    * Single cahnnel cameras do not need to override this default implementation
    */
    int CEVA_NDE_PicoCamera::GetChannelName(unsigned  channel, char* name)
   {
	   unsigned count=0;
	   char tmp[30] = "Channel A";
	   	 for (int ch = 0; ch < unit.channelCount ; ch++)
		{
			if(unit.channelSettings[ch].enabled)
			{
				if(channel == count){
					tmp[8] +=ch;
					break;
				}
				count++;
			}
		}
      CDeviceUtils::CopyLimitedString(name, tmp);
      return DEVICE_OK;
   }

/**
* Returns the current exposure setting in milliseconds.
* Required by the MM::Camera API.
*/
double CEVA_NDE_PicoCamera::GetExposure() const
{

   char buf[MM::MaxStrLength];
   int ret = GetProperty(MM::g_Keyword_Exposure, buf);
   if (ret != DEVICE_OK)
      return 0.0;
   return atof(buf);
}

/**
 * Returns the current exposure from a sequence and increases the sequence counter
 * Used for exposure sequences
 */
double CEVA_NDE_PicoCamera::GetSequenceExposure() 
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
void CEVA_NDE_PicoCamera::SetExposure(double exp)
{
   SetProperty(MM::g_Keyword_Exposure, CDeviceUtils::ConvertToString(exp));
   GetCoreCallback()->OnExposureChanged(this, exp);;
}

/**
* Returns the current binning factor.
* Required by the MM::Camera API.
*/
int CEVA_NDE_PicoCamera::GetBinning() const
{

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
int CEVA_NDE_PicoCamera::SetBinning(int binF)
{

   return SetProperty(MM::g_Keyword_Binning, CDeviceUtils::ConvertToString(binF));
}

/**
 * Clears the list of exposures used in sequences
 */
int CEVA_NDE_PicoCamera::ClearExposureSequence()
{
   exposureSequence_.clear();
   return DEVICE_OK;
}

/**
 * Adds an exposure to a list of exposures used in sequences
 */
int CEVA_NDE_PicoCamera::AddToExposureSequence(double exposureTime_ms) 
{
   exposureSequence_.push_back(exposureTime_ms);
   return DEVICE_OK;
}

int CEVA_NDE_PicoCamera::SetAllowedBinning() 
{

   vector<string> binValues;
   binValues.push_back("1");
   //binValues.push_back("2");

   LogMessage("Setting Allowed Binning settings", true);
   return SetAllowedValues(MM::g_Keyword_Binning, binValues);
}


/**
 * Required by the MM::Camera API
 * Please implement this yourself and do not rely on the base class implementation
 * The Base class implementation is deprecated and will be removed shortly
 */
int CEVA_NDE_PicoCamera::StartSequenceAcquisition(double interval) {

   return StartSequenceAcquisition(LONG_MAX, interval, false);            
}

/**                                                                       
* Stop and wait for the Sequence thread finished                                   
*/                                                                        
int CEVA_NDE_PicoCamera::StopSequenceAcquisition()                                     
{
   //if (IsCallbackRegistered())
   //{
   //}

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
int CEVA_NDE_PicoCamera::StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow)
{

   if (IsCapturing())
      return DEVICE_CAMERA_BUSY_ACQUIRING;

   int ret = GetCoreCallback()->PrepareForAcq(this);
   if (ret != DEVICE_OK)
      return ret;
   sequenceStartTime_ = GetCurrentMMTime();
   imageCounter_ = 0;

   //rapid block mode
	try
	{
		picoInitRapidBlock(&unit,sampleOffset_,timeout_,isExternalTriggerOn_);
	}
	catch( CMMError& e){
		return DEVICE_ERR;
	}

   thd_->Start(numImages,interval_ms);
   stopOnOverflow_ = stopOnOverflow;
   return DEVICE_OK;
}

/*
 * Inserts Image and MetaData into MMCore circular Buffer
 */
int CEVA_NDE_PicoCamera::InsertImage()
{

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

   // This method inserts a new image into the circular buffer (residing in MMCore)
   //int ret = GetCoreCallback()->InsertMultiChannel(this, pI, 1, w, h, b, &md ); // Inserting the md causes crash in debug builds
   //int c = GetNumberOfChannels();
   int ret = GetCoreCallback()->InsertImage(this, pI, w, h, b, md.Serialize().c_str());
 //    int ret=0;
	//for(int i=0;i<c;i++)
	//	ret = GetCoreCallback()->InsertMultiChannel(this, GetImageBuffer(i),i, w, h, b, &md);

   if (!stopOnOverflow_ && ret == DEVICE_BUFFER_OVERFLOW)
   {
      // do not stop on overflow - just reset the buffer
      GetCoreCallback()->ClearImageBuffer(this);
      // don't process this same image again...
      return GetCoreCallback()->InsertImage(this, pI, w, h, b, md.Serialize().c_str(), false);
	  //for(int i=0;i<c;i++)
		 //ret = GetCoreCallback()->InsertMultiChannel(this, GetImageBuffer(i),i, w, h, b, &md);
	  //return ret;
   } else
      return ret;

}

/*
 * Do actual capturing
 * Called from inside the thread  
 */
int CEVA_NDE_PicoCamera::ThreadRun (MM::MMTime startTime)
{

   int ret=DEVICE_ERR;

   MMThreadGuard g(imgPixelsLock_);
   short* pBuf = (short*) const_cast<unsigned char*>(img_.GetPixels());

   uint32_t nCompletedSamples;
   uint32_t nCompletedCaptures;
   try
   {
	   ret = picoRunRapidBlock(&unit,img_.Height(),img_.Width(),&nCompletedSamples,&nCompletedCaptures,pBuf);
   }
   catch( CMMError& e){
	   return DEVICE_ERR;
   }
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

bool CEVA_NDE_PicoCamera::IsCapturing() {
   return !thd_->IsStopped();
}

/*
 * called from the thread function before exit 
 */
void CEVA_NDE_PicoCamera::OnThreadExiting() throw()
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


MySequenceThread::MySequenceThread(CEVA_NDE_PicoCamera* pCam)
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
// CEVA_NDE_PicoCamera Action handlers
///////////////////////////////////////////////////////////////////////////////



//int CEVA_NDE_PicoCamera::OnSwitch(MM::PropertyBase* pProp, MM::ActionType eAct)
//{
   // use cached values
//   return DEVICE_OK;
//}

/**
* Handles "Binning" property.
*/
int CEVA_NDE_PicoCamera::OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   int ret = DEVICE_ERR;
   switch(eAct)
   {
   case MM::AfterSet:
      {
         if(IsCapturing())
            return DEVICE_CAMERA_BUSY_ACQUIRING;

         // the user just set the new value for the property, so we have to
         // apply this value to the 'hardware'.
         long binFactor;
         pProp->Get(binFactor);
		if(binFactor > 0 && binFactor < 10)
		{
			//img_.Resize(image_width/binFactor, image_height/binFactor);
			binSize_ = binFactor;
			ResizeImageBuffer();

			std::ostringstream os;
			os << binSize_;
			OnPropertyChanged("Binning", os.str().c_str());
			ret=DEVICE_OK;
		}
      }break;
   case MM::BeforeGet:
      {
         ret=DEVICE_OK;
			pProp->Set(binSize_);
      }break;
   }
   return ret; 
}
/**
* Handles "RowCount" property.
*/
int CEVA_NDE_PicoCamera::OnRowCount(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
		pProp->Set(image_height);
   }
   else if (eAct == MM::AfterSet)
   {
      long value;
      pProp->Get(value);
		if ( (value < 1) || (MAX_SAMPLE_LENGTH < value))
			return DEVICE_ERR;  // invalid image size
		if( value != image_height)
		{
			image_height = value;
			//img_.Resize(image_width/binSize_, image_height/binSize_);
			ResizeImageBuffer();
		}
   }
	return DEVICE_OK; 
}
/**
* Handles "Input Range" property.
*/
int CEVA_NDE_PicoCamera::OnInputRange(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::AfterSet)
   {
	  long value;
      pProp->Get(value);
		if ( (value < 1) || (MAX_SAMPLE_LENGTH < value))
			return DEVICE_ERR;  // invalid image size
		int indexFound = -1;
		for (int i = unit.firstRange; i <= unit.lastRange; i++) 
		{
			 if(value==inputRanges[i])
			 {
				 indexFound = i;
			 }
		}
		if(indexFound>0)
			picoSetVoltages(&unit,indexFound);
   }
   else if (eAct == MM::BeforeGet)
   {
	   pProp->Set((long)inputRanges[unit.channelSettings[0].range]);
   }

   return DEVICE_OK;
}
/**
* Handles "SampleLength" property.
*/
int CEVA_NDE_PicoCamera::OnSampleLength(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::AfterSet)
   {
	  long value;
      pProp->Get(value);
		if ( (value < 1) || (MAX_SAMPLE_LENGTH < value))
			return DEVICE_ERR;  // invalid image size
		if( value != image_width)
		{
			image_width = value;
			//img_.Resize(image_width/binSize_, image_height/binSize_);
			ResizeImageBuffer();
		}
   }
   else if (eAct == MM::BeforeGet)
   {
	pProp->Set(image_width);
   }

   return DEVICE_OK;
}

/**
* Handles "Timebase" property.
*/
int CEVA_NDE_PicoCamera::OnTimebase(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::AfterSet)
   {
	  long value;
      pProp->Get(value);
	  picoSetTimebase(&unit,value);
   }
   else if (eAct == MM::BeforeGet)
   {
	pProp->Set((long)timebase);
   }

   return DEVICE_OK;
}
/**
* Handles "TimeInterval" property.
*/
int CEVA_NDE_PicoCamera::OnTimeInterval(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
	pProp->Set((long)timeInterval);
   }

   return DEVICE_OK;
}/**
* Handles "TimeoutMs" property.
*/
int CEVA_NDE_PicoCamera::OnTimeoutMs(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::AfterSet)
   {
	  long value;
      pProp->Get(value);
	  timeout_=value;
   }
   else if (eAct == MM::BeforeGet)
   {
	pProp->Set(timeout_);
   }

   return DEVICE_OK;
}
/**
* Handles "SampleOffset" property.
*/
int CEVA_NDE_PicoCamera::OnSampleOffset(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::AfterSet)
   {
	  long value;
      pProp->Get(value);
	  sampleOffset_=value;
   }
   else if (eAct == MM::BeforeGet)
   {
	pProp->Set(sampleOffset_);
   }

   return DEVICE_OK;
}


int CEVA_NDE_PicoCamera::OnIsSequenceable(MM::PropertyBase* pProp, MM::ActionType eAct)
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

int CEVA_NDE_PicoCamera::OnExternalTrigger(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   std::string val = "OFF";
   if (eAct == MM::BeforeGet)
   {
      if (isExternalTriggerOn_) 
      {
         val = "ON";
      }
	  else
	  {
		 val = "OFF";
	  }
      pProp->Set(val.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      
      pProp->Get(val);
	  if (val.compare("ON")==0) 
      {
         isExternalTriggerOn_ = 1;
      }
	  else
	  {
		  isExternalTriggerOn_ = 0;
	  }
		try
	   {
		  picoInitRapidBlock(&unit,sampleOffset_,timeout_,isExternalTriggerOn_);
	   }
	   catch( CMMError& e){
		   return DEVICE_ERR;
	   }
   }
   return DEVICE_OK;
}
int CEVA_NDE_PicoCamera::OnChannelEnable(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   std::string val = "ON";
   if (eAct == MM::BeforeGet)
   {
      val = "";

      pProp->Set(val.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(val);
	  int ch = val.c_str()[0]-'A';
	  if(val.c_str()[4]=='N')
      {
		  unit.channelSettings[ch].enabled = 1;
      }
	  else
	  {
		  unit.channelSettings[ch].enabled = 0;
	  }
	  //to make sure there is a channel enabled
	  if(GetNumberOfChannels()<=0)
		  unit.channelSettings[0].enabled = 1;

	  int nRet = ResizeImageBuffer();
		   try
	   {
		  picoInitRapidBlock(&unit,sampleOffset_,timeout_,isExternalTriggerOn_);
	   }
	   catch( CMMError& e){
		   return DEVICE_ERR;
	   }
	   if (nRet != DEVICE_OK)
		  return nRet;
   }
   return DEVICE_OK;
}
///////////////////////////////////////////////////////////////////////////////
// Private CEVA_NDE_PicoCamera methods
///////////////////////////////////////////////////////////////////////////////

/**
* Sync internal image buffer size to the chosen property values.
*/
int CEVA_NDE_PicoCamera::ResizeImageBuffer()
{
   char buf[MM::MaxStrLength];
   int ret = GetProperty(MM::g_Keyword_Binning, buf);
   if (ret != DEVICE_OK)
      return ret;
   binSize_ = atol(buf);

   ret = GetProperty(MM::g_Keyword_PixelType, buf);
   if (ret != DEVICE_OK)
      return ret;

	std::string pixelType(buf);
	int byteDepth = 0;

   if (pixelType.compare(g_PixelType_8bit) == 0)
   {
      byteDepth = 1;
   }
   else if (pixelType.compare(g_PixelType_16bit) == 0)
   {
      byteDepth = 2;
   }

   //MMThreadGuard g(imgPixelsLock_);
   img_.Resize(image_width/binSize_, image_height/binSize_, byteDepth * GetNumberOfChannels());
   return DEVICE_OK;
}

void CEVA_NDE_PicoCamera::GenerateEmptyImage(ImgBuffer& img)
{
   MMThreadGuard g(imgPixelsLock_);
   if (img.Height() == 0 || img.Width() == 0 || img.Depth() == 0)
      return;
   unsigned char* pBuf = const_cast<unsigned char*>(img.GetPixels());
   memset(pBuf, 0, img.Height()*img.Width()*img.Depth());
}

void CEVA_NDE_PicoCamera::TestResourceLocking(const bool recurse)
{
   MMThreadGuard g(*pEVA_NDE_PicoResourceLock_);
   if(recurse)
      TestResourceLocking(false);
}



/****
* EVA_NDE_Pico DA device
*/

EVA_NDE_PicoDA::EVA_NDE_PicoDA () : 
volt_(0), 
gatedVolts_(0), 
open_(true),
sequenceRunning_(false),
sequenceIndex_(0),
sentSequence_(vector<double>()),
nascentSequence_(vector<double>())
{
   SetErrorText(HUB_NOT_AVAILABLE, "Hub is not available");
   SetErrorText(ERR_SEQUENCE_INACTIVE, "Sequence triggered, but sequence is not running");

   // parent ID display
   CreateHubIDProperty();
}

EVA_NDE_PicoDA::~EVA_NDE_PicoDA() {
}

void EVA_NDE_PicoDA::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_DADeviceName);
}

int EVA_NDE_PicoDA::Initialize()
{
   EVA_NDE_PicoHub* pHub = static_cast<EVA_NDE_PicoHub*>(GetParentHub());
   if (pHub)
   {
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   // Triggers to test sequence capabilities
   CPropertyAction* pAct = new CPropertyAction (this, &EVA_NDE_PicoDA::OnTrigger);
   CreateProperty("Trigger", "-", MM::String, false, pAct);
   AddAllowedValue("Trigger", "-");
   AddAllowedValue("Trigger", "+");

   pAct = new CPropertyAction(this, &EVA_NDE_PicoDA::OnVoltage);
   CreateProperty("Voltage", "0", MM::Float, false, pAct);
   SetPropertyLimits("Voltage", 0.0, 10.0);

   pAct = new CPropertyAction(this, &EVA_NDE_PicoDA::OnRealVoltage);
   CreateProperty("Real Voltage", "0", MM::Float, true, pAct);

   return DEVICE_OK;
}

int EVA_NDE_PicoDA::SetGateOpen(bool open) 
{
   open_ = open; 
   if (open_) 
      gatedVolts_ = volt_; 
   else 
      gatedVolts_ = 0;

   return DEVICE_OK;
}

int EVA_NDE_PicoDA::GetGateOpen(bool& open) 
{
   open = open_; 
   return DEVICE_OK;
}

int EVA_NDE_PicoDA::SetSignal(double volts) {

   volt_ = volts; 
   if (open_)
      gatedVolts_ = volts;
   stringstream s;
   s << "Voltage set to " << volts;
   LogMessage(s.str(), false);
   return DEVICE_OK;
}

int EVA_NDE_PicoDA::GetSignal(double& volts) 
{

   volts = volt_; 
   return DEVICE_OK;
}

int EVA_NDE_PicoDA::SendDASequence() 
{

   (const_cast<EVA_NDE_PicoDA*> (this))->SetSentSequence();
   return DEVICE_OK;
}

// private
void EVA_NDE_PicoDA::SetSentSequence()
{
   sentSequence_ = nascentSequence_;
   nascentSequence_.clear();
}

int EVA_NDE_PicoDA::ClearDASequence()
{

   nascentSequence_.clear();
   return DEVICE_OK;
}

int EVA_NDE_PicoDA::AddToDASequence(double voltage) {

   nascentSequence_.push_back(voltage);
   return DEVICE_OK;
}

int EVA_NDE_PicoDA::OnTrigger(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
      pProp->Set("-");
   } else if (eAct == MM::AfterSet) {
      if (!sequenceRunning_)
         return ERR_SEQUENCE_INACTIVE;
      std::string tr;
      pProp->Get(tr);
      if (tr == "+") {
         if (sequenceIndex_ < sentSequence_.size()) {
            double voltage = sentSequence_[sequenceIndex_];
            int ret = SetSignal(voltage);
            if (ret != DEVICE_OK)
               return ERR_IN_SEQUENCE;
            sequenceIndex_++;
            if (sequenceIndex_ >= sentSequence_.size()) {
               sequenceIndex_ = 0;
            }
         } else
         {
            return ERR_IN_SEQUENCE;
         }
      }
   }
   return DEVICE_OK;
}

int EVA_NDE_PicoDA::OnVoltage(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      double volts = 0.0;
      GetSignal(volts);
      pProp->Set(volts);
   }
   else if (eAct == MM::AfterSet)
   {
      double volts = 0.0;
      pProp->Get(volts);
      SetSignal(volts);
   }
   return DEVICE_OK;
}

int EVA_NDE_PicoDA::OnRealVoltage(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(gatedVolts_);
   }
   return DEVICE_OK;
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


int EVA_NDE_PicoHub::Initialize()
{

   SetErrorText(HUB_NOT_AVAILABLE, "Hub is not available");

   initialized_ = true;
 
	return DEVICE_OK;
}

int EVA_NDE_PicoHub::DetectInstalledDevices()
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

MM::Device* EVA_NDE_PicoHub::CreatePeripheralDevice(const char* adapterName)
{
   for (unsigned i=0; i<GetNumberOfInstalledDevices(); i++)
   {
      MM::Device* d = GetInstalledDevice(i);
      char name[MM::MaxStrLength];
      d->GetName(name);
      if (strcmp(adapterName, name) == 0)
         return CreateDevice(adapterName);

   }
   return 0; // adapter name not found
}


void EVA_NDE_PicoHub::GetName(char* pName) const
{
   CDeviceUtils::CopyLimitedString(pName, g_HubDeviceName);
}


