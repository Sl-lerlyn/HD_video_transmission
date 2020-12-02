#include <QMessageBox>
#include <iostream>
#include "CapturePreview.h"
#include "ui_CapturePreview.h"

// Video input connector map 
const QVector<QPair<BMDVideoConnection, QString>> kVideoInputConnections =
{
	qMakePair(bmdVideoConnectionSDI,		QString("SDI")),
	qMakePair(bmdVideoConnectionHDMI,		QString("HDMI")),
	qMakePair(bmdVideoConnectionOpticalSDI,	QString("Optical SDI")),
	qMakePair(bmdVideoConnectionComponent,	QString("Component")),
	qMakePair(bmdVideoConnectionComposite,	QString("Composite")),
	qMakePair(bmdVideoConnectionSVideo,		QString("S-Video")),
};


CapturePreview::CapturePreview(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CapturePreviewDialog),
	m_selectedDevice(nullptr),
	m_deckLinkDiscovery(nullptr),
	m_profileCallback(nullptr),
	m_selectedInputConnection(bmdVideoConnectionUnspecified)
{
	ui->setupUi(this);

	layout = new QGridLayout(ui->previewContainer);
	layout->setMargin(0);

	m_previewView = new DeckLinkOpenGLWidget(dynamic_cast<QWidget*>(this));
	m_previewView->resize(ui->previewContainer->size());
	m_previewView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	layout->addWidget(m_previewView, 0, 0, 0, 0);
	m_previewView->clear();

	m_ancillaryDataTable = new AncillaryDataTable(this);
	ui->ancillaryTableView->setModel(m_ancillaryDataTable);
	ui->ancillaryTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->ancillaryTableView->horizontalHeader()->setStretchLastSection(true);

	ui->invalidSignalLabel->setVisible(false);

	connect(ui->startButton, &QPushButton::clicked, this, &CapturePreview::toggleStart);
	connect(ui->inputDevicePopup, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CapturePreview::inputDeviceChanged);
	connect(ui->inputConnectionPopup, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CapturePreview::inputConnectionChanged);
	enableInterface(false);
	show();
}

CapturePreview::~CapturePreview()
{
	delete ui;
}

void CapturePreview::setup()
{
	// Create and initialise DeckLink device discovery
	m_deckLinkDiscovery = make_com_ptr<DeckLinkDeviceDiscovery>(this);
	if (m_deckLinkDiscovery)
	{
		if (!m_deckLinkDiscovery->enable())
		{
			QMessageBox::critical(this, "This application requires the DeckLink drivers installed.", "Please install the Blackmagic DeckLink drivers to use the features of this application.");
		}
	}

	// Create DeckLink profile callback objects
	m_profileCallback = make_com_ptr<ProfileCallback>(this);
	if (m_profileCallback)
		m_profileCallback->onProfileChanging(std::bind(&CapturePreview::haltStreams, this));
}

void CapturePreview::customEvent(QEvent *event)
{
	if (event->type() == kAddDeviceEvent)
	{
		DeckLinkDeviceDiscoveryEvent* discoveryEvent = dynamic_cast<DeckLinkDeviceDiscoveryEvent*>(event);
		com_ptr<IDeckLink> deckLink(discoveryEvent->deckLink());
		addDevice(deckLink);
	}
	else if (event->type() == kRemoveDeviceEvent)
	{
		DeckLinkDeviceDiscoveryEvent* discoveryEvent = dynamic_cast<DeckLinkDeviceDiscoveryEvent*>(event);
		com_ptr<IDeckLink> deckLink(discoveryEvent->deckLink());
		removeDevice(deckLink);
	}
	else if (event->type() == kVideoFormatChangedEvent)
	{
		DeckLinkInputFormatChangedEvent* formatEvent = dynamic_cast<DeckLinkInputFormatChangedEvent*>(event);
		videoFormatChanged(formatEvent->DisplayMode());
	}
	else if (event->type() == kVideoFrameArrivedEvent)
	{
		DeckLinkInputFrameArrivedEvent* frameArrivedEvent = dynamic_cast<DeckLinkInputFrameArrivedEvent*>(event);
		ui->invalidSignalLabel->setVisible(!frameArrivedEvent->SignalValid());
		m_ancillaryDataTable->UpdateFrameData(frameArrivedEvent->AncillaryData(), frameArrivedEvent->Metadata());
		delete frameArrivedEvent->AncillaryData();
		delete frameArrivedEvent->Metadata();
	}
	else if (event->type() == kProfileActivatedEvent)
	{
		ProfileActivatedEvent* profileEvent = dynamic_cast<ProfileActivatedEvent*>(event);
		com_ptr<IDeckLinkProfile> deckLinkProfile(profileEvent->deckLinkProfile());
		updateProfile(deckLinkProfile);
	}
}

