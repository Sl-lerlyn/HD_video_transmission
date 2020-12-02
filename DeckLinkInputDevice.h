#pragma once

#include <atomic>
#include <functional>
#include <QString>

#include "DeckLinkAPI.h"
#include "com_ptr.h"
#include "CapturePreviewEvents.h"
#include "AncillaryDataTable.h"

class DeckLinkInputDevice : public IDeckLinkInputCallback
{
	using DisplayModeQueryFunc = std::function<void(com_ptr<IDeckLinkDisplayMode>&)>;

public:
	DeckLinkInputDevice(QObject* owner, com_ptr<IDeckLink>& deckLink);
	virtual ~DeckLinkInputDevice() = default;

	bool						Init();

	const QString&				getDeviceName() const { return m_deviceName; }
	bool						isCapturing() const { return m_currentlyCapturing; }
	bool						supportsFormatDetection() const { return m_supportsFormatDetection; }
	BMDVideoConnection			getVideoConnections() const { return (BMDVideoConnection) m_supportedInputConnections; }
	void						queryDisplayModes(DisplayModeQueryFunc func);

	bool						startCapture(BMDDisplayMode displayMode, IDeckLinkScreenPreviewCallback* screenPreviewCallback, bool applyDetectedInputMode);
	void						stopCapture(void);

	com_ptr<IDeckLink>					getDeckLinkInstance() const { return m_deckLink; }
	com_ptr<IDeckLinkInput>				getDeckLinkInput() const { return m_deckLinkInput; }
	com_ptr<IDeckLinkConfiguration>		getDeckLinkConfiguration() const { return m_deckLinkConfig; }
	com_ptr<IDeckLinkProfileManager>	getProfileManager() const { return m_deckLinkProfileManager; }

	// IUnknown interface
	HRESULT		QueryInterface (REFIID iid, LPVOID *ppv) override;
	ULONG		AddRef(void) override;
	ULONG		Release(void) override;

	// IDeckLinkInputCallback interface
	HRESULT		VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode *newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags) override;
	HRESULT		VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioPacket) override;

private:
	QObject*							m_owner;
	std::atomic<ULONG>					m_refCount;
	//
	QString								m_deviceName;
	com_ptr<IDeckLink>					m_deckLink;
	com_ptr<IDeckLinkInput>				m_deckLinkInput;
	com_ptr<IDeckLinkConfiguration>		m_deckLinkConfig;
	com_ptr<IDeckLinkHDMIInputEDID>		m_deckLinkHDMIInputEDID;
	com_ptr<IDeckLinkProfileManager>	m_deckLinkProfileManager;

	bool								m_supportsFormatDetection;
	bool								m_currentlyCapturing;
	bool								m_applyDetectedInputMode;
	int64_t								m_supportedInputConnections;
	//
	static void	GetAncillaryDataFromFrame(IDeckLinkVideoInputFrame* frame, BMDTimecodeFormat format, QString* timecodeString, QString* userBitsString);
	static void	GetMetadataFromFrame(IDeckLinkVideoInputFrame* videoFrame, MetadataStruct* metadata);
};

class DeckLinkInputFormatChangedEvent : public QEvent
{
public:
	DeckLinkInputFormatChangedEvent(BMDDisplayMode displayMode);
	virtual ~DeckLinkInputFormatChangedEvent() {}

	BMDDisplayMode DisplayMode() const { return m_displayMode; }

private:
	BMDDisplayMode m_displayMode;
};

class DeckLinkInputFrameArrivedEvent : public QEvent
{
public:
	DeckLinkInputFrameArrivedEvent(AncillaryDataStruct* ancillaryData, MetadataStruct* metadata, bool signalValid);
	virtual ~DeckLinkInputFrameArrivedEvent() {}

	AncillaryDataStruct*	AncillaryData(void) const { return m_ancillaryData; }
	MetadataStruct*			Metadata(void) const { return m_metadata; }
	bool					SignalValid(void) const { return m_signalValid; }

private:
	AncillaryDataStruct*	m_ancillaryData;
	MetadataStruct*			m_metadata;
	bool					m_signalValid;
};

