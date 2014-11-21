// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/browser_compositor_view_mac.h"

#include "base/debug/trace_event.h"
#include "base/lazy_instance.h"
#include "content/browser/compositor/browser_compositor_ca_layer_tree_mac.h"
#include "content/browser/renderer_host/render_widget_resize_helper.h"
#include "content/public/browser/context_factory.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserCompositorMac

namespace content {

namespace {

// The number of placeholder objects allocated. If this reaches zero, then
// the BrowserCompositorMac being held on to for recycling,
// |g_recyclable_browser_compositor|, will be freed.
uint32 g_placeholder_count = 0;

// A spare BrowserCompositorMac kept around for recycling.
base::LazyInstance<scoped_ptr<BrowserCompositorMac>>
  g_recyclable_browser_compositor;

}  // namespace

BrowserCompositorMac::BrowserCompositorMac()
    : compositor_(
          accelerated_widget_mac_.accelerated_widget(),
          content::GetContextFactory(),
          RenderWidgetResizeHelper::Get()->task_runner()) {
  compositor_.SetVisible(false);
}

// static
scoped_ptr<BrowserCompositorMac> BrowserCompositorMac::Create() {
  if (g_recyclable_browser_compositor.Get())
    return g_recyclable_browser_compositor.Get().Pass();
  return scoped_ptr<BrowserCompositorMac>(new BrowserCompositorMac).Pass();
}

// static
void BrowserCompositorMac::Recycle(
    scoped_ptr<BrowserCompositorMac> compositor) {
  DCHECK(compositor);

  // Make this BrowserCompositorMac recyclable for future instances.
  g_recyclable_browser_compositor.Get().swap(compositor);

  // If there are no placeholders allocated, destroy the recyclable
  // BrowserCompositorMac that we just populated.
  if (!g_placeholder_count)
    g_recyclable_browser_compositor.Get().reset();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserCompositorMacPlaceholder

BrowserCompositorMacPlaceholder::BrowserCompositorMacPlaceholder() {
  g_placeholder_count += 1;
}

BrowserCompositorMacPlaceholder::~BrowserCompositorMacPlaceholder() {
  DCHECK_GT(g_placeholder_count, 0u);
  g_placeholder_count -= 1;

  // If there are no placeholders allocated, destroy the recyclable
  // BrowserCompositorMac.
  if (!g_placeholder_count)
    g_recyclable_browser_compositor.Get().reset();
}

}  // namespace content