void CapturePreview::closeEvent(QCloseEvent *)
{
	if (m_selectedDevice)
	{
		// Stop capturing
		if (m_selectedDevice->isCapturing())
			stopCapture();

		// Disable profile callback
		if (m_selectedDevice->getProfileManager())
			m_selectedDevice->getProfileManager()->SetCallback(nullptr);
	}

	// Disable DeckLink device discovery
	m_deckLinkDiscovery->disable();
}

void CapturePreview::toggleStart()
{
	if (!m_selectedDevice)
		return;
	
	if (!m_selectedDevice->isCapturing())
		startCapture();
	else
		stopCapture();
}

void CapturePreview::startCapture()
{
	BMDDisplayMode displayMode = bmdModeUnknown;
	bool applyDetectedInputMode = ui->autoDetectCheckBox->isChecked();

	QVariant v = ui->videoFormatPopup->itemData(ui->videoFormatPopup->currentIndex());
	displayMode = (BMDDisplayMode)v.value<unsigned int>();

	if (m_selectedDevice && 
		m_selectedDevice->startCapture(displayMode, m_previewView->delegate(), applyDetectedInputMode))
	{
		// Update UI
		ui->startButton->setText("Stop");
		enableInterface(false);
	}
}

void CapturePreview::stopCapture()
{
	if (m_selectedDevice)
		m_selectedDevice->stopCapture();

	// Update UI
	ui->invalidSignalLabel->setVisible(false);
	ui->startButton->setText("Start");
	enableInterface(true);
}

void CapturePreview::enableInterface(bool enable)
{
	// Set the enable state of capture preview properties elements
	for (auto& combobox : ui->propertiesGroupBox->findChildren<QComboBox*>())
	{
		combobox->setEnabled(enable);
	}
	ui->autoDetectCheckBox->setEnabled(enable);
}

void CapturePreview::refreshInputConnectionMenu(void)
{
	BMDVideoConnection		supportedConnections;
	int64_t					currentInputConnection;

	// Get the available input video connections for the device
	supportedConnections = m_selectedDevice->getVideoConnections();
	
	// Get the current selected input connection
	if (m_selectedDevice->getDeckLinkConfiguration()->GetInt(bmdDeckLinkConfigVideoInputConnection, &currentInputConnection) != S_OK)
	{
		currentInputConnection = bmdVideoConnectionUnspecified;
	}

	ui->inputConnectionPopup->clear();

	for (auto& inputConnection : kVideoInputConnections)
	{
		if (inputConnection.first & supportedConnections)
			ui->inputConnectionPopup->addItem(inputConnection.second, QVariant::fromValue((int64_t)inputConnection.first));

		if (inputConnection.first == (BMDVideoConnection)currentInputConnection)
			ui->inputConnectionPopup->setCurrentIndex(ui->inputConnectionPopup->count() - 1);
	}
}

void CapturePreview::refreshDisplayModeMenu(void)
{
	ui->videoFormatPopup->clear();
	
	m_selectedDevice->queryDisplayModes([this](com_ptr<IDeckLinkDisplayMode>& displayMode)
 	{
		// Populate the display mode menu with a list of display modes supported by the installed DeckLink card
		const char*			modeName;
		BMDDisplayMode		mode = displayMode->GetDisplayMode();
		
		if (displayMode->GetName(&modeName) == S_OK)
		{
			ui->videoFormatPopup->addItem(QString(modeName), QVariant::fromValue((uint64_t)mode));
			free((void*)modeName);
		}
	});

	ui->videoFormatPopup->setCurrentIndex(0);
	ui->startButton->setEnabled(ui->videoFormatPopup->count() != 0);
}

void CapturePreview::addDevice(com_ptr<IDeckLink>& deckLink)
{
	com_ptr<DeckLinkInputDevice> inputDevice = make_com_ptr<DeckLinkInputDevice>(this, deckLink);

	// Initialise new DeckLinkDevice object
	if (!inputDevice->Init())
	{
		// Device does not have IDeckLinkInput interface, eg it is a DeckLink Mini Monitor
		return;
	}

	// Store input device to map to maintain reference
	m_inputDevices[(intptr_t)deckLink.get()] = inputDevice;

	// Add this DeckLink input device to the device list
	ui->inputDevicePopup->addItem(inputDevice->getDeviceName(), QVariant::fromValue<intptr_t>((intptr_t)deckLink.get()));

	if (ui->inputDevicePopup->count() == 1)
	{
		// We have added our first item, refresh and enable UI
		ui->inputDevicePopup->setCurrentIndex(0);
		inputDeviceChanged(0);

		ui->startButton->setText("Start");
		enableInterface(true);
	}
}

