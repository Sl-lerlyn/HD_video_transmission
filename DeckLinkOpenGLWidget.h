#pragma once

#include <atomic>
#include <mutex>
#include <QOpenGLWidget>
#include "com_ptr.h"
#include "DeckLinkAPI.h"

class DeckLinkOpenGLDelegate : public QObject, public IDeckLinkScreenPreviewCallback
{
	Q_OBJECT

public:
	DeckLinkOpenGLDelegate();
	virtual ~DeckLinkOpenGLDelegate() = default;
	
	// IUnknown
	HRESULT		QueryInterface(REFIID iid, LPVOID *ppv) override;
	ULONG		AddRef() override;
	ULONG		Release() override;

	// IDeckLinkScreenPreviewCallback
	HRESULT		DrawFrame(IDeckLinkVideoFrame* theFrame) override;

signals:
	void		frameArrived(com_ptr<IDeckLinkVideoFrame> frame);

private:
	std::atomic<ULONG>		m_refCount;
};

class DeckLinkOpenGLWidget : public QOpenGLWidget
{
	Q_OBJECT

public:
	DeckLinkOpenGLWidget(QWidget* parent = nullptr);
	virtual ~DeckLinkOpenGLWidget() = default;

	IDeckLinkScreenPreviewCallback* delegate(void) const { return m_delegate.get(); }

	void clear();

protected:
	// QOpenGLWidget
	void	initializeGL() override;
	void	paintGL() override;
	void	resizeGL(int width, int height) override;

private slots:
	void	setFrame(com_ptr<IDeckLinkVideoFrame> frame);

private:
	com_ptr<DeckLinkOpenGLDelegate>			m_delegate;
	com_ptr<IDeckLinkGLScreenPreviewHelper>	m_deckLinkScreenPreviewHelper;
	std::mutex								m_mutex;
};
