/*
 * Copyright (C) 2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <cstring>
#include <string>
#include "ignition/transport/Packet.hh"

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
Header::Header(const uint16_t _version,
               const std::string &_pUuid,
               const uint8_t _type,
               const uint16_t _flags)
{
  this->SetVersion(_version);
  this->SetPUuid(_pUuid);
  this->SetType(_type);
  this->SetFlags(_flags);
}

//////////////////////////////////////////////////
uint16_t Header::GetVersion() const
{
  return this->version;
}

//////////////////////////////////////////////////
std::string Header::GetPUuid() const
{
  return this->pUuid;
}

//////////////////////////////////////////////////
uint8_t Header::GetType() const
{
  return this->type;
}

//////////////////////////////////////////////////
uint16_t Header::GetFlags() const
{
  return this->flags;
}

//////////////////////////////////////////////////
void Header::SetVersion(const uint16_t _version)
{
  this->version = _version;
}

//////////////////////////////////////////////////
void Header::SetPUuid(const std::string &_pUuid)
{
  this->pUuid = _pUuid;
}

//////////////////////////////////////////////////
void Header::SetType(const uint8_t _type)
{
  this->type = _type;
}

//////////////////////////////////////////////////
void Header::SetFlags(const uint16_t _flags)
{
  this->flags = _flags;
}

//////////////////////////////////////////////////
int Header::GetHeaderLength()
{
  return sizeof(this->version) +
         sizeof(size_t) + this->pUuid.size() +
         sizeof(this->type) + sizeof(this->flags);
}

//////////////////////////////////////////////////
size_t Header::Pack(char *_buffer)
{
  // Uninitialized.
  if ((this->version == 0) || (this->pUuid == "") ||
      (this->type  == Uninitialized))
  {
    std::cerr << "Header::Pack() error: You're trying to pack an incomplete "
              << "header:" << std::endl << *this;
    return 0;
  }

  // Pack the discovery protocol version.
  memcpy(_buffer, &this->version, sizeof(this->version));
  _buffer += sizeof(this->version);

  // Pack the process UUID length.
  size_t pUuidLength = this->pUuid.size();
  memcpy(_buffer, &pUuidLength, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);

  // Pack the process UUID.
  memcpy(_buffer, this->pUuid.data(), pUuidLength);
  _buffer += pUuidLength;

  // Pack the message type (ADVERTISE, SUBSCRIPTION, ...).
  memcpy(_buffer, &this->type, sizeof(this->type));
  _buffer += sizeof(this->type);

  // Pack the flags.
  memcpy(_buffer, &this->flags, sizeof(this->flags));

  return this->GetHeaderLength();
}

//////////////////////////////////////////////////
size_t Header::Unpack(const char *_buffer)
{
  // Unpack the version.
  memcpy(&this->version, _buffer, sizeof(this->version));
  _buffer += sizeof(this->version);

  // Unpack the process UUID length.
  size_t pUuidLength;
  memcpy(&pUuidLength, _buffer, sizeof(pUuidLength));
  _buffer += sizeof(pUuidLength);

  // Unpack the process UUID.
  this->pUuid = std::string(_buffer, _buffer + pUuidLength);
  _buffer += pUuidLength;

  // Unpack the message type.
  memcpy(&this->type, _buffer, sizeof(this->type));
  _buffer += sizeof(this->type);

  // Unpack the flags.
  memcpy(&this->flags, _buffer, sizeof(this->flags));
  _buffer += sizeof(this->flags);

  return this->GetHeaderLength();
}

//////////////////////////////////////////////////
SubscriptionMsg::SubscriptionMsg(const Header &_header,
                                 const std::string &_topic)
{
  this->SetHeader(_header);
  this->SetTopic(_topic);
}

//////////////////////////////////////////////////
Header SubscriptionMsg::GetHeader() const
{
  return this->header;
}

//////////////////////////////////////////////////
std::string SubscriptionMsg::GetTopic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
void SubscriptionMsg::SetHeader(const Header &_header)
{
  this->header = _header;
}

//////////////////////////////////////////////////
void SubscriptionMsg::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
size_t SubscriptionMsg::GetMsgLength()
{
  return this->header.GetHeaderLength() +
         sizeof(size_t) + this->topic.size();
}

//////////////////////////////////////////////////
size_t SubscriptionMsg::Pack(char *_buffer)
{
  // Pack the header.
  size_t headerLen = this->GetHeader().Pack(_buffer);
  if (headerLen == 0)
    return 0;

  if (this->topic == "")
  {
    std::cerr << "SubscriptionMsg::Pack() error: You're trying to pack a "
              << "message with an empty topic" << std::endl;
    return 0;
  }

  _buffer += headerLen;

  // Pack the topic length.
  size_t topicLength = this->topic.size();
  memcpy(_buffer, &topicLength, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Pack the topic.
  memcpy(_buffer, this->topic.data(), topicLength);

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t SubscriptionMsg::UnpackBody(char *_buffer)
{
  // Unpack the topic length.
  size_t topicLength;
  memcpy(&topicLength, _buffer, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Unpack the topic.
  this->topic = std::string(_buffer, _buffer + topicLength);
  _buffer += topicLength;

  return sizeof(topicLength) + topicLength;
}

//////////////////////////////////////////////////
AdvertiseBase::AdvertiseBase(const Header &_header,
                             const std::string &_topic,
                             const std::string &_addr,
                             const std::string &_ctrl,
                             const std::string &_nUuid,
                             const Scope &_scope)
{
  this->SetHeader(_header);
  this->SetTopic(_topic);
  this->SetAddress(_addr);
  this->SetControlAddress(_ctrl);
  this->SetNodeUuid(_nUuid);
  this->SetScope(_scope);
}

//////////////////////////////////////////////////
Header AdvertiseBase::GetHeader() const
{
  return this->header;
}

//////////////////////////////////////////////////
std::string AdvertiseBase::GetTopic() const
{
  return this->topic;
}

//////////////////////////////////////////////////
std::string AdvertiseBase::GetAddress() const
{
  return this->addr;
}

//////////////////////////////////////////////////
std::string AdvertiseBase::GetControlAddress() const
{
  return this->ctrl;
}

//////////////////////////////////////////////////
std::string AdvertiseBase::GetNodeUuid() const
{
  return this->nUuid;
}

//////////////////////////////////////////////////
transport::Scope AdvertiseBase::GetScope() const
{
  return this->scope;
}

//////////////////////////////////////////////////
void AdvertiseBase::SetHeader(const Header &_header)
{
  this->header = _header;
}

//////////////////////////////////////////////////
void AdvertiseBase::SetTopic(const std::string &_topic)
{
  this->topic = _topic;
}

//////////////////////////////////////////////////
void AdvertiseBase::SetAddress(const std::string &_addr)
{
  this->addr = _addr;
}

//////////////////////////////////////////////////
void AdvertiseBase::SetControlAddress(const std::string &_ctrl)
{
  this->ctrl = _ctrl;
}

//////////////////////////////////////////////////
void AdvertiseBase::SetNodeUuid(const std::string &_nUuid)
{
  this->nUuid = _nUuid;
}

//////////////////////////////////////////////////
void AdvertiseBase::SetScope(const Scope &_scope)
{
  this->scope = _scope;
}

//////////////////////////////////////////////////
size_t AdvertiseBase::GetMsgLength()
{
  return this->header.GetHeaderLength() +
         sizeof(size_t) + this->topic.size() +
         sizeof(size_t) + this->addr.size() +
         sizeof(size_t) + this->ctrl.size() +
         sizeof(size_t) + this->nUuid.size() +
         sizeof(this->scope);
}

//////////////////////////////////////////////////
size_t AdvertiseBase::Pack(char *_buffer)
{
  // Pack the header.
  size_t headerLen = this->GetHeader().Pack(_buffer);
  if (headerLen == 0)
    return 0;

  if ((this->topic == "") || (this->addr == "") || (this->nUuid == ""))
  {
    std::cerr << "AdvertiseBase::Pack() error: You're trying to pack an "
              << "incomplete msg body:" << std::endl << *this;
    return 0;
  }

  _buffer += headerLen;

  // Pack the topic length.
  size_t topicLength = this->topic.size();
  memcpy(_buffer, &topicLength, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Pack the topic.
  memcpy(_buffer, this->topic.data(), topicLength);
  _buffer += topicLength;

  // Pack the zeromq address length.
  size_t addrLength = this->addr.size();
  memcpy(_buffer, &addrLength, sizeof(addrLength));
  _buffer += sizeof(addrLength);

  // Pack the zeromq address.
  memcpy(_buffer, this->addr.data(), addrLength);
  _buffer += addrLength;

  // Pack the zeromq control address length.
  size_t ctrlLength = this->ctrl.size();
  memcpy(_buffer, &ctrlLength, sizeof(ctrlLength));
  _buffer += sizeof(ctrlLength);

  // Pack the zeromq control address.
  memcpy(_buffer, this->ctrl.data(), ctrlLength);
  _buffer += ctrlLength;

  // Pack the node UUID length.
  size_t nUuidLength = this->nUuid.size();
  memcpy(_buffer, &nUuidLength, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);

  // Pack the node UUID.
  memcpy(_buffer, this->nUuid.data(), nUuidLength);
  _buffer += nUuidLength;

  // Pack the topic scope.
  memcpy(_buffer, &this->scope, sizeof(this->scope));

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseBase::UnpackBody(char *_buffer)
{
  // Unpack the topic length.
  size_t topicLength;
  memcpy(&topicLength, _buffer, sizeof(topicLength));
  _buffer += sizeof(topicLength);

  // Unpack the topic.
  this->topic = std::string(_buffer, _buffer + topicLength);
  _buffer += topicLength;

  // Unpack the zeromq address length.
  size_t addrLength;
  memcpy(&addrLength, _buffer, sizeof(addrLength));
  _buffer += sizeof(addrLength);

  // Unpack the zeromq address.
  this->addr = std::string(_buffer, _buffer + addrLength);
  _buffer += addrLength;

  // Unpack the zeromq control address length.
  size_t ctrlLength;
  memcpy(&ctrlLength, _buffer, sizeof(ctrlLength));
  _buffer += sizeof(ctrlLength);

  // Unpack the zeromq control address.
  this->ctrl = std::string(_buffer, _buffer + ctrlLength);
  _buffer += ctrlLength;

  // Unpack the node UUID length.
  size_t nUuidLength;
  memcpy(&nUuidLength, _buffer, sizeof(nUuidLength));
  _buffer += sizeof(nUuidLength);

  // Unpack the node UUID.
  this->nUuid = std::string(_buffer, _buffer + nUuidLength);
  _buffer += nUuidLength;

  // Unpack the topic scope.
  memcpy(&this->scope, _buffer, sizeof(this->scope));

  return sizeof(topicLength) + topicLength +
         sizeof(addrLength) + addrLength +
         sizeof(ctrlLength) + ctrlLength +
         sizeof(nUuidLength) + nUuidLength +
         sizeof(this->scope);
}

//////////////////////////////////////////////////
AdvertiseMsg::AdvertiseMsg(const Header &_header,
                           const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_nUuid,
                           const Scope &_scope,
                           const std::string &_msgTypeName)
  : AdvertiseBase(_header, _topic, _addr, _ctrl, _nUuid, _scope)
{
  this->SetMsgTypeName(_msgTypeName);
}

//////////////////////////////////////////////////
std::string AdvertiseMsg::GetMsgTypeName() const
{
  return this->msgTypeName;
}

//////////////////////////////////////////////////
void AdvertiseMsg::SetMsgTypeName(const std::string &_msgTypeName)
{
  this->msgTypeName = _msgTypeName;
}

//////////////////////////////////////////////////
size_t AdvertiseMsg::GetMsgLength()
{
  return AdvertiseBase::GetMsgLength() +
         sizeof(size_t) + this->msgTypeName.size();
}

//////////////////////////////////////////////////
size_t AdvertiseMsg::Pack(char *_buffer)
{
  // Pack the common part of any advertise message.
  size_t len = AdvertiseBase::Pack(_buffer);
  if (len == 0)
    return 0;

  if (this->msgTypeName == "")
  {
    std::cerr << "AdvertiseMsg::Pack() error: You're trying to pack a message "
              << "with an empty msgTypeName" << std::endl;
    return 0;
  }

  _buffer += len;

  // Pack the length of the probouf name contained in the message.
  size_t msgTypeNameLength = this->msgTypeName.size();
  memcpy(_buffer, &msgTypeNameLength, sizeof(msgTypeNameLength));
  _buffer += sizeof(msgTypeNameLength);

  // Pack the protobuf name contained in the message.
  memcpy(_buffer, this->msgTypeName.data(), msgTypeNameLength);

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseMsg::UnpackBody(char *_buffer)
{
  // Unpack the common part of any advertise message.
  _buffer += AdvertiseBase::UnpackBody(_buffer);

  // Unpack the msgTypeName length.
  size_t msgTypeNameLen;
  memcpy(&msgTypeNameLen, _buffer, sizeof(msgTypeNameLen));
  _buffer += sizeof(msgTypeNameLen);

  // Unpack the msgTypeName.
  this->msgTypeName = std::string(_buffer, _buffer + msgTypeNameLen);

  return this->GetMsgLength() - this->GetHeader().GetHeaderLength();
}

//////////////////////////////////////////////////
AdvertiseSrv::AdvertiseSrv(const Header &_header,
                           const std::string &_topic,
                           const std::string &_addr,
                           const std::string &_ctrl,
                           const std::string &_nUuid,
                           const Scope &_scope,
                           const std::string &_reqTypeName,
                           const std::string &_repTypeName)
  : AdvertiseBase(_header, _topic, _addr, _ctrl, _nUuid, _scope)
{
  this->SetReqTypeName(_reqTypeName);
  this->SetRepTypeName(_repTypeName);
}

//////////////////////////////////////////////////
std::string AdvertiseSrv::GetReqTypeName() const
{
  return this->reqTypeName;
}

//////////////////////////////////////////////////
std::string AdvertiseSrv::GetRepTypeName() const
{
  return this->repTypeName;
}

//////////////////////////////////////////////////
void AdvertiseSrv::SetReqTypeName(const std::string &_reqTypeName)
{
  this->reqTypeName = _reqTypeName;
}

//////////////////////////////////////////////////
void AdvertiseSrv::SetRepTypeName(const std::string &_repTypeName)
{
  this->repTypeName = _repTypeName;
}

//////////////////////////////////////////////////
size_t AdvertiseSrv::GetMsgLength()
{
  return AdvertiseBase::GetMsgLength() +
         sizeof(size_t) + this->reqTypeName.size() +
         sizeof(size_t) + this->repTypeName.size();
}

//////////////////////////////////////////////////
size_t AdvertiseSrv::Pack(char *_buffer)
{
  // Pack the common part of any advertise message.
  size_t len = AdvertiseBase::Pack(_buffer);
  if (len == 0)
    return 0;

  if ((this->reqTypeName == "") || (this->repTypeName == ""))
  {
    std::cerr << "AdvertiseSrv::Pack() error: You're trying to pack an "
              << "incomplete msg body:" << std::endl << *this << std::endl;
    return 0;
  }

  _buffer += len;

  // Pack the length of the protobuf name used as a service call request.
  size_t reqTypeNameLength = this->reqTypeName.size();
  memcpy(_buffer, &reqTypeNameLength, sizeof(reqTypeNameLength));
  _buffer += sizeof(reqTypeNameLength);

  // Pack the protobuf name used as a service call request.
  memcpy(_buffer, this->reqTypeName.data(), reqTypeNameLength);
  _buffer += reqTypeNameLength;

  // Pack the length of the protobuf name used as a service call response.
  size_t repTypeNameLength = this->repTypeName.size();
  memcpy(_buffer, &repTypeNameLength, sizeof(repTypeNameLength));
  _buffer += sizeof(repTypeNameLength);

  // Pack the protobuf name used as a service call response.
  memcpy(_buffer, this->repTypeName.data(), repTypeNameLength);

  return this->GetMsgLength();
}

//////////////////////////////////////////////////
size_t AdvertiseSrv::UnpackBody(char *_buffer)
{
  // Unpack the common part of any advertise message.
  _buffer += AdvertiseBase::UnpackBody(_buffer);

  // Unpack the reqTypeName length.
  size_t reqTypeNameLen;
  memcpy(&reqTypeNameLen, _buffer, sizeof(reqTypeNameLen));
  _buffer += sizeof(reqTypeNameLen);

  // Unpack the reqTypeName.
  this->reqTypeName = std::string(_buffer, _buffer + reqTypeNameLen);
  _buffer += reqTypeNameLen;

  // Unpack the repTypeName length.
  size_t repTypeNameLen;
  memcpy(&repTypeNameLen, _buffer, sizeof(repTypeNameLen));
  _buffer += sizeof(repTypeNameLen);

  // Unpack the repTypeName.
  this->repTypeName = std::string(_buffer, _buffer + repTypeNameLen);

  return this->GetMsgLength() - this->GetHeader().GetHeaderLength();
}
