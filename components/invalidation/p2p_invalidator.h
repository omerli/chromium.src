// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// An invalidator that uses p2p invalidations based on XMPP push
// notifications.  Used only for sync integration tests.

#ifndef COMPONENTS_INVALIDATION_P2P_INVALIDATOR_H_
#define COMPONENTS_INVALIDATION_P2P_INVALIDATOR_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "components/invalidation/invalidation_export.h"
#include "components/invalidation/invalidator.h"
#include "components/invalidation/invalidator_registrar.h"
#include "components/invalidation/invalidator_state.h"
#include "components/invalidation/object_id_invalidation_map.h"
#include "jingle/notifier/base/notifier_options.h"
#include "jingle/notifier/listener/push_client.h"
#include "jingle/notifier/listener/push_client_observer.h"

namespace notifier {
class PushClient;
}  // namespace notifier

namespace syncer {

// The channel to use for sync notifications.
INVALIDATION_EXPORT extern const char kSyncP2PNotificationChannel[];

// The intended recipient(s) of a P2P notification.
enum P2PNotificationTarget {
  NOTIFY_SELF,
  FIRST_NOTIFICATION_TARGET = NOTIFY_SELF,
  NOTIFY_OTHERS,
  NOTIFY_ALL,
  LAST_NOTIFICATION_TARGET = NOTIFY_ALL
};

INVALIDATION_EXPORT_PRIVATE std::string P2PNotificationTargetToString(
    P2PNotificationTarget target);

// If |target_str| can't be parsed, assumes NOTIFY_SELF.
INVALIDATION_EXPORT_PRIVATE P2PNotificationTarget
P2PNotificationTargetFromString(const std::string& target_str);

// Helper notification data class that can be serialized to and
// deserialized from a string.
class INVALIDATION_EXPORT_PRIVATE P2PNotificationData {
 public:
  // Initializes with an empty sender ID, target set to NOTIFY_SELF,
  // and empty changed types.
  P2PNotificationData();
  P2PNotificationData(const std::string& sender_id,
                      P2PNotificationTarget target,
                      const ObjectIdInvalidationMap& invalidation_map);

  ~P2PNotificationData();

  // Returns true if the given ID is targeted by this notification.
  bool IsTargeted(const std::string& id) const;

  const ObjectIdInvalidationMap& GetIdInvalidationMap() const;

  bool Equals(const P2PNotificationData& other) const;

  std::string ToString() const;

  // Returns whether parsing |str| was successful.  If parsing was
  // unsuccessful, the state of the notification is undefined.
  bool ResetFromString(const std::string& str);

 private:
  // The unique ID of the client that sent the notification.
  std::string sender_id_;
  // The intendent recipient(s) of the notification.
  P2PNotificationTarget target_;
  // The invalidation map for the notification.
  ObjectIdInvalidationMap invalidation_map_;
};

class INVALIDATION_EXPORT_PRIVATE P2PInvalidator
    : public Invalidator,
      public NON_EXPORTED_BASE(notifier::PushClientObserver) {
 public:
  // The |send_notification_target| parameter was added to allow us to send
  // self-notifications in some cases, but not others.  The value should be
  // either NOTIFY_ALL to send notifications to all clients, or NOTIFY_OTHERS
  // to send notifications to all clients except for the one that triggered the
  // notification.  See crbug.com/97780.
  P2PInvalidator(scoped_ptr<notifier::PushClient> push_client,
                 const std::string& invalidator_client_id,
                 P2PNotificationTarget send_notification_target);

  virtual ~P2PInvalidator();

  // Invalidator implementation.
  virtual void RegisterHandler(InvalidationHandler* handler) override;
  virtual void UpdateRegisteredIds(InvalidationHandler* handler,
                                   const ObjectIdSet& ids) override;
  virtual void UnregisterHandler(InvalidationHandler* handler) override;
  virtual InvalidatorState GetInvalidatorState() const override;
  virtual void UpdateCredentials(
      const std::string& email, const std::string& token) override;
  virtual void RequestDetailedStatus(
      base::Callback<void(const base::DictionaryValue&)> callback) const
      override;

  // PushClientObserver implementation.
  virtual void OnNotificationsEnabled() override;
  virtual void OnNotificationsDisabled(
      notifier::NotificationsDisabledReason reason) override;
  virtual void OnIncomingNotification(
      const notifier::Notification& notification) override;

  void SendInvalidation(const ObjectIdSet& ids);

  void SendNotificationDataForTest(
      const P2PNotificationData& notification_data);

 private:
  void SendNotificationData(const P2PNotificationData& notification_data);

  base::ThreadChecker thread_checker_;

  InvalidatorRegistrar registrar_;

  // The push client.
  scoped_ptr<notifier::PushClient> push_client_;
  // Our unique ID.
  std::string invalidator_client_id_;
  // Whether we have called UpdateCredentials() yet.
  bool logged_in_;
  bool notifications_enabled_;
  // Which set of clients should be sent notifications.
  P2PNotificationTarget send_notification_target_;

  DISALLOW_COPY_AND_ASSIGN(P2PInvalidator);
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_P2P_INVALIDATOR_H_
