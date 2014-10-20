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

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <string>
#include <thread>
#if (_MSC_VER >= 1400)  // VS2005 for setenv
#include <sstream>
#endif
#include "gtest/gtest.h"
#include "ignition/transport/Node.hh"
#include "msg/int.pb.h"

// Implement non POSIX setenv call in Visual Studio
#if (_MSC_VER >= 1400)
#include <sstream>

int setenv(const char * name, const char * value, int /*rewrite*/)
{
  std::stringstream sstr;
  sstr << *name << '=' << value;
  return _putenv(sstr.str().c_str());
}
#endif

using namespace ignition;

std::string topic = "/foo";
int data = 5;
bool cbExecuted;
bool cb2Executed;
bool srvExecuted;
bool responseExecuted;
int counter = 0;
bool terminatePub = false;

//////////////////////////////////////////////////
/// \brief Initialize some global variables.
void reset()
{
  cbExecuted = false;
  cb2Executed = false;
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  terminatePub = false;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb(const std::string &_topic, const transport::msgs::Int &_msg)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_msg.data(), data);
  cbExecuted = true;
  counter++;
}

//////////////////////////////////////////////////
/// \brief Function called each time a topic update is received.
void cb2(const std::string &_topic, const transport::msgs::Int &_msg)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_msg.data(), data);
  cb2Executed = true;
}

//////////////////////////////////////////////////
/// \brief Provide a service call.
void srvEcho(const std::string &_topic, const transport::msgs::Int &_req,
  transport::msgs::Int &_rep, bool &_result)
{
  EXPECT_EQ(_topic, topic);
  srvExecuted = true;

  EXPECT_EQ(_req.data(), data);
  _rep.set_data(_req.data());
  _result = true;
}

//////////////////////////////////////////////////
/// \brief Service call response callback.
void response(const std::string &_topic, const transport::msgs::Int &_rep,
  bool _result)
{
  EXPECT_EQ(_topic, topic);
  EXPECT_EQ(_rep.data(), data);
  EXPECT_TRUE(_result);

  responseExecuted = true;
  ++counter;
}

//////////////////////////////////////////////////
/// \brief A class for testing subscription passing a member function
/// as a callback.
class MyTestClass
{
  /// \brief Class constructor.
  public: MyTestClass()
    : callbackExecuted(false),
      callbackSrvExecuted(false)
  {
    // Subscribe to an illegal topic.
    EXPECT_FALSE(node.Subscribe("invalid topic", &MyTestClass::Cb, this));

    EXPECT_TRUE(this->node.Subscribe(topic, &MyTestClass::Cb, this));
  }

  // Member function used as a callback for responding to a service call.
  public: void Echo(const std::string &_topic,
    const transport::msgs::Int &_req, transport::msgs::Int &_rep,
    bool &_result)
  {
    EXPECT_EQ(_topic, topic);
    EXPECT_EQ(_req.data(), data);
    _rep.set_data(_req.data());
    _result = true;
    this->callbackSrvExecuted = true;
  }

  /// \brief Member function called each time a topic update is received.
  public: void Cb(const std::string &_topic,
    const transport::msgs::Int &_msg)
  {
    EXPECT_EQ(_topic, topic);
    EXPECT_EQ(_msg.data(), data);
    this->callbackExecuted = true;
  };

  /// \brief Advertise a topic and publish a message.
  public: void SendSomeData()
  {
    transport::msgs::Int msg;
    msg.set_data(data);

    // Advertise an illegal topic.
    EXPECT_FALSE(node.Advertise("invalid topic"));

    EXPECT_TRUE(this->node.Advertise(topic));
    EXPECT_TRUE(this->node.Publish(topic, msg));
  }

  public: void TestServiceCall()
  {
    transport::msgs::Int req;
    transport::msgs::Int rep;
    int timeout = 500;
    bool result;

    req.set_data(data);

    // Advertise an illegal service name.
    EXPECT_FALSE(node.Advertise("invalid service", &MyTestClass::Echo, this));

    EXPECT_TRUE(this->node.Advertise(topic, &MyTestClass::Echo, this));
    EXPECT_TRUE(this->node.Request(topic, req, timeout, rep, result));
    ASSERT_TRUE(result);
    EXPECT_EQ(rep.data(), data);
  }

