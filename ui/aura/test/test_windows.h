// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_TEST_WINDOWS_H_
#define UI_AURA_TEST_TEST_WINDOWS_H_
#pragma once

#include "base/compiler_specific.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/aura/client/window_types.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/aura_test_base.h"

namespace aura {
namespace test {

Window* CreateTestWindowWithId(int id, Window* parent);
Window* CreateTestWindowWithBounds(const gfx::Rect& bounds, Window* parent);
Window* CreateTestWindow(SkColor color,
                         int id,
                         const gfx::Rect& bounds,
                         Window* parent);
Window* CreateTestWindowWithDelegate(WindowDelegate* delegate,
                                     int id,
                                     const gfx::Rect& bounds,
                                     Window* parent);
Window* CreateTestWindowWithDelegateAndType(WindowDelegate* delegate,
                                            client::WindowType type,
                                            int id,
                                            const gfx::Rect& bounds,
                                            Window* parent);

// Creates a transient child window of |parent|.
Window* CreateTransientChild(int id, Window* parent);

// Returns true if |upper| is above |lower| in the window stacking order.
bool WindowIsAbove(Window* upper, Window* lower);

// Returns true if |upper|'s layer is above |lower|'s layer in the layer
// stacking order.
bool LayerIsAbove(Window* upper, Window* lower);

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_TEST_WINDOWS_H_
