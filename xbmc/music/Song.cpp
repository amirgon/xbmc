/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Song.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"

using namespace MUSIC_INFO;

CSong::CSong(CFileItem& item)
{
  CMusicInfoTag& tag = *item.GetMusicInfoTag();
  SYSTEMTIME stTime;
  tag.GetReleaseDate(stTime);
  strTitle = tag.GetTitle();
  genre = tag.GetGenre();
  std::vector<std::string> artist = tag.GetArtist();
  std::vector<std::string> musicBrainArtistHints = tag.GetMusicBrainzArtistHints();
  strArtistDesc = tag.GetArtistString();
  if (!tag.GetMusicBrainzArtistID().empty())
  { // have musicbrainz artist info, so use it
    for (size_t i = 0; i < tag.GetMusicBrainzArtistID().size(); i++)
    {
      std::string artistId = tag.GetMusicBrainzArtistID()[i];
      std::string artistName;
      /*
       We try and get the corresponding artist name from the hints list.
       If the hints list is missing or the wrong length, it will try the artist list.
       We match on the same index, and if that fails just use the first name we have.
       */
      if (i < musicBrainArtistHints.size())
        artistName = musicBrainArtistHints[i];
      else if (!artist.empty())
        artistName = (i < artist.size()) ? artist[i] : artist[0];
      if (artistName.empty())
        artistName = artistId;
      CArtistCredit artistCredit(artistName, artistId);
      artistCredits.push_back(artistCredit);
    }
  }
  else
  { // no musicbrainz info, so fill in directly
    for (std::vector<std::string>::const_iterator it = tag.GetArtist().begin(); it != tag.GetArtist().end(); ++it)
    {
      CArtistCredit artistCredit(*it);
      artistCredits.push_back(artistCredit);
    }
  }
  strAlbum = tag.GetAlbum();
  m_albumArtist = tag.GetAlbumArtist();
  strMusicBrainzTrackID = tag.GetMusicBrainzTrackID();
  m_musicRoles = tag.GetContributors();
  strComment = tag.GetComment();
  strCueSheet = tag.GetCueSheet();
  strMood = tag.GetMood();
  rating = tag.GetRating();
  userrating = tag.GetUserrating();
  votes = tag.GetVotes();
  iYear = stTime.wYear;
  iTrack = tag.GetTrackAndDiscNumber();
  iDuration = tag.GetDuration();
  bCompilation = tag.GetCompilation();
  embeddedArt = tag.GetCoverArtInfo();
  strFileName = tag.GetURL().empty() ? item.GetPath() : tag.GetURL();
  dateAdded = tag.GetDateAdded();
  strThumb = item.GetUserMusicThumb(true);
  iStartOffset = item.m_lStartOffset;
  iEndOffset = item.m_lEndOffset;
  idSong = -1;
  iTimesPlayed = 0;
  idAlbum = -1;
}

CSong::CSong()
{
  Clear();
}

void CSong::MergeScrapedSong(const CSong& source, bool override)
{
  if ((override && !source.strTitle.empty()) || strTitle.empty())
    strTitle = source.strTitle;
  if ((override && source.iTrack != 0) || iTrack == 0)
    iTrack = source.iTrack;
  // artist = source.artist; // artist is read-only from the database
  if (override)
    artistCredits = source.artistCredits;
  else if (source.artistCredits.size() > artistCredits.size())
    artistCredits.insert(artistCredits.end(), source.artistCredits.begin()+artistCredits.size(), source.artistCredits.end());
}

void CSong::Serialize(CVariant& value) const
{
  value["filename"] = strFileName;
  value["title"] = strTitle;
  value["artist"] = GetArtist();
  value["album"] = strAlbum;
  value["albumartist"] = GetAlbumArtist();
  value["genre"] = genre;
  value["duration"] = iDuration;
  value["track"] = iTrack;
  value["year"] = iYear;
  value["musicbrainztrackid"] = strMusicBrainzTrackID;
  value["comment"] = strComment;
  value["mood"] = strMood;
  value["rating"] = rating;
  value["userrating"] = userrating;
  value["votes"] = votes;
  value["timesplayed"] = iTimesPlayed;
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDateTime() : "";
  value["dateadded"] = dateAdded.IsValid() ? dateAdded.GetAsDBDateTime() : "";
  value["albumid"] = idAlbum;
}

void CSong::Clear()
{
  strFileName.clear();
  strTitle.clear();
  strAlbum.clear();
  m_albumArtist.clear();
  genre.clear();
  strThumb.clear();
  strMusicBrainzTrackID.clear();
  m_musicRoles.clear();
  strComment.clear();
  strMood.clear();
  rating = 0;
  userrating = 0;
  votes = 0;
  iTrack = 0;
  iDuration = 0;
  iYear = 0;
  iStartOffset = 0;
  iEndOffset = 0;
  idSong = -1;
  iTimesPlayed = 0;
  lastPlayed.Reset();
  dateAdded.Reset();
  idAlbum = -1;
  bCompilation = false;
  embeddedArt.clear();
}
const std::vector<std::string> CSong::GetArtist() const
{
  //Get artist names as vector from artist credits
  std::vector<std::string> songartists;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
  {
    songartists.push_back(artistCredit->GetArtist());
  }
  //When artist credits have not been populated attempt to build an artist vector from the descrpition string
  //This is a tempory fix, in the longer term other areas should query the song_artist table and populate
  //artist credits. Note that splitting the string may not give the same artists as held in the song_artist table
  if (songartists.empty() && !strArtistDesc.empty())
    songartists = StringUtils::Split(strArtistDesc, g_advancedSettings.m_musicItemSeparator);
  return songartists;
}

const std::vector<std::string> CSong::GetMusicBrainzArtistID() const
{
  //Get artist MusicBrainz IDs as vector from artist credits
  std::vector<std::string> muisicBrainzID;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
  {
    muisicBrainzID.push_back(artistCredit->GetMusicBrainzArtistID());
  }
  return muisicBrainzID;
}

const std::string CSong::GetArtistString() const
{
  //Artist description may be different from the artists in artistcredits (see ARTISTS tag processing)
  //but is takes precidence as a string because artistcredits is not always filled during processing
  if (!strArtistDesc.empty())
    return strArtistDesc;
  std::vector<std::string> artistvector;
  for (VECARTISTCREDITS::const_iterator i = artistCredits.begin(); i != artistCredits.end(); ++i)
    artistvector.push_back(i->GetArtist());
  std::string artistString;
  if (!artistvector.empty())
    artistString = StringUtils::Join(artistvector, g_advancedSettings.m_musicItemSeparator);
  return artistString;
}

const std::vector<int> CSong::GetArtistIDArray() const
{
  // Get song artist IDs for json rpc
  std::vector<int> artistids;
  for (VECARTISTCREDITS::const_iterator artistCredit = artistCredits.begin(); artistCredit != artistCredits.end(); ++artistCredit)
    artistids.push_back(artistCredit->GetArtistId());
  return artistids;
}

void CSong::AppendArtistRole(const CMusicRole& musicRole)
{
  m_musicRoles.push_back(musicRole);
}

bool CSong::HasArt() const
{
  if (!strThumb.empty()) return true;
  if (!embeddedArt.empty()) return true;
  return false;
}

bool CSong::ArtMatches(const CSong &right) const
{
  return (right.strThumb == strThumb &&
          embeddedArt.matches(right.embeddedArt));
}