  /// \brief Member variable that flags when the callback is executed.
  public: bool callbackExecuted;
  public: bool callbackSrvExecuted;

  /// \brief Transport node;
  private: transport::Node node;
};

//////////////////////////////////////////////////
/// \brief Create a subscriber and wait for a callback to be executed.
void CreateSubscriber()
{
  transport::Node node;
  EXPECT_TRUE(node.Subscribe(topic, cb));

  int i = 0;
  while (i < 100 && !cbExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }
}

//////////////////////////////////////////////////
/// \brief Use two threads using their own transport nodes. One thread
/// will publish a message, whereas the other thread is subscribed to the topic.
/// \param[in] _scope Scope used to advertise the topic.
void CreatePubSubTwoThreads(const transport::Scope &_sc = transport::Scope::All)
{
  reset();

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, _sc));

  // Subscribe to a topic in a different thread and wait until the callback is
  // received.
  std::thread subscribeThread(CreateSubscriber);

  // Wait some time until the subscriber is alive.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a msg on topic.
  EXPECT_TRUE(node.Publish(topic, msg));

  // Wait until the subscribe thread finishes.
  subscribeThread.join();

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);
}

//////////////////////////////////////////////////
/// \brief A message should not be published if it is not advertised before.
TEST(NodeTest, PubWithoutAdvertise)
{
  reset();

  transport::msgs::Int msg;
  msg.set_data(data);

  // Check that an invalid namespace is ignored. The callbacks are expecting an
  // empty namespace.
  transport::Node node1("invalid namespace");
  transport::Node node2;

  // Check the advertised/subscribed topics and advertised services.
  EXPECT_EQ(node1.GetAdvertisedTopics().size(), 0);
  EXPECT_EQ(node1.GetSubscribedTopics().size(), 0);
  EXPECT_EQ(node1.GetAdvertisedServices().size(), 0);

  // Publish some data on topic without advertising it first.
  EXPECT_FALSE(node1.Publish(topic, msg));

  EXPECT_TRUE(node1.Advertise(topic));

  auto v = node1.GetAdvertisedTopics();
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0), topic);

  EXPECT_TRUE(node2.Advertise(topic));
  v = node2.GetAdvertisedTopics();
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0), topic);

  EXPECT_TRUE(node2.Subscribe(topic, cb));
  auto m = node2.GetSubscribedTopics();
  ASSERT_EQ(m.size(), 1);
  EXPECT_EQ(m.begin()->first, topic);

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Publish a message by each node.
  EXPECT_TRUE(node1.Publish(topic, msg));
  EXPECT_TRUE(node2.Publish(topic, msg));

  // Wait some time for the messages to arrive.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was received twice.
  EXPECT_TRUE(cbExecuted);
  EXPECT_EQ(counter, 2);
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, PubSubSameThread)
{
  reset();

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node;

  // Advertise an illegal topic.
  EXPECT_FALSE(node.Advertise("invalid topic"));

  EXPECT_TRUE(node.Advertise(topic));

  // Subscribe to an illegal topic.
  EXPECT_FALSE(node.Subscribe("invalid topic", cb));

  EXPECT_TRUE(node.Subscribe(topic, cb));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Try to publish a message using an invalid topic.
  EXPECT_FALSE(node.Publish("invalid topic", msg));

  // Publish a first message.
  EXPECT_TRUE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the message was received.
  EXPECT_TRUE(cbExecuted);

  reset();

  // Publish a second message on topic.
  EXPECT_TRUE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the data was received.
  EXPECT_TRUE(cbExecuted);

  reset();

  // Unadvertise an illegal topic.
  EXPECT_FALSE(node.Unadvertise("invalid topic"));

  EXPECT_TRUE(node.Unadvertise(topic));

  // Publish a third message.
  EXPECT_FALSE(node.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(cbExecuted);
}

//////////////////////////////////////////////////
/// \brief Use two threads using their own transport nodes. One thread
/// will publish a message, whereas the other thread is subscribed to the topic.
TEST(NodeTest, PubSubTwoThreadsSameTopic)
{
  CreatePubSubTwoThreads();
}

//////////////////////////////////////////////////
/// \brief Use two different transport node on the same thread. Check that
/// both receive the updates when they are subscribed to the same topic. Check
/// also that when one of the nodes unsubscribes, no longer receives updates.
TEST(NodeTest, PubSubOneThreadTwoSubs)
{
  reset();

  transport::msgs::Int msg;
  msg.set_data(data);

  transport::Node node1;
  transport::Node node2;

  EXPECT_TRUE(node1.Advertise(topic));

  // Subscribe to topic in node1.
  EXPECT_TRUE(node1.Subscribe(topic, cb));

  // Subscribe to topic in node2.
  EXPECT_TRUE(node2.Subscribe(topic, cb2));

  // Wait some time before publishing.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was received by node1.
  EXPECT_TRUE(cbExecuted);
  // Check that the msg was received by node2.
  EXPECT_TRUE(cb2Executed);

  auto m = node1.GetSubscribedTopics();
  ASSERT_EQ(m.size(), 1);
  EXPECT_EQ(m.begin()->first, topic);

  reset();

  // Try to unsubscribe from an invalid topic.
  EXPECT_FALSE(node1.Unsubscribe("invalid topic"));

  // Node1 is not interested in the topic anymore.
  EXPECT_TRUE(node1.Unsubscribe(topic));

  // Give some time to receive the unsubscription.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Publish a second message.
  EXPECT_TRUE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check that the msg was no received by node1.
  EXPECT_FALSE(cbExecuted);
  // Check that the msg was received by node2.
  EXPECT_TRUE(cb2Executed);

  ASSERT_EQ(node1.GetSubscribedTopics().size(), 0);

  reset();

  EXPECT_TRUE(node1.Unadvertise(topic));

  // Publish a third message
  EXPECT_FALSE(node1.Publish(topic, msg));

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Anybody should have received the message.
  EXPECT_FALSE(cbExecuted);
  EXPECT_FALSE(cb2Executed);

  auto v = node1.GetAdvertisedServices();
  ASSERT_EQ(v.size(), 0);
}

//////////////////////////////////////////////////
/// \brief Use the transport inside a class and check advertise, subscribe and
/// publish.
TEST(NodeTest, ClassMemberCallback)
{
  MyTestClass client;

  // Wait for the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  client.SendSomeData();

  // Give some time to the subscribers.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(client.callbackExecuted);

  client.TestServiceCall();

  EXPECT_TRUE(client.callbackSrvExecuted);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in different threads are able to communicate
/// advertising a topic with "Process" scope.
TEST(NodeTest, ScopeProcess)
{
  CreatePubSubTwoThreads(transport::Scope::Process);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in diffetent threads are able to communicate
/// advertising a topic with "Host" scope.
TEST(NodeTest, ScopeHost)
{
  CreatePubSubTwoThreads(transport::Scope::Host);
}

//////////////////////////////////////////////////
/// \brief Check that two nodes in diffetent threads are able to communicate
/// advertising a topic with "All" scope.
TEST(NodeTest, ScopeAll)
{
  CreatePubSubTwoThreads(transport::Scope::All);
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallAsync)
{
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  transport::msgs::Int req;
  req.set_data(data);

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvEcho));

  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  auto v = node.GetAdvertisedServices();
  ASSERT_EQ(v.size(), 1);
  EXPECT_EQ(v.at(0), topic);

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, response));

  EXPECT_TRUE(node.Request(topic, req, response));

  int i = 0;
  while (i < 100 && !srvExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 1);

  // Make another request.
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  EXPECT_TRUE(node.Request(topic, req, response));

  i = 0;
  while (i < 100 && !responseExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 1);

  // Try to unadvertise an invalid service.
  EXPECT_FALSE(node.UnadvertiseSrv("invalid service"));

  EXPECT_TRUE(node.UnadvertiseSrv(topic));

  ASSERT_EQ(node.GetAdvertisedServices().size(), 0);
}

