#include <QCoreApplication>
#include <QMessageBox>
#include <QTextStream>
#include <iostream>
#include <stdio.h>

#include "com_ptr.h"
#include "DeckLinkInputDevice.h"
#include "ConnectToAgora.h"

DeckLinkInputDevice::DeckLinkInputDevice(QObject* owner, com_ptr<IDeckLink>& device) : 
	m_owner(owner),
	m_refCount(1),
	m_deckLink(device),
	m_deckLinkInput(IID_IDeckLinkInput, device),
	m_deckLinkConfig(IID_IDeckLinkConfiguration, device),
	m_deckLinkHDMIInputEDID(IID_IDeckLinkHDMIInputEDID, device),
	m_deckLinkProfileManager(IID_IDeckLinkProfileManager, device),  // Non-null when the device has > 1 profiles
	m_supportsFormatDetection(false),
	m_currentlyCapturing(false),
	m_applyDetectedInputMode(false),
	m_supportedInputConnections(0)
{
	m_deckLink->AddRef();
}

HRESULT	DeckLinkInputDevice::QueryInterface(REFIID iid, LPVOID *ppv)
{
	CFUUIDBytes		iunknown;
	HRESULT			result = E_NOINTERFACE;

	if (ppv == nullptr)
		return E_INVALIDARG;

	// Initialise the return result
	*ppv = nullptr;

	// Obtain the IUnknown interface and compare it the provided REFIID
	iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
	if (memcmp(&iid, &iunknown, sizeof(REFIID)) == 0)
	{
		*ppv = this;
		AddRef();
		result = S_OK;
	}
	else if (memcmp(&iid, &IID_IDeckLinkInputCallback, sizeof(REFIID)) == 0)
	{
		*ppv = (IDeckLinkInputCallback*)this;
		AddRef();
		result = S_OK;
	}

	return result;
}

ULONG DeckLinkInputDevice::AddRef(void)
{
	return ++m_refCount;
}

ULONG DeckLinkInputDevice::Release(void)
{
	ULONG newRefValue = --m_refCount;
	if (newRefValue == 0)
		delete this;

	return newRefValue;
}

bool DeckLinkInputDevice::Init()
{
	com_ptr<IDeckLinkProfileAttributes>		deckLinkAttributes(IID_IDeckLinkProfileAttributes, m_deckLink);
	const char*								deviceNameStr;

	if (!m_deckLinkInput)
	{
		// This may occur if device does not have input interface, for instance DeckLink Mini Monitor.
		return false;
	}
		
	// The configuration interface is valid until destructor to retain input connector setting
	if (!m_deckLinkConfig)
	{
		QMessageBox::critical(qobject_cast<QWidget*>(m_owner), "DeckLink Input initialization error", "Unable to query IDeckLinkConfiguration object interface");
		return false;
	}

	if (!deckLinkAttributes)
	{
		QMessageBox::critical(qobject_cast<QWidget*>(m_owner), "DeckLink Input initialization error", "Unable to query IDeckLinkProfileAttributes object interface");
		return false;
	}

	// Check if input mode detection is supported.
	if (deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &m_supportsFormatDetection) != S_OK)
		m_supportsFormatDetection = false;

	// Get the supported input connections for the device
	if (deckLinkAttributes->GetInt(BMDDeckLinkVideoInputConnections, &m_supportedInputConnections) != S_OK)
		m_supportedInputConnections = 0;
		
	// Enable all EDID functionality if possible
	if (m_deckLinkHDMIInputEDID)
	{
		int64_t allKnownRanges = bmdDynamicRangeSDR | bmdDynamicRangeHDRStaticPQ | bmdDynamicRangeHDRStaticHLG;
		m_deckLinkHDMIInputEDID->SetInt(bmdDeckLinkHDMIInputEDIDDynamicRange, allKnownRanges);
		m_deckLinkHDMIInputEDID->WriteToEDID();
	}

	// Get device name
	if (m_deckLink->GetDisplayName(&deviceNameStr) == S_OK)
	{
		m_deviceName = deviceNameStr;
		free((void*)deviceNameStr);
	}
	else
	{
		m_deviceName = "DeckLink";
	}

	return true;
}

void DeckLinkInputDevice::queryDisplayModes(DisplayModeQueryFunc func)
{
	com_ptr<IDeckLinkDisplayModeIterator>	displayModeIterator;
	com_ptr<IDeckLinkDisplayMode>			displayMode;

	if (m_deckLinkInput->GetDisplayModeIterator(displayModeIterator.releaseAndGetAddressOf()) != S_OK)
		return;

	while (displayModeIterator->Next(displayMode.releaseAndGetAddressOf()) == S_OK)
	{
		func(displayMode);
	}
}

