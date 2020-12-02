#include "CapturePreview.h"
#include <QApplication>
#include <iostream>
#include "ConnectToAgora.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	qRegisterMetaType<com_ptr<IDeckLinkVideoFrame>>("com_ptr<IDeckLinkVideoFrame>");

    if (connectAgora() > 0);  //printf("success connect to agora")
    CapturePreview w;
    w.show();
    w.setup();
    int temp = a.exec();

    if (disconnectAgora() > 0);  //printf("success disconect to agora")
    return temp;
}