//////////////////////////////////////////////////
/// \brief Request multiple service calls at the same time.
TEST(NodeTest, MultipleServiceCallAsync)
{
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  transport::msgs::Int req;
  req.set_data(data);

  transport::Node node;

  // Advertise an invalid service name.
  EXPECT_FALSE(node.Advertise("invalid service", srvEcho));

  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, response));

  EXPECT_TRUE(node.Request(topic, req, response));

  int i = 0;
  while (i < 100 && !srvExecuted)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 1);

  // Make another request.
  srvExecuted = false;
  responseExecuted = false;
  counter = 0;
  EXPECT_TRUE(node.Request(topic, req, response));
  EXPECT_TRUE(node.Request(topic, req, response));
  EXPECT_TRUE(node.Request(topic, req, response));

  i = 0;
  while (i < 100 && counter < 3)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++i;
  }

  // Check that the service call response was executed.
  EXPECT_TRUE(responseExecuted);
  EXPECT_TRUE(srvExecuted);
  EXPECT_EQ(counter, 3);

  // Try to unadvertise an invalid service.
  EXPECT_FALSE(node.UnadvertiseSrv("invalid service"));

  EXPECT_TRUE(node.UnadvertiseSrv(topic));
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallSync)
{
  transport::msgs::Int req;
  transport::msgs::Int rep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;
  EXPECT_TRUE(node.Advertise(topic, srvEcho));

  // Request an invalid service name.
  EXPECT_FALSE(node.Request("invalid service", req, timeout, rep, result));

  EXPECT_TRUE(node.Request(topic, req, timeout, rep, result));

  // Check that the service call response was executed.
  EXPECT_TRUE(result);
  EXPECT_EQ(rep.data(), req.data());
}

