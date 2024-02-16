/* -LICENSE-START-
** Copyright (c) 2019 Blackmagic Design
**  
** Permission is hereby granted, free of charge, to any person or organization 
** obtaining a copy of the software and accompanying documentation (the 
** "Software") to use, reproduce, display, distribute, sub-license, execute, 
** and transmit the Software, and to prepare derivative works of the Software, 
** and to permit third-parties to whom the Software is furnished to do so, in 
** accordance with:
** 
** (1) if the Software is obtained from Blackmagic Design, the End User License 
** Agreement for the Software Development Kit (“EULA”) available at 
** https://www.blackmagicdesign.com/EULA/DeckLinkSDK; or
** 
** (2) if the Software is obtained from any third party, such licensing terms 
** as notified by that third party,
** 
** and all subject to the following:
** 
** (3) the copyright notices in the Software and this entire statement, 
** including the above license grant, this restriction and the following 
** disclaimer, must be included in all copies of the Software, in whole or in 
** part, and all derivative works of the Software, unless such copies or 
** derivative works are solely in the form of machine-executable object code 
** generated by a source language processor.
** 
** (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
** DEALINGS IN THE SOFTWARE.
** 
** A copy of the Software is available free of charge at 
** https://www.blackmagicdesign.com/desktopvideo_sdk under the EULA.
** 
** -LICENSE-END-
*/

#include "AncillaryDataTable.h"

AncillaryDataTable::AncillaryDataTable(QObject* parent)
	: QAbstractTableModel(parent)
{
	m_ancillaryDataValues << "" << "" << "" << "" << "" << "" << "" << "" << "" << "" << "" << "";
	m_metadataValues << "" << "" << "" << "" << "" << "" << "" << "" << "" << "" << "" << "" << "" << "";
}

void AncillaryDataTable::UpdateFrameData(AncillaryDataStruct* newAncData, MetadataStruct* newMetadata)
{
	// VITC timecodes
	m_ancillaryDataValues.replace(0, newAncData->vitcF1Timecode);
	m_ancillaryDataValues.replace(1, newAncData->vitcF1UserBits);
	m_ancillaryDataValues.replace(2, newAncData->vitcF2Timecode);
	m_ancillaryDataValues.replace(3, newAncData->vitcF2UserBits);

	// RP188 timecodes and user bits
	m_ancillaryDataValues.replace(4, newAncData->rp188vitc1Timecode);
	m_ancillaryDataValues.replace(5, newAncData->rp188vitc1UserBits);
	m_ancillaryDataValues.replace(6, newAncData->rp188vitc2Timecode);
	m_ancillaryDataValues.replace(7, newAncData->rp188vitc2UserBits);
	m_ancillaryDataValues.replace(8, newAncData->rp188ltcTimecode);
	m_ancillaryDataValues.replace(9, newAncData->rp188ltcUserBits);
	m_ancillaryDataValues.replace(10, newAncData->rp188hfrtcTimecode);
	m_ancillaryDataValues.replace(11, newAncData->rp188hfrtcUserBits);

	// Static Metadata
	m_metadataValues.replace(0, newMetadata->electroOpticalTransferFunction);
	m_metadataValues.replace(1, newMetadata->displayPrimariesRedX);
	m_metadataValues.replace(2, newMetadata->displayPrimariesRedY);
	m_metadataValues.replace(3, newMetadata->displayPrimariesGreenX);
	m_metadataValues.replace(4, newMetadata->displayPrimariesGreenY);
	m_metadataValues.replace(5, newMetadata->displayPrimariesBlueX);
	m_metadataValues.replace(6, newMetadata->displayPrimariesBlueY);
	m_metadataValues.replace(7, newMetadata->whitePointX);
	m_metadataValues.replace(8, newMetadata->whitePointY);
	m_metadataValues.replace(9, newMetadata->maxDisplayMasteringLuminance);
	m_metadataValues.replace(10, newMetadata->minDisplayMasteringLuminance);
	m_metadataValues.replace(11, newMetadata->maximumContentLightLevel);
	m_metadataValues.replace(12, newMetadata->maximumFrameAverageLightLevel);
	m_metadataValues.replace(13, newMetadata->colorspace);

	emit dataChanged(index(0, static_cast<int>(AncillaryHeader::Values)), index(rowCount()-1, static_cast<int>(AncillaryHeader::Values)));
}

QVariant AncillaryDataTable::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if ((index.row() >= (kAncillaryDataTypes.size() + kMetadataTypes.size())) || (index.column() >= kAncillaryTableColumnCount))
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		if (index.column() == static_cast<int>(AncillaryHeader::Types))
		{
			if (index.row() < kAncillaryDataTypes.size())
				return kAncillaryDataTypes.at(index.row());
			else
				return kMetadataTypes.at(index.row() - kAncillaryDataTypes.size());
		}
		else if (index.column() == static_cast<int>(AncillaryHeader::Values))
		{
			if (index.row() < kAncillaryDataTypes.size())
				return m_ancillaryDataValues.at(index.row());
			else
				return m_metadataValues.at(index.row() - kAncillaryDataTypes.size());
		}
	}

	return QVariant();
}

QVariant AncillaryDataTable::headerData(int section, Qt::Orientation orientation, int role) const
{
	if ((role == Qt::DisplayRole) && (orientation == Qt::Horizontal))
	{
		if (section == static_cast<int>(AncillaryHeader::Types))
			return "Type";
		else if (section == static_cast<int>(AncillaryHeader::Values))
			return "Value";
	}

	return QVariant();
}