void CapturePreview::removeDevice(com_ptr<IDeckLink>& deckLink)
{
	// If device to remove is selected device, stop capture if active
	if (m_selectedDevice->getDeckLinkInstance().get() == deckLink.get())
	{
		if (m_selectedDevice->isCapturing())
			m_selectedDevice->stopCapture();
		m_selectedDevice = nullptr;
	}

	// Find the combo box entry to remove .
	for (int i = 0; i < ui->inputDevicePopup->count(); ++i)
	{
		if (deckLink.get() == (IDeckLink*)(((QVariant)ui->inputDevicePopup->itemData(i)).value<void*>()))
		{
			ui->inputDevicePopup->removeItem(i);
			break;
		}
	}

	// Dereference input device by removing from list 
	auto iter = m_inputDevices.find((intptr_t)deckLink.get());
	if (iter != m_inputDevices.end())
	{
		m_inputDevices.erase(iter);
	}

	// Check how many devices are left
	if (ui->inputDevicePopup->count() == 0)
	{
		// We have removed the last device, disable the interface.
		enableInterface(false);
	}
	else if (!m_selectedDevice)
	{
		// The device that was removed was the one selected in the UI.
		// Select the first available device in the list and reset the UI.
		ui->inputDevicePopup->setCurrentIndex(0);
		inputDeviceChanged(0);
	}
}

void CapturePreview::videoFormatChanged(BMDDisplayMode newDisplayMode)
{
	// Update videoFormatPopup with auto-detected display mode
	for (int i = 0; i < ui->videoFormatPopup->count(); ++i)
	{
		if (((QVariant)ui->videoFormatPopup->itemData(i)).value<BMDDisplayMode>() == newDisplayMode)
		{
			ui->videoFormatPopup->setCurrentIndex(i);
			return;
		}
	}
}

void CapturePreview::haltStreams(void)
{
	// Profile is changing, stop capture if running
	if (m_selectedDevice && m_selectedDevice->isCapturing())
		stopCapture();
}

void CapturePreview::updateProfile(com_ptr<IDeckLinkProfile>& /* newProfile */)
{
	// Action as if new device selected to check whether device is active/inactive
	// This will subsequently update input connections and video modes combo boxes
	inputDeviceChanged(ui->inputDevicePopup->currentIndex());
}

void CapturePreview::inputDeviceChanged(int selectedDeviceIndex)
{
	if (selectedDeviceIndex == -1)
		return;

	// Disable profile callback for previous selected device
	if (m_selectedDevice && (m_selectedDevice->getProfileManager()))
		m_selectedDevice->getProfileManager()->SetCallback(nullptr);

	QVariant selectedDeviceVariant = ui->inputDevicePopup->itemData(selectedDeviceIndex);
	intptr_t deckLinkPtr = (intptr_t)selectedDeviceVariant.value<intptr_t>();

	// Find input device based on IDeckLink* object
	auto iter = m_inputDevices.find(deckLinkPtr);
	if (iter == m_inputDevices.end())
		return;

	m_selectedDevice = iter->second;

	// Register profile callback with newly selected device's profile manager
	if (m_selectedDevice)
	{
		com_ptr<IDeckLinkProfileAttributes> deckLinkAttributes(IID_IDeckLinkProfileAttributes, m_selectedDevice->getDeckLinkInstance());

		if (m_selectedDevice->getProfileManager())
			m_selectedDevice->getProfileManager()->SetCallback(m_profileCallback.get());

		// Query duplex mode attribute to check whether sub-device is active
		if (deckLinkAttributes)
		{
			int64_t		duplexMode;

			if ((deckLinkAttributes->GetInt(BMDDeckLinkDuplex, &duplexMode) == S_OK) &&
					(duplexMode != bmdDuplexInactive))
			{
				// Update the input connector popup menu
				refreshInputConnectionMenu();

				ui->autoDetectCheckBox->setEnabled(m_selectedDevice->supportsFormatDetection());
				ui->autoDetectCheckBox->setChecked(m_selectedDevice->supportsFormatDetection());
			}
			else
			{
				ui->inputConnectionPopup->clear();
				ui->videoFormatPopup->clear();
				ui->autoDetectCheckBox->setCheckState(Qt::Unchecked);
				ui->autoDetectCheckBox->setEnabled(false);
				ui->startButton->setEnabled(false);
			}
		}
	}
}

void CapturePreview::inputConnectionChanged(int selectedConnectionIndex)
{
	if (selectedConnectionIndex == -1)
		return;

	QVariant selectedConnectionVariant = ui->inputConnectionPopup->itemData(selectedConnectionIndex);
	m_selectedInputConnection = selectedConnectionVariant.value<BMDVideoConnection>();

	if (m_selectedDevice->getDeckLinkConfiguration()->SetInt(bmdDeckLinkConfigVideoInputConnection, (int64_t)m_selectedInputConnection) != S_OK)
	{
		QMessageBox::critical(this, "Input connection error", "Unable to set video input connector");
	}
	
	// Update the video mode popup menu
	refreshDisplayModeMenu();
}