//////////////////////////////////////////////////
/// \brief A thread can create a node, and send and receive messages.
TEST(NodeTest, ServiceCallSyncTimeout)
{
  transport::msgs::Int req;
  transport::msgs::Int rep;
  bool result;
  unsigned int timeout = 1000;

  req.set_data(data);

  transport::Node node;

  auto t1 = std::chrono::system_clock::now();
  bool executed = node.Request(topic, req, timeout, rep, result);
  auto t2 = std::chrono::system_clock::now();

  double elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // Check if the elapsed time was close to the timeout.
  EXPECT_NEAR(elapsed, timeout, 5.0);

  // Check that the service call response was not executed.
  EXPECT_FALSE(executed);
}

//////////////////////////////////////////////////
/// \brief Create a publisher that sends messages "forever". This function will
/// be used emiting a SIGINT or SIGTERM signal, to make sure that the transport
/// library captures the signals, stop all the tasks and signal the event with
/// the method Interrupted().
void createInfinitePublisher()
{
  transport::msgs::Int msg;
  msg.set_data(data);
  transport::Node node;

  EXPECT_TRUE(node.Advertise(topic));

  auto i = 0;
  while (!terminatePub)
  {
    EXPECT_TRUE(node.Publish(topic, msg));
    ++i;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_LT(i, 200);
}

void signal_handler(int _signal)
{
  if (_signal == SIGINT || _signal == SIGTERM)
    terminatePub = true;
}

//////////////////////////////////////////////////
/// \brief Check that an external program can capture a SIGINT and terminate
/// the program without problems.
TEST(NodeTest, SigIntTermination)
{
  reset();

  // Install a signal handler for SIGINT.
  std::signal(SIGINT, signal_handler);

  auto thread = std::thread(createInfinitePublisher);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::raise(SIGINT);
  thread.join();
}

//////////////////////////////////////////////////
/// \brief Check that an external program can capture a SIGTERM and terminate
/// the program without problems.
TEST(NodeTest, SigTermTermination)
{
  reset();

  // Install a signal handler for SIGINT.
  std::signal(SIGTERM, signal_handler);

  auto thread = std::thread(createInfinitePublisher);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::raise(SIGINT);
  thread.join();
}


//////////////////////////////////////////////////
int main(int argc, char **argv)
{
  // Enable verbose mode.
  setenv("IGN_VERBOSE", "1", 1);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