bool DeckLinkInputDevice::startCapture(BMDDisplayMode displayMode, IDeckLinkScreenPreviewCallback* screenPreviewCallback, bool applyDetectedInputMode)
{
	HRESULT				result;
	BMDVideoInputFlags	videoInputFlags = bmdVideoInputFlagDefault;

	m_applyDetectedInputMode = applyDetectedInputMode;

	// Enable input video mode detection if the device supports it
	if (m_supportsFormatDetection)
		videoInputFlags |=  bmdVideoInputEnableFormatDetection;

	// Set the screen preview
	m_deckLinkInput->SetScreenPreviewCallback(screenPreviewCallback);

	// Set capture callback
	m_deckLinkInput->SetCallback(this);

	// Set the video input mode
	result = m_deckLinkInput->EnableVideoInput(displayMode, bmdFormat8BitYUV, videoInputFlags);
	if (result != S_OK)
	{
		QMessageBox::critical(qobject_cast<QWidget*>(m_owner), "Error starting the capture", "This application was unable to select the chosen video mode. Perhaps, the selected device is currently in-use.");
		return false;
	}


    //add
    result = m_deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2);
    if (result != S_OK)
    {
        //QMessageBox::critical(qobject_cast<QWidget*>(m_owner), "Error starting the capture", "This application was unable to select the chosen video mode. Perhaps, the selected device is currently in-use.");
        return false;
    }
    //addend

	// Start the capture
	result = m_deckLinkInput->StartStreams();
	if (result != S_OK)
	{
		QMessageBox::critical(qobject_cast<QWidget*>(m_owner), "Error starting the capture", "This application was unable to start the capture. Perhaps, the selected device is currently in-use.");
		return false;
	}

	m_currentlyCapturing = true;

	return true;
}

void DeckLinkInputDevice::stopCapture()
{
	if (m_deckLinkInput)
	{
		// Stop the capture
		m_deckLinkInput->StopStreams();

		//
		m_deckLinkInput->SetScreenPreviewCallback(nullptr);

		// Delete capture callback
		m_deckLinkInput->SetCallback(nullptr);
	}

	m_currentlyCapturing = false;
}

HRESULT DeckLinkInputDevice::VideoInputFormatChanged (BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
	HRESULT 		result;
	BMDPixelFormat	pixelFormat = bmdFormat10BitYUV;

	// Unexpected callback when auto-detect mode not enabled
	if (!m_applyDetectedInputMode)
		return E_FAIL;;

	if (detectedSignalFlags & bmdDetectedVideoInputRGB444)
		pixelFormat = bmdFormat10BitRGB;

	// Stop the capture
	m_deckLinkInput->StopStreams();

	// Set the video input mode
	result = m_deckLinkInput->EnableVideoInput(newMode->GetDisplayMode(), pixelFormat, bmdVideoInputEnableFormatDetection);
	if (result != S_OK)
	{
		QMessageBox::critical(qobject_cast<QWidget*>(m_owner), "Error restarting the capture", "This application was unable to set new display mode");
		return result;
	}

	// Start the capture
	result = m_deckLinkInput->StartStreams();
	if (result != S_OK)
	{
		QMessageBox::critical(qobject_cast<QWidget*>(m_owner), "Error restarting the capture", "This application was unable to restart capture");
		return result;
	}

	// Notify UI of new display mode
	if ((m_owner != nullptr) && (notificationEvents & bmdVideoInputDisplayModeChanged))
		QCoreApplication::postEvent(m_owner, new DeckLinkInputFormatChangedEvent(newMode->GetDisplayMode()));
	
	return S_OK;
}

int yuyv_to_yuv420p(const unsigned char *in, unsigned char *out, unsigned int width, unsigned int height)
{
    unsigned char *y = out;
    unsigned char *u = out + width*height;
    unsigned char *v = out + width*height + width*height/4;

    unsigned int i,j;
    unsigned int base_h;
    unsigned int is_y = 1, is_u = 1;
    unsigned int y_index = 0, u_index = 0, v_index = 0;

    unsigned long yuv422_length = 2 * width * height;

    //序列为YU YV YU YV，一个yuv422帧的长度 width * height * 2 个字节
    //丢弃偶数行 u v
    for(i=0; i<yuv422_length; i+=2){

        *(y+y_index) = *(in+i);

        y_index++;

    }

    for(i=0; i<height; i+=2){
        base_h = i*width*2;
        for(j=base_h+1; j<base_h+width*2; j+=2){
            if(is_u){
                *(u+u_index) = *(in+j);
                u_index++;
                is_u = 0;
            }

            else{
                *(v+v_index) = *(in+j);
                v_index++;
                is_u = 1;
            }
        }
    }
    return 1;
}


