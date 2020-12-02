#include <QCoreApplication>
#include "ProfileCallback.h"

ProfileCallback::ProfileCallback(QObject* owner) : 
	m_owner(owner), 
	m_refCount(1)
{
}

/// IUnknown methods

HRESULT ProfileCallback::QueryInterface(REFIID iid, LPVOID *ppv)
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

	return result;
}

ULONG ProfileCallback::AddRef(void)
{
	return ++m_refCount;
}

ULONG ProfileCallback::Release(void)
{
	ULONG newRefValue = --m_refCount;
	if (newRefValue == 0)
		delete this;

	return newRefValue;
}

/// IDeckLinkProfileCallback methods

HRESULT ProfileCallback::ProfileChanging(IDeckLinkProfile* /* profileToBeActivated */, bool streamsWillBeForcedToStop)
{
	// When streamsWillBeForcedToStop is true, the profile to be activated is incompatible with the current
	// profile and capture will be stopped by the DeckLink driver. It is better to notify the
	// controller to gracefully stop capture, so that the UI is set to a known state.
	if ((streamsWillBeForcedToStop) && (m_profileChangingCallback != nullptr))
		m_profileChangingCallback();

	return S_OK;
}

HRESULT ProfileCallback::ProfileActivated(IDeckLinkProfile *activatedProfile)
{
	// New profile activated, inform owner to update popup menus
	QCoreApplication::postEvent(m_owner, new ProfileActivatedEvent(activatedProfile));

	return S_OK;
}
