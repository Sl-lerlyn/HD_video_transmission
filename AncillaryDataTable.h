#pragma once

#include <QAbstractTableModel>
#include <QMutex>
#include <QStringList>

enum class AncillaryHeader : int { Types, Values };
const int kAncillaryTableColumnCount = 2;

const QStringList kAncillaryDataTypes = {
	"VITC Timecode field 1",
	"VITC User bits field 1",
	"VITC Timecode field 2",
	"VITC User bits field 2",
	"RP188 VITC1 Timecode",
	"RP188 VITC1 User bits",
	"RP188 VITC2 Timecode",
	"RP188 VITC2 User bits",
	"RP188 LTC Timecode",
	"RP188 LTC User bits",
	"RP188 HFRTC Timecode",
	"RP188 HFRTC User bits",
};

const QStringList kMetadataTypes = {
	"Static HDR Electro-optical Transfer Function",
	"Static HDR Display Primaries Red X",
	"Static HDR Display Primaries Red Y",
	"Static HDR Display Primaries Green X",
	"Static HDR Display Primaries Green Y",
	"Static HDR Display Primaries Blue X",
	"Static HDR Display Primaries Blue Y",
	"Static HDR White Point X",
	"Static HDR White Point Y",
	"Static HDR Max Display Mastering Luminance",
	"Static HDR Min Display Mastering Luminance",
	"Static HDR Max Content Light Level",
	"Static HDR Max Frame Average Light Level",
	"Static Colorspace",
};

typedef struct {
	// VITC timecodes and user bits for field 1 & 2
	QString vitcF1Timecode;
	QString vitcF1UserBits;
	QString vitcF2Timecode;
	QString vitcF2UserBits;

	// RP188 timecodes and user bits (VITC1, VITC2, LTC and HFRTC)
	QString rp188vitc1Timecode;
	QString rp188vitc1UserBits;
	QString rp188vitc2Timecode;
	QString rp188vitc2UserBits;
	QString rp188ltcTimecode;
	QString rp188ltcUserBits;
	QString rp188hfrtcTimecode;
	QString rp188hfrtcUserBits;
} AncillaryDataStruct;

typedef struct {
	QString electroOpticalTransferFunction;
	QString displayPrimariesRedX;
	QString displayPrimariesRedY;
	QString displayPrimariesGreenX;
	QString displayPrimariesGreenY;
	QString displayPrimariesBlueX;
	QString displayPrimariesBlueY;
	QString whitePointX;
	QString whitePointY;
	QString maxDisplayMasteringLuminance;
	QString minDisplayMasteringLuminance;
	QString maximumContentLightLevel;
	QString maximumFrameAverageLightLevel;
	QString colorspace;
} MetadataStruct;

class AncillaryDataTable : public QAbstractTableModel
{
	Q_OBJECT

public:
	AncillaryDataTable(QObject* parent = nullptr);
	virtual ~AncillaryDataTable() {}

	void UpdateFrameData(AncillaryDataStruct* newAncData, MetadataStruct* newMetadata);

	// QAbstractTableModel methods
	int			rowCount(const QModelIndex& parent = QModelIndex()) const override { Q_UNUSED(parent); return kAncillaryDataTypes.size() + kMetadataTypes.size(); }
	int			columnCount(const QModelIndex& parent = QModelIndex()) const override { Q_UNUSED(parent); return kAncillaryTableColumnCount; }
	QVariant	data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant	headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
	QMutex			m_updateMutex;
	QStringList		m_ancillaryDataValues;
	QStringList		m_metadataValues;
};

