// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_WEB_CONTENTS_MAIN_FRAME_OBSERVER_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_WEB_CONTENTS_MAIN_FRAME_OBSERVER_H_

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace dom_distiller {

// Tracks whether DocumentLoadedInFrame has been called for the main frame for
// the current main frame for the given WebContents. It removes itself as an
// observer if the WebContents is destroyed or the render process is gone.
class WebContentsMainFrameObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebContentsMainFrameObserver> {
 public:
  virtual ~WebContentsMainFrameObserver();

  bool is_document_loaded_in_main_frame() {
    return is_document_loaded_in_main_frame_;
  }

  bool is_initialized() { return is_initialized_; }

  // content::WebContentsObserver implementation.
  virtual void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;
  virtual void DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) override;
  virtual void RenderProcessGone(base::TerminationStatus status) override;

 private:
  explicit WebContentsMainFrameObserver(content::WebContents* web_contents);
  friend class content::WebContentsUserData<WebContentsMainFrameObserver>;

  // Removes the observer and clears the WebContents member.
  void CleanUp();

  // Whether DocumentLoadedInFrame has been called for the tracked WebContents
  // for the current main frame. This is cleared when the main frame navigates,
  // and set again when DocumentLoadedInFrame is called for the main frame.
  bool is_document_loaded_in_main_frame_;

  // Whether this object has been correctly initialized. This is set as soon as
  // at least one call to DidNavigateMainFrame has happened.
  bool is_initialized_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsMainFrameObserver);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CONTENT_WEB_CONTENTS_MAIN_FRAME_OBSERVER_H_
