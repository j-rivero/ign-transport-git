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

#include <cstdio>
#include <iostream>
#include <string>
#include <ignition/transport.hh>
#include "msg/stringmsg.pb.h"

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const std::string &_topic, const example::mymsgs::StringMsg &_msg)
{
  std::cout << "Topic:" << _topic << "\n"
            << "Msg:  " << _msg.data() << "\n\n";
}

//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ignition::transport::Node node;

  // Subscribe to a topic by registering a callback.
  node.Subscribe("/foo", cb);

  // Zzzzzz.
  std::cout << "Press <ENTER> to exit" << std::endl;
  getchar();

  return 0;
}
