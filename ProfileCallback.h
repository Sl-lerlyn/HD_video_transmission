#pragma once

#include <atomic>
#include <functional>
#include "com_ptr.h"
#include "CapturePreviewEvents.h"
#include "DeckLinkAPI.h"

class ProfileCallback : public IDeckLinkProfileCallback
{
	using ProfileChangingCallback = std::function<void(void)>;

public:
	ProfileCallback(QObject* owner);
	virtual ~ProfileCallback() = default;

	// IDeckLinkProfileCallback interface
	HRESULT		ProfileChanging(IDeckLinkProfile *profileToBeActivated, bool streamsWillBeForcedToStop) override;
	HRESULT		ProfileActivated(IDeckLinkProfile *activatedProfile) override;

	// IUnknown interface
	HRESULT		QueryInterface(REFIID iid, LPVOID *ppv) override;
	ULONG		AddRef() override;
	ULONG		Release() override;

	void		onProfileChanging(const ProfileChangingCallback& callback) { m_profileChangingCallback = callback; }

private:
	QObject*					m_owner;
	std::atomic<ULONG>			m_refCount;
	ProfileChangingCallback		m_profileChangingCallback;
};

class ProfileActivatedEvent : public QEvent
{
public:
	ProfileActivatedEvent(IDeckLinkProfile* deckLinkProfile) :
		QEvent(kProfileActivatedEvent), m_deckLinkProfile(deckLinkProfile) {}
	virtual ~ProfileActivatedEvent() = default;

	com_ptr<IDeckLinkProfile> deckLinkProfile() const { return m_deckLinkProfile; }

private:
	com_ptr<IDeckLinkProfile> m_deckLinkProfile;
};
