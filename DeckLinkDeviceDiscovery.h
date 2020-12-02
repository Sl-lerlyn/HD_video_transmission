#pragma once

#include <QEvent>
#include <atomic>
#include "com_ptr.h"
#include "DeckLinkAPI.h"

class DeckLinkDeviceDiscovery : public IDeckLinkDeviceNotificationCallback
{
public:
	DeckLinkDeviceDiscovery(QObject* owner);
	virtual ~DeckLinkDeviceDiscovery();

	// IUnknown interface
	HRESULT		QueryInterface(REFIID iid, LPVOID *ppv) override;
	ULONG		AddRef() override;
	ULONG		Release() override;

	// IDeckLinkDeviceArrivalNotificationCallback interface
	HRESULT		DeckLinkDeviceArrived(IDeckLink* deckLinkDevice) override;
	HRESULT		DeckLinkDeviceRemoved(IDeckLink* deckLinkDevice) override;

	// Other methods
	bool		enable();
	void		disable();

private:
	com_ptr<IDeckLinkDiscovery>		m_deckLinkDiscovery;
	QObject*						m_owner;
	std::atomic<ULONG>				m_refCount;
};

class DeckLinkDeviceDiscoveryEvent : public QEvent
{
public:
	DeckLinkDeviceDiscoveryEvent(QEvent::Type type, IDeckLink* deckLinkDevice)
		: QEvent(type), m_deckLink(deckLinkDevice) { }
	virtual ~DeckLinkDeviceDiscoveryEvent() = default;

	com_ptr<IDeckLink> deckLink() const { return m_deckLink; }

private:
	com_ptr<IDeckLink> m_deckLink;
};
