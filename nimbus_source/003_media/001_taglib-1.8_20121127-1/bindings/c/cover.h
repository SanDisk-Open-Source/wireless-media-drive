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

#ifndef _COVER_H_
#define _COVER_H_

#include <fileref.h>
#include <id3v2tag.h>
#include <apetag.h>
#include <mp4file.h>
#include <flacfile.h>
#include <asffile.h>

bool WriteCover(const TagLib::ByteVector& data, char *thumbnail);
bool ExtractID3(TagLib::ID3v2::Tag* tag, char *thumbnail);
bool ExtractAPE(TagLib::APE::Tag* tag, char *thumbnail);
bool ExtractMP4(TagLib::MP4::File* file, char *thumbnail);
bool ExtractFLAC(TagLib::FLAC::File* file, char *thumbnail);
bool ExtractASF(TagLib::ASF::File* file, char *thumbnail);

#endif
