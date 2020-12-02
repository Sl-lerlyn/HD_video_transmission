#pragma once

#include <QEvent>
#include <QMainWindow>
#include <QWidget>

#include "DeckLinkInputDevice.h"
#include "DeckLinkDeviceDiscovery.h"
#include "DeckLinkOpenGLWidget.h"
#include "AncillaryDataTable.h"
#include "ProfileCallback.h"

#include "ui_CapturePreview.h"

class CapturePreview : public QDialog
{
	Q_OBJECT

public:
	explicit CapturePreview(QWidget *parent = 0);
	virtual ~CapturePreview();

	void customEvent(QEvent* event);
	void closeEvent(QCloseEvent *event);

	void setup();
	void enableInterface(bool);

	void startCapture();
	void stopCapture();
	
	void refreshDisplayModeMenu(void);
	void refreshInputConnectionMenu(void);
	void addDevice(com_ptr<IDeckLink>& deckLink);
	void removeDevice(com_ptr<IDeckLink>& deckLink);
	void videoFormatChanged(BMDDisplayMode newDisplayMode);
	void haltStreams(void);
	void updateProfile(com_ptr<IDeckLinkProfile>& newProfile);

private:
	Ui::CapturePreviewDialog*		ui;
	QGridLayout*					layout;

	com_ptr<DeckLinkInputDevice>		m_selectedDevice;
	com_ptr<DeckLinkDeviceDiscovery>	m_deckLinkDiscovery;
	DeckLinkOpenGLWidget*				m_previewView;
	com_ptr<ProfileCallback>			m_profileCallback;
	AncillaryDataTable*					m_ancillaryDataTable;
	BMDVideoConnection					m_selectedInputConnection;

	std::map<intptr_t, com_ptr<DeckLinkInputDevice>>		m_inputDevices;

public slots:
	void inputDeviceChanged(int selectedDeviceIndex);
	void inputConnectionChanged(int selectedConnectionIndex);
	void toggleStart();
};
