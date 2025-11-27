/*
  This is modified version of original code copied from (accessed 27 nov 2025):
  https://github.com/libmtp/libmtp/blob/master/examples/pathutils.c
*/
/** 
 * \file pathutils.c
 *
 * Copyright (C) 2005-2008 Linus Walleij <triad@df.lth.se>
 * Copyright (C) 2006 Chris A. Debenham <chris@adebenham.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include "common.h"
#include <libmtp.h>

/* Find the file type based on extension */
LIBMTP_filetype_t
find_filetype (const char * filename)
{
  char *ptype;
  LIBMTP_filetype_t filetype;

#ifdef __WIN32__
  ptype = strrchr(filename, '.');
#else
  ptype = rindex(filename,'.');
#endif
  // This accounts for the case with a filename without any "." (period).
  if (!ptype) {
    ptype = (char*)"";
  } else {
    ++ptype;
  }

  /* This need to be kept constantly updated as new file types arrive. */
  if (!strcasecmp (ptype, "wav")) {
    filetype = LIBMTP_FILETYPE_WAV;
  } else if (!strcasecmp (ptype, "mp3")) {
    filetype = LIBMTP_FILETYPE_MP3;
  } else if (!strcasecmp (ptype, "wma")) {
    filetype = LIBMTP_FILETYPE_WMA;
  } else if (!strcasecmp (ptype, "ogg")) {
    filetype = LIBMTP_FILETYPE_OGG;
  } else if (!strcasecmp (ptype, "mp4")) {
    filetype = LIBMTP_FILETYPE_MP4;
  } else if (!strcasecmp (ptype, "wmv")) {
    filetype = LIBMTP_FILETYPE_WMV;
  } else if (!strcasecmp (ptype, "avi")) {
    filetype = LIBMTP_FILETYPE_AVI;
  } else if (!strcasecmp (ptype, "mpeg") || !strcasecmp (ptype, "mpg")) {
    filetype = LIBMTP_FILETYPE_MPEG;
  } else if (!strcasecmp (ptype, "asf")) {
    filetype = LIBMTP_FILETYPE_ASF;
  } else if (!strcasecmp (ptype, "qt") || !strcasecmp (ptype, "mov")) {
    filetype = LIBMTP_FILETYPE_QT;
  } else if (!strcasecmp (ptype, "wma")) {
    filetype = LIBMTP_FILETYPE_WMA;
  } else if (!strcasecmp (ptype, "jpg") || !strcasecmp (ptype, "jpeg")) {
    filetype = LIBMTP_FILETYPE_JPEG;
  } else if (!strcasecmp (ptype, "jfif")) {
    filetype = LIBMTP_FILETYPE_JFIF;
  } else if (!strcasecmp (ptype, "tif") || !strcasecmp (ptype, "tiff")) {
    filetype = LIBMTP_FILETYPE_TIFF;
  } else if (!strcasecmp (ptype, "bmp")) {
    filetype = LIBMTP_FILETYPE_BMP;
  } else if (!strcasecmp (ptype, "gif")) {
    filetype = LIBMTP_FILETYPE_GIF;
  } else if (!strcasecmp (ptype, "pic") || !strcasecmp (ptype, "pict")) {
    filetype = LIBMTP_FILETYPE_PICT;
  } else if (!strcasecmp (ptype, "png")) {
    filetype = LIBMTP_FILETYPE_PNG;
  } else if (!strcasecmp (ptype, "wmf")) {
    filetype = LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT;
  } else if (!strcasecmp (ptype, "ics")) {
    filetype = LIBMTP_FILETYPE_VCALENDAR2;
  } else if (!strcasecmp (ptype, "exe") || !strcasecmp (ptype, "com") ||
	     !strcasecmp (ptype, "bat") || !strcasecmp (ptype, "dll") ||
	     !strcasecmp (ptype, "sys")) {
    filetype = LIBMTP_FILETYPE_WINEXEC;
  } else if (!strcasecmp (ptype, "aac")) {
    filetype = LIBMTP_FILETYPE_AAC;
  } else if (!strcasecmp (ptype, "mp2")) {
    filetype = LIBMTP_FILETYPE_MP2;
  } else if (!strcasecmp (ptype, "flac")) {
    filetype = LIBMTP_FILETYPE_FLAC;
  } else if (!strcasecmp (ptype, "m4a")) {
    filetype = LIBMTP_FILETYPE_M4A;
  } else if (!strcasecmp (ptype, "doc")) {
    filetype = LIBMTP_FILETYPE_DOC;
  } else if (!strcasecmp (ptype, "xml")) {
    filetype = LIBMTP_FILETYPE_XML;
  } else if (!strcasecmp (ptype, "xls")) {
    filetype = LIBMTP_FILETYPE_XLS;
  } else if (!strcasecmp (ptype, "ppt")) {
    filetype = LIBMTP_FILETYPE_PPT;
  } else if (!strcasecmp (ptype, "mht")) {
    filetype = LIBMTP_FILETYPE_MHT;
  } else if (!strcasecmp (ptype, "jp2")) {
    filetype = LIBMTP_FILETYPE_JP2;
  } else if (!strcasecmp (ptype, "jpx")) {
    filetype = LIBMTP_FILETYPE_JPX;
  } else if (!strcasecmp (ptype, "bin")) {
    filetype = LIBMTP_FILETYPE_FIRMWARE;
  } else if (!strcasecmp (ptype, "vcf")) {
    filetype = LIBMTP_FILETYPE_VCARD3;
  } else {
    /* Tagging as unknown file type */
    filetype = LIBMTP_FILETYPE_UNKNOWN;
  }
  // printf("type: %s, %d\n", ptype, filetype);
  return filetype;
}