HRESULT DeckLinkInputDevice::VideoInputFrameArrived (IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket*  audioPacket)
{
	bool					validFrame;
	AncillaryDataStruct*	ancillaryData;
	MetadataStruct*			metadata;

	if (videoFrame == nullptr)
		return S_OK;
    if (audioPacket == nullptr)
        return S_OK;

	validFrame = (videoFrame->GetFlags() & bmdFrameHasNoInputSource) == 0;

	// Get the various timecodes and userbits attached to this frame
	ancillaryData = new AncillaryDataStruct();
	GetAncillaryDataFromFrame(videoFrame, bmdTimecodeVITC,					&ancillaryData->vitcF1Timecode,		&ancillaryData->vitcF1UserBits);
	GetAncillaryDataFromFrame(videoFrame, bmdTimecodeVITCField2,			&ancillaryData->vitcF2Timecode,		&ancillaryData->vitcF2UserBits);
	GetAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC1,			&ancillaryData->rp188vitc1Timecode,	&ancillaryData->rp188vitc1UserBits);
	GetAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC2,			&ancillaryData->rp188vitc2Timecode,	&ancillaryData->rp188vitc2UserBits);
	GetAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188LTC,				&ancillaryData->rp188ltcTimecode,	&ancillaryData->rp188ltcUserBits);
	GetAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188HighFrameRate,	&ancillaryData->rp188hfrtcTimecode,	&ancillaryData->rp188hfrtcUserBits);

	metadata = new MetadataStruct();
	GetMetadataFromFrame(videoFrame, metadata);

    //begin*****************************************************************
    int frameSize = videoFrame->GetRowBytes() * videoFrame->GetHeight();

    void* buffer;
    unsigned char* mbuf = new unsigned char[frameSize];
    unsigned char* buf = new unsigned char[frameSize];
    HRESULT getBytes = videoFrame->GetBytes(&buffer);

    //uyvy422 to yuv422p
    for (int i=0; i<frameSize/2; i++){
        mbuf[i] = *(unsigned char*)(buffer+i*2+1);
    }
    for (int i=0; i<frameSize/4; i++){
        mbuf[i+frameSize/2] = *(unsigned char*)(buffer+i*4);
        mbuf[i+frameSize/4*3] = *(unsigned char*)(buffer+i*4+2);
    }

    //uyvy422 to yuyv422
    /*for (int i=0; i<frameSize; i++){
        if (i&1) mbuf[i] = *(unsigned char*)(buffer+i-1);
        else mbuf[i] = *(unsigned char*)(buffer+i+1);
    }

    yuyv_to_yuv420p(mbuf, buf, 1920, 1080);*/
    if (sendOneYuvFrame((void*)mbuf) > 0);  //printf("send one yuv frame success")


    //std::cout << audioPacket->GetSampleFrameCount() << std::endl;
    // 40ms numofchannel=2 detph=16
    audioPacket->GetBytes(&buffer);
    if (sendOnePcmFrame(buffer) > 0);  //printf("send one pcm frame success")


    /*FILE *out;
    out = fopen("test.yuv", "wb");
    fwrite(mbuf, 1, frameSize, out);
    fclose(out);
    std::cout << std::endl;*/

    delete[] mbuf;
    mbuf = nullptr;
    delete[] buf;
    buf = nullptr;

    //std::cout << "***********************************************" << std::endl;
    //end ***********************************************************



	// Update the UI with new Ancillary data
	if (m_owner != nullptr)
		QCoreApplication::postEvent(m_owner, new DeckLinkInputFrameArrivedEvent(ancillaryData, metadata, validFrame));
	else
	{
		delete ancillaryData;
		delete metadata;
	}

	return S_OK;
}

void DeckLinkInputDevice::GetAncillaryDataFromFrame(IDeckLinkVideoInputFrame* videoFrame, BMDTimecodeFormat timecodeFormat, QString* timecodeString, QString* userBitsString)
{
	com_ptr<IDeckLinkTimecode>		timecode;
	const char*						timecodeStr;
	BMDTimecodeUserBits				userBits	= 0;

	if ((videoFrame != nullptr) && (timecodeString != nullptr) && (userBitsString != nullptr)
		&& (videoFrame->GetTimecode(timecodeFormat, timecode.releaseAndGetAddressOf()) == S_OK))
	{
		if (timecode->GetString(&timecodeStr) == S_OK)
		{
			*timecodeString = timecodeStr;
			free((void*)timecodeStr);
		}
		else
		{
			*timecodeString = "";
		}

		timecode->GetTimecodeUserBits(&userBits);
		*userBitsString = QString("0x%1").arg(userBits, 8, 16, QChar('0'));
	}
	else
	{
		*timecodeString = "";
		*userBitsString = "";
	}
}

