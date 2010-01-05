/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2009 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "UTMetadataRequestExtensionMessage.h"
#include "BDE.h"
#include "bencode.h"
#include "util.h"
#include "a2functional.h"
#include "bittorrent_helper.h"
#include "DlAbortEx.h"
#include "StringFormat.h"
#include "BtMessageFactory.h"
#include "BtMessageDispatcher.h"
#include "Peer.h"
#include "UTMetadataRejectExtensionMessage.h"
#include "UTMetadataDataExtensionMessage.h"
#include "BtConstants.h"
#include "DownloadContext.h"
#include "BtMessage.h"
#include "PieceStorage.h"

namespace aria2 {

UTMetadataRequestExtensionMessage::UTMetadataRequestExtensionMessage
(uint8_t extensionMessageID):UTMetadataExtensionMessage(extensionMessageID) {}

std::string UTMetadataRequestExtensionMessage::getPayload()
{
  BDE dict = BDE::dict();
  dict["msg_type"] = 0;
  dict["piece"] = _index;
  return bencode::encode(dict);
}

std::string UTMetadataRequestExtensionMessage::toString() const
{
  return strconcat("ut_metadata request piece=", util::uitos(_index));
}

void UTMetadataRequestExtensionMessage::doReceivedAction()
{
  const BDE& attrs = _dctx->getAttribute(bittorrent::BITTORRENT);
  uint8_t id = _peer->getExtensionMessageID("ut_metadata");
  if(!attrs.containsKey(bittorrent::METADATA)) {
    SharedHandle<UTMetadataRejectExtensionMessage> m
      (new UTMetadataRejectExtensionMessage(id));
    m->setIndex(_index);
    SharedHandle<BtMessage> msg = _messageFactory->createBtExtendedMessage(m);
    _dispatcher->addMessageToQueue(msg);
  }else if(_index*METADATA_PIECE_SIZE <
           (size_t)attrs[bittorrent::METADATA_SIZE].i()){
    SharedHandle<UTMetadataDataExtensionMessage> m
      (new UTMetadataDataExtensionMessage(id));
    m->setIndex(_index);
    m->setTotalSize(attrs[bittorrent::METADATA_SIZE].i());
    const BDE& metadata = attrs[bittorrent::METADATA];
    std::string::const_iterator begin =
      metadata.s().begin()+_index*METADATA_PIECE_SIZE;
    std::string::const_iterator end =
      (_index+1)*METADATA_PIECE_SIZE <= metadata.s().size()?
      metadata.s().begin()+(_index+1)*METADATA_PIECE_SIZE:metadata.s().end();
    m->setData(std::string(begin, end));
    SharedHandle<BtMessage> msg = _messageFactory->createBtExtendedMessage(m);
    _dispatcher->addMessageToQueue(msg);
  } else {
    throw DL_ABORT_EX
      (StringFormat("Metadata piece index is too big. piece=%d", _index).str());
  }
}

} // namespace aria2
