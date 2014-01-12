/*
  Copyright (C) 2011 Birunthan Mohanathas (www.poiru.net)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "cover.h"

#include <stdio.h>
#include <stdlib.h>
#include <fileref.h>
#include <tfile.h>
#include <asffile.h>
#include <vorbisfile.h>
#include <mpegfile.h>
#include <flacfile.h>
#include <oggflacfile.h>
#include <mpcfile.h>
#include <wavpackfile.h>
#include <speexfile.h>
#include <trueaudiofile.h>
#include <mp4file.h>
#include <tag.h>
#include <string.h>
#include <id3v2framefactory.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <asfpicture.h>

#include <apefile.h>
#include <apetag.h>
#include <asffile.h>
#include <attachedpictureframe.h>
#include <commentsframe.h>
#include <flacfile.h>
#include <mpcfile.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <taglib.h>
#include <textidentificationframe.h>
#include <tstring.h>
#include <vorbisfile.h>
#include <wavpackfile.h>

bool WriteCover(const TagLib::ByteVector& data, char *thumbnail)
{
	bool written = 0;

	FILE *l_jpg_file = fopen(thumbnail,"wb");
	if (l_jpg_file) {
		written = (fwrite(data.data(), 1, data.size(), l_jpg_file) == data.size());
		fclose(l_jpg_file);
	}

	return written;
}

bool ExtractID3(TagLib::ID3v2::Tag* tag, char *thumbnail)
{
	if (tag) {
		const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
		if (!frameList.isEmpty())
		{
			// Grab the first image
			TagLib::ID3v2::AttachedPictureFrame* frame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
			return WriteCover(frame->picture(), thumbnail);
		}
	}

	return false;
}

bool ExtractAPE(TagLib::APE::Tag* tag, char *thumbnail)
{
	if (tag) {
		const TagLib::APE::ItemListMap& listMap = tag->itemListMap();
		if (listMap.contains("COVER ART (FRONT)"))
		{
			const TagLib::ByteVector nullStringTerminator(1, 0);

			TagLib::ByteVector item = listMap["COVER ART (FRONT)"].value();
			int pos = item.find(nullStringTerminator);	// Skip the filename

			if (++pos > 0)
			{
				const TagLib::ByteVector& pic = item.mid(pos);
				return WriteCover(pic, thumbnail);
			}
		}
	}

	return false;
}

bool ExtractMP4(TagLib::MP4::File* file, char *thumbnail)
{
	if (file) {
		TagLib::MP4::Tag* tag = file->tag();
		if (tag && tag->itemListMap().contains("covr"))
		{
			TagLib::MP4::CoverArtList coverList = tag->itemListMap()["covr"].toCoverArtList();
			if (coverList[0].data().size() > 0)
			{
				return WriteCover(coverList[0].data(), thumbnail);
			}
		}
	}

	return false;
}

bool ExtractFLAC(TagLib::FLAC::File* file, char *thumbnail)
{
	if (file) {
		const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
		if (!picList.isEmpty())
		{
			// Let's grab the first image
			TagLib::FLAC::Picture* pic = picList[0];
			return WriteCover(pic->data(), thumbnail);
		}
	}

	return false;
}

bool ExtractASF(TagLib::ASF::File* file, char *thumbnail)
{
	if (file && file->tag()) {
		const TagLib::ASF::AttributeListMap& attrListMap = file->tag()->attributeListMap();
		if (attrListMap.contains("WM/Picture"))
		{
			const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];

			if (!attrList.isEmpty())
			{
				// Let's grab the first cover. TODO: Check/loop for correct type
				TagLib::ASF::Picture wmpic = attrList[0].toPicture();
				if (wmpic.isValid())
				{
					return WriteCover(wmpic.picture(), thumbnail);
				}
			}
		}
	}

	return false;
}
