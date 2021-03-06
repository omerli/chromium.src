// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/histogram_manager.h"

#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_samples.h"
#include "components/metrics/histogram_encoder.h"

namespace metrics {

// TODO(rtenneti): move g_histogram_manager into java code.
static base::LazyInstance<HistogramManager>::Leaky
    g_histogram_manager = LAZY_INSTANCE_INITIALIZER;

HistogramManager::HistogramManager() : histogram_snapshot_manager_(this) {
}

HistogramManager::~HistogramManager() {
}

// static
HistogramManager* HistogramManager::GetInstance() {
  return g_histogram_manager.Pointer();
}

void HistogramManager::RecordDelta(const base::HistogramBase& histogram,
                                   const base::HistogramSamples& snapshot) {
  EncodeHistogramDelta(histogram.histogram_name(), snapshot, &uma_proto_);
}

void HistogramManager::InconsistencyDetected(
    base::HistogramBase::Inconsistency problem) {
  UMA_HISTOGRAM_ENUMERATION("Histogram.InconsistenciesBrowser.Cronet",
                            problem, base::HistogramBase::NEVER_EXCEEDED_VALUE);
}

void HistogramManager::UniqueInconsistencyDetected(
    base::HistogramBase::Inconsistency problem) {
  UMA_HISTOGRAM_ENUMERATION("Histogram.InconsistenciesBrowserUnique.Cronet",
                            problem, base::HistogramBase::NEVER_EXCEEDED_VALUE);
}

void HistogramManager::InconsistencyDetectedInLoggedCount(int amount) {
  UMA_HISTOGRAM_COUNTS("Histogram.InconsistentSnapshotBrowser.Cronet",
                       std::abs(amount));
}

bool HistogramManager::GetDeltas(std::vector<uint8>* data) {
  // Clear the protobuf between calls.
  uma_proto_.Clear();
  histogram_snapshot_manager_.PrepareDeltas(
      base::Histogram::kNoFlags, base::Histogram::kUmaTargetedHistogramFlag);
  int32_t data_size = uma_proto_.ByteSize();
  data->resize(data_size);
  if (uma_proto_.SerializeToArray(&(*data)[0], data_size))
    return true;
  data->clear();
  return false;
}

}  // namespace metrics
