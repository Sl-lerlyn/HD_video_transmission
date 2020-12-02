#include <QCoreApplication>
#include "CapturePreviewEvents.h"
#include "DeckLinkDeviceDiscovery.h"

DeckLinkDeviceDiscovery::DeckLinkDeviceDiscovery(QObject* owner) : 
	m_owner(owner), 
	m_refCount(1)
{
	m_deckLinkDiscovery = CreateDeckLinkDiscoveryInstance();
}

DeckLinkDeviceDiscovery::~DeckLinkDeviceDiscovery()
{
	if (m_deckLinkDiscovery)
	{
		// Uninstall device arrival notifications 
		m_deckLinkDiscovery->UninstallDeviceNotifications();
	}
}

/// IUnknown methods

HRESULT DeckLinkDeviceDiscovery::QueryInterface(REFIID iid, LPVOID *ppv)
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
	else if (memcmp(&iid, &IID_IDeckLinkDeviceNotificationCallback, sizeof(REFIID)) == 0)
	{
		*ppv = (IDeckLinkDeviceNotificationCallback*)this;
		AddRef();
		result = S_OK;
	}

	return result;
}

ULONG DeckLinkDeviceDiscovery::AddRef(void)
{
	return ++m_refCount;
}

ULONG DeckLinkDeviceDiscovery::Release(void)
{
	ULONG newRefValue = --m_refCount;
	if (newRefValue == 0)
		delete this;

	return newRefValue;
}

/// IDeckLinkDeviceArrivalNotificationCallback methods

HRESULT DeckLinkDeviceDiscovery::DeckLinkDeviceArrived(/* in */ IDeckLink* deckLink)
{
	// Notify owner that device has been added
	QCoreApplication::postEvent(m_owner, new DeckLinkDeviceDiscoveryEvent(kAddDeviceEvent, deckLink ) );
	return S_OK;
}

HRESULT DeckLinkDeviceDiscovery::DeckLinkDeviceRemoved(/* in */ IDeckLink* deckLink)
{
	// Notify owner that device has been removed
	QCoreApplication::postEvent(m_owner, new DeckLinkDeviceDiscoveryEvent(kRemoveDeviceEvent, deckLink ) );
	return S_OK;
}

// Other methods

bool DeckLinkDeviceDiscovery::enable()
{
	HRESULT result = E_FAIL;

	// Install device arrival notifications
	if (m_deckLinkDiscovery)
		result = m_deckLinkDiscovery->InstallDeviceNotifications(this);

	return result == S_OK;
}

void DeckLinkDeviceDiscovery::disable()
{
	// Uninstall device arrival notifications
	if (m_deckLinkDiscovery)
		m_deckLinkDiscovery->UninstallDeviceNotifications();
}
