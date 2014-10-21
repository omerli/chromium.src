// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_TEST_INTEGRATION_MIGRATION_WAITER_H_
#define CHROME_BROWSER_SYNC_TEST_INTEGRATION_MIGRATION_WAITER_H_

#include "base/macros.h"
#include "chrome/browser/sync/test/integration/status_change_checker.h"
#include "sync/internal_api/public/base/model_type.h"

class MigrationWatcher;

// Helper class that checks if the sync backend has successfully completed
// migration for a set of data types.
//
// Collaborates with the MigrationWatcher, defined above.
class MigrationWaiter : public StatusChangeChecker {
 public:
  // Initialize a waiter that will wait until |watcher|'s migrated types
  // match the provided |exptected_types|.
  MigrationWaiter(syncer::ModelTypeSet expected_types,
                  MigrationWatcher* watcher);

  ~MigrationWaiter() override;

  // Implementation of StatusChangeChecker.
  bool IsExitConditionSatisfied() override;
  std::string GetDebugMessage() const override;

  // Function to trigger the waiting.
  void Wait();

  // Callback invoked by our associated waiter when migration state changes.
  void OnMigrationStateChange();

 private:
  // The MigrationWatcher we're observering.
  MigrationWatcher* const watcher_;

  // The set of data types that are expected to eventually undergo migration.
  const syncer::ModelTypeSet expected_types_;

  DISALLOW_COPY_AND_ASSIGN(MigrationWaiter);
};

#endif  // CHROME_BROWSER_SYNC_TEST_INTEGRATION_MIGRATION_WAITER_H_
