// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WIFI_FAKE_WIFI_SERVICE_H_
#define COMPONENTS_WIFI_FAKE_WIFI_SERVICE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/wifi/network_properties.h"
#include "components/wifi/wifi_service.h"

namespace wifi {

// Fake implementation of WiFiService used to satisfy expectations of
// networkingPrivateApi browser test.
class FakeWiFiService : public WiFiService {
 public:
  FakeWiFiService();
  virtual ~FakeWiFiService();

  virtual void Initialize(
      scoped_refptr<base::SequencedTaskRunner> task_runner) override;
  virtual void UnInitialize() override;
  virtual void GetProperties(const std::string& network_guid,
                             base::DictionaryValue* properties,
                             std::string* error) override;
  virtual void GetManagedProperties(const std::string& network_guid,
                                    base::DictionaryValue* managed_properties,
                                    std::string* error) override;
  virtual void GetState(const std::string& network_guid,
                        base::DictionaryValue* properties,
                        std::string* error) override;
  virtual void SetProperties(const std::string& network_guid,
                             scoped_ptr<base::DictionaryValue> properties,
                             std::string* error) override;
  virtual void CreateNetwork(bool shared,
                             scoped_ptr<base::DictionaryValue> properties,
                             std::string* network_guid,
                             std::string* error) override;
  virtual void GetVisibleNetworks(const std::string& network_type,
                                  base::ListValue* network_list,
                                  bool include_details) override;
  virtual void RequestNetworkScan() override;
  virtual void StartConnect(const std::string& network_guid,
                            std::string* error) override;
  virtual void StartDisconnect(const std::string& network_guid,
                               std::string* error) override;
  virtual void GetKeyFromSystem(const std::string& network_guid,
                                std::string* key_data,
                                std::string* error) override;
  virtual void SetEventObservers(
      scoped_refptr<base::MessageLoopProxy> message_loop_proxy,
      const NetworkGuidListCallback& networks_changed_observer,
      const NetworkGuidListCallback& network_list_changed_observer) override;
  virtual void RequestConnectedNetworkUpdate() override;

 private:
  NetworkList::iterator FindNetwork(const std::string& network_guid);

  void DisconnectAllNetworksOfType(const std::string& type);

  void SortNetworks();

  void NotifyNetworkListChanged(const NetworkList& networks);

  void NotifyNetworkChanged(const std::string& network_guid);

  NetworkList networks_;
  scoped_refptr<base::MessageLoopProxy> message_loop_proxy_;
  NetworkGuidListCallback networks_changed_observer_;
  NetworkGuidListCallback network_list_changed_observer_;

  DISALLOW_COPY_AND_ASSIGN(FakeWiFiService);
};

}  // namespace wifi

#endif  // COMPONENTS_WIFI_FAKE_WIFI_SERVICE_H_