void DeckLinkInputDevice::GetMetadataFromFrame(IDeckLinkVideoInputFrame* videoFrame, MetadataStruct* metadata)
{
	com_ptr<IDeckLinkVideoFrameMetadataExtensions> metadataExtensions(IID_IDeckLinkVideoFrameMetadataExtensions, com_ptr<IDeckLinkVideoInputFrame>(videoFrame));

	metadata->electroOpticalTransferFunction = "";
	metadata->displayPrimariesRedX = "";
	metadata->displayPrimariesRedY = "";
	metadata->displayPrimariesGreenX = "";
	metadata->displayPrimariesGreenY = "";
	metadata->displayPrimariesBlueX = "";
	metadata->displayPrimariesBlueY = "";
	metadata->whitePointX = "";
	metadata->whitePointY = "";
	metadata->maxDisplayMasteringLuminance = "";
	metadata->minDisplayMasteringLuminance = "";
	metadata->maximumContentLightLevel = "";
	metadata->maximumFrameAverageLightLevel = "";
	metadata->colorspace = "";

	if (metadataExtensions)
	{
		double doubleValue = 0.0;
		int64_t intValue = 0;

		if (metadataExtensions->GetInt(bmdDeckLinkFrameMetadataHDRElectroOpticalTransferFunc, &intValue) == S_OK)
		{
			switch (intValue)
			{
			case 0:
				metadata->electroOpticalTransferFunction = "SDR";
				break;
			case 1:
				metadata->electroOpticalTransferFunction = "HDR";
				break;
			case 2:
				metadata->electroOpticalTransferFunction = "PQ (ST2084)";
				break;
			case 3:
				metadata->electroOpticalTransferFunction = "HLG";
				break;
			default:
				metadata->electroOpticalTransferFunction = QString("Unknown EOTF: %1").arg((int32_t)intValue);
				break;
			}
		}

		if (videoFrame->GetFlags() & bmdFrameContainsHDRMetadata)
		{
			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedX, &doubleValue) == S_OK)
				metadata->displayPrimariesRedX = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedY, &doubleValue) == S_OK)
				metadata->displayPrimariesRedY = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenX, &doubleValue) == S_OK)
				metadata->displayPrimariesGreenX = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenY, &doubleValue) == S_OK)
				metadata->displayPrimariesGreenY = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueX, &doubleValue) == S_OK)
				metadata->displayPrimariesBlueX = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueY, &doubleValue) == S_OK)
				metadata->displayPrimariesBlueY = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRWhitePointX, &doubleValue) == S_OK)
				metadata->whitePointX = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRWhitePointY, &doubleValue) == S_OK)
				metadata->whitePointY = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMaxDisplayMasteringLuminance, &doubleValue) == S_OK)
				metadata->maxDisplayMasteringLuminance = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMinDisplayMasteringLuminance, &doubleValue) == S_OK)
				metadata->minDisplayMasteringLuminance = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMaximumContentLightLevel, &doubleValue) == S_OK)
				metadata->maximumContentLightLevel = QString::number(doubleValue, 'f', 4);

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMaximumFrameAverageLightLevel, &doubleValue) == S_OK)
				metadata->maximumFrameAverageLightLevel = QString::number(doubleValue, 'f', 4);
		}

		if (metadataExtensions->GetInt(bmdDeckLinkFrameMetadataColorspace, &intValue) == S_OK)
		{
			switch (intValue)
			{
			case bmdColorspaceRec601:
				metadata->colorspace = "Rec.601";
				break;
			case bmdColorspaceRec709:
				metadata->colorspace = "Rec.709";
				break;
			case bmdColorspaceRec2020:
				metadata->colorspace = "Rec.2020";
				break;
			default:
				metadata->colorspace = QString("Unknown Colorspace: %1").arg((int32_t)intValue);
				break;
			}
		}
	}
}

DeckLinkInputFormatChangedEvent::DeckLinkInputFormatChangedEvent(BMDDisplayMode displayMode)
	: QEvent(kVideoFormatChangedEvent), m_displayMode(displayMode)
{
}

DeckLinkInputFrameArrivedEvent::DeckLinkInputFrameArrivedEvent(AncillaryDataStruct* ancillaryData, MetadataStruct* metadata, bool signalValid)
	: QEvent(kVideoFrameArrivedEvent), m_ancillaryData(ancillaryData), m_metadata(metadata), m_signalValid(signalValid)
{
}


