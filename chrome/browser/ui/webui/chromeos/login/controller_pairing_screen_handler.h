// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_CONTROLLER_PAIRING_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_CONTROLLER_PAIRING_SCREEN_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/controller_pairing_screen_actor.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

class ControllerPairingScreenHandler : public ControllerPairingScreenActor,
                                       public BaseScreenHandler {
 public:
  ControllerPairingScreenHandler();
  virtual ~ControllerPairingScreenHandler();

 private:
  void HandleUserActed(const std::string& action);
  void HandleContextChanged(const base::DictionaryValue* diff);

  // Overridden from BaseScreenHandler:
  virtual void Initialize() override;
  virtual void DeclareLocalizedValues(LocalizedValuesBuilder* builder) override;

  // Overridden from content::WebUIMessageHandler:
  virtual void RegisterMessages() override;

  // Overridden from ControllerPairingScreenActor:
  virtual void Show() override;
  virtual void Hide() override;
  virtual void SetDelegate(Delegate* delegate) override;
  virtual void OnContextChanged(const base::DictionaryValue& diff) override;
  virtual content::BrowserContext* GetBrowserContext() override;

  ControllerPairingScreenActor::Delegate* delegate_;
  bool show_on_init_;

  DISALLOW_COPY_AND_ASSIGN(ControllerPairingScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_CONTROLLER_PAIRING_SCREEN_HANDLER_H_
