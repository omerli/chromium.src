// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/message_channel.h"

#include <cstdlib>
#include <string>

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "content/renderer/pepper/host_array_buffer_var.h"
#include "content/renderer/pepper/pepper_plugin_instance_impl.h"
#include "content/renderer/pepper/pepper_try_catch.h"
#include "content/renderer/pepper/plugin_module.h"
#include "content/renderer/pepper/plugin_object.h"
#include "content/renderer/pepper/v8_var_converter.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "gin/public/gin_embedders.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/scoped_pp_var.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/shared_impl/var_tracker.h"
#include "third_party/WebKit/public/web/WebBindings.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebDOMMessageEvent.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebNode.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/WebKit/public/web/WebSerializedScriptValue.h"
#include "v8/include/v8.h"

using ppapi::ArrayBufferVar;
using ppapi::PpapiGlobals;
using ppapi::ScopedPPVar;
using ppapi::StringVar;
using blink::WebBindings;
using blink::WebElement;
using blink::WebDOMEvent;
using blink::WebDOMMessageEvent;
using blink::WebPluginContainer;
using blink::WebSerializedScriptValue;

namespace content {

namespace {

const char kPostMessage[] = "postMessage";
const char kPostMessageAndAwaitResponse[] = "postMessageAndAwaitResponse";
const char kV8ToVarConversionError[] =
    "Failed to convert a PostMessage "
    "argument from a JavaScript value to a PP_Var. It may have cycles or be of "
    "an unsupported type.";
const char kVarToV8ConversionError[] =
    "Failed to convert a PostMessage "
    "argument from a PP_Var to a Javascript value. It may have cycles or be of "
    "an unsupported type.";

}  // namespace

// MessageChannel --------------------------------------------------------------
struct MessageChannel::VarConversionResult {
  VarConversionResult() : success_(false), conversion_completed_(false) {}
  void ConversionCompleted(const ScopedPPVar& var,
                           bool success) {
    conversion_completed_ = true;
    var_ = var;
    success_ = success;
  }
  const ScopedPPVar& var() const { return var_; }
  bool success() const { return success_; }
  bool conversion_completed() const { return conversion_completed_; }

 private:
  ScopedPPVar var_;
  bool success_;
  bool conversion_completed_;
};

// static
gin::WrapperInfo MessageChannel::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
MessageChannel* MessageChannel::Create(PepperPluginInstanceImpl* instance,
                                       v8::Persistent<v8::Object>* result) {
  MessageChannel* message_channel = new MessageChannel(instance);
  v8::HandleScope handle_scope(instance->GetIsolate());
  v8::Context::Scope context_scope(instance->GetContext());
  gin::Handle<MessageChannel> handle =
      gin::CreateHandle(instance->GetIsolate(), message_channel);
  result->Reset(instance->GetIsolate(), handle.ToV8()->ToObject());
  return message_channel;
}

MessageChannel::~MessageChannel() {
  passthrough_object_.Reset();
  if (instance_)
    instance_->MessageChannelDestroyed();
}

void MessageChannel::InstanceDeleted() {
  instance_ = NULL;
}

void MessageChannel::PostMessageToJavaScript(PP_Var message_data) {
  v8::HandleScope scope(v8::Isolate::GetCurrent());

  // Because V8 is probably not on the stack for Native->JS calls, we need to
  // enter the appropriate context for the plugin.
  v8::Local<v8::Context> context = instance_->GetContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  v8::Handle<v8::Value> v8_val;
  if (!V8VarConverter(instance_->pp_instance())
           .ToV8Value(message_data, context, &v8_val)) {
    PpapiGlobals::Get()->LogWithSource(instance_->pp_instance(),
                                       PP_LOGLEVEL_ERROR,
                                       std::string(),
                                       kVarToV8ConversionError);
    return;
  }

  WebSerializedScriptValue serialized_val =
      WebSerializedScriptValue::serialize(v8_val);

  if (early_message_queue_state_ != SEND_DIRECTLY) {
    // We can't just PostTask here; the messages would arrive out of
    // order. Instead, we queue them up until we're ready to post
    // them.
    early_message_queue_.push_back(serialized_val);
  } else {
    // The proxy sent an asynchronous message, so the plugin is already
    // unblocked. Therefore, there's no need to PostTask.
    DCHECK(early_message_queue_.empty());
    PostMessageToJavaScriptImpl(serialized_val);
  }
}

void MessageChannel::Start() {
  // We PostTask here instead of draining the message queue directly
  // since we haven't finished initializing the PepperWebPluginImpl yet, so
  // the plugin isn't available in the DOM.
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&MessageChannel::DrainEarlyMessageQueue,
                 weak_ptr_factory_.GetWeakPtr()));
}

void MessageChannel::SetPassthroughObject(v8::Handle<v8::Object> passthrough) {
  passthrough_object_.Reset(instance_->GetIsolate(), passthrough);
}

void MessageChannel::SetReadOnlyProperty(PP_Var key, PP_Var value) {
  StringVar* key_string = StringVar::FromPPVar(key);
  if (key_string) {
    internal_named_properties_[key_string->value()] = ScopedPPVar(value);
  } else {
    NOTREACHED();
  }
}

MessageChannel::MessageChannel(PepperPluginInstanceImpl* instance)
    : gin::NamedPropertyInterceptor(instance->GetIsolate(), this),
      instance_(instance),
      early_message_queue_state_(QUEUE_MESSAGES),
      weak_ptr_factory_(this) {
}

gin::ObjectTemplateBuilder MessageChannel::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<MessageChannel>::GetObjectTemplateBuilder(isolate)
      .AddNamedPropertyInterceptor();
}

v8::Local<v8::Value> MessageChannel::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& identifier) {
  if (!instance_)
    return v8::Local<v8::Value>();

  PepperTryCatchV8 try_catch(instance_, V8VarConverter::kDisallowObjectVars,
                             isolate);
  if (identifier == kPostMessage) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&MessageChannel::PostMessageToNative,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  } else if (identifier == kPostMessageAndAwaitResponse) {
    return gin::CreateFunctionTemplate(isolate,
        base::Bind(&MessageChannel::PostBlockingMessageToNative,
                   weak_ptr_factory_.GetWeakPtr()))->GetFunction();
  }

  std::map<std::string, ScopedPPVar>::const_iterator it =
      internal_named_properties_.find(identifier);
  if (it != internal_named_properties_.end()) {
    v8::Handle<v8::Value> result = try_catch.ToV8(it->second.get());
    if (try_catch.ThrowException())
      return v8::Local<v8::Value>();
    return result;
  }

  PluginObject* plugin_object = GetPluginObject(isolate);
  if (plugin_object)
    return plugin_object->GetNamedProperty(isolate, identifier);
  return v8::Local<v8::Value>();
}

bool MessageChannel::SetNamedProperty(v8::Isolate* isolate,
                                      const std::string& identifier,
                                      v8::Local<v8::Value> value) {
  if (!instance_)
    return false;
  PepperTryCatchV8 try_catch(instance_, V8VarConverter::kDisallowObjectVars,
                             isolate);
  if (identifier == kPostMessage ||
      (identifier == kPostMessageAndAwaitResponse)) {
    try_catch.ThrowException("Cannot set properties with the name postMessage"
                             "or postMessageAndAwaitResponse");
    return true;
  }

  // We don't forward this to the passthrough object; no plugins use that
  // feature.
  // TODO(raymes): Remove SetProperty support from PPP_Class.

  return false;
}

std::vector<std::string> MessageChannel::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::vector<std::string> result;
  PluginObject* plugin_object = GetPluginObject(isolate);
  if (plugin_object)
    result = plugin_object->EnumerateNamedProperties(isolate);
  result.push_back(kPostMessage);
  result.push_back(kPostMessageAndAwaitResponse);
  return result;
}

void MessageChannel::PostMessageToNative(gin::Arguments* args) {
  if (!instance_)
    return;
  if (args->Length() != 1) {
    // TODO(raymes): Consider throwing an exception here. We don't now for
    // backward compatibility.
    return;
  }

  v8::Handle<v8::Value> message_data;
  if (!args->GetNext(&message_data)) {
    NOTREACHED();
  }

  EnqueuePluginMessage(message_data);
  DrainCompletedPluginMessages();
}

void MessageChannel::PostBlockingMessageToNative(gin::Arguments* args) {
  if (!instance_)
    return;
  PepperTryCatchV8 try_catch(instance_, V8VarConverter::kDisallowObjectVars,
                             args->isolate());
  if (args->Length() != 1) {
    try_catch.ThrowException(
        "postMessageAndAwaitResponse requires one argument");
    return;
  }

  v8::Handle<v8::Value> message_data;
  if (!args->GetNext(&message_data)) {
    NOTREACHED();
  }

  if (early_message_queue_state_ == QUEUE_MESSAGES) {
    try_catch.ThrowException(
        "Attempted to call a synchronous method on a plugin that was not "
        "yet loaded.");
    return;
  }

  // If the queue of messages to the plugin is non-empty, we're still waiting on
  // pending Var conversions. This means at some point in the past, JavaScript
  // called postMessage (the async one) and passed us something with a browser-
  // side host (e.g., FileSystem) and we haven't gotten a response from the
  // browser yet. We can't currently support sending a sync message if the
  // plugin does this, because it will break the ordering of the messages
  // arriving at the plugin.
  // TODO(dmichael): Fix this.
  // See https://code.google.com/p/chromium/issues/detail?id=367896#c4
  if (!plugin_message_queue_.empty()) {
    try_catch.ThrowException(
        "Failed to convert parameter synchronously, because a prior "
        "call to postMessage contained a type which required asynchronous "
        "transfer which has not completed. Not all types are supported yet by "
        "postMessageAndAwaitResponse. See crbug.com/367896.");
    return;
  }
  ScopedPPVar param = try_catch.FromV8(message_data);
  if (try_catch.ThrowException())
    return;

  ScopedPPVar pp_result;
  bool was_handled = instance_->HandleBlockingMessage(param, &pp_result);
  if (!was_handled) {
    try_catch.ThrowException(
        "The plugin has not registered a handler for synchronous messages. "
        "See the documentation for PPB_Messaging::RegisterMessageHandler "
        "and PPP_MessageHandler.");
    return;
  }
  v8::Handle<v8::Value> v8_result = try_catch.ToV8(pp_result.get());
  if (try_catch.ThrowException())
    return;

  args->Return(v8_result);
}

void MessageChannel::PostMessageToJavaScriptImpl(
    const WebSerializedScriptValue& message_data) {
  DCHECK(instance_);

  WebPluginContainer* container = instance_->container();
  // It's possible that container() is NULL if the plugin has been removed from
  // the DOM (but the PluginInstance is not destroyed yet).
  if (!container)
    return;

  WebDOMEvent event =
      container->element().document().createEvent("MessageEvent");
  WebDOMMessageEvent msg_event = event.to<WebDOMMessageEvent>();
  msg_event.initMessageEvent("message",     // type
                             false,         // canBubble
                             false,         // cancelable
                             message_data,  // data
                             "",            // origin [*]
                             NULL,          // source [*]
                             "");           // lastEventId
  // [*] Note that the |origin| is only specified for cross-document and server-
  //     sent messages, while |source| is only specified for cross-document
  //     messages:
  //      http://www.whatwg.org/specs/web-apps/current-work/multipage/comms.html
  //     This currently behaves like Web Workers. On Firefox, Chrome, and Safari
  //     at least, postMessage on Workers does not provide the origin or source.
  //     TODO(dmichael):  Add origin if we change to a more iframe-like origin
  //                      policy (see crbug.com/81537)
  container->element().dispatchEvent(msg_event);
}

PluginObject* MessageChannel::GetPluginObject(v8::Isolate* isolate) {
  return PluginObject::FromV8Object(isolate,
      v8::Local<v8::Object>::New(isolate, passthrough_object_));
}

void MessageChannel::EnqueuePluginMessage(v8::Handle<v8::Value> v8_value) {
  plugin_message_queue_.push_back(VarConversionResult());
  // Convert NPVariantType_Object in to an appropriate PP_Var like Dictionary,
  // Array, etc. Note NPVariantToVar would convert to an "Object" PP_Var,
  // which we don't support for Messaging.
  // TODO(raymes): Possibly change this to use TryCatch to do the conversion and
  // throw an exception if necessary.
  V8VarConverter v8_var_converter(instance_->pp_instance());
  V8VarConverter::VarResult conversion_result =
      v8_var_converter.FromV8Value(
          v8_value,
          v8::Isolate::GetCurrent()->GetCurrentContext(),
          base::Bind(&MessageChannel::FromV8ValueComplete,
                     weak_ptr_factory_.GetWeakPtr(),
                     &plugin_message_queue_.back()));
  if (conversion_result.completed_synchronously) {
    plugin_message_queue_.back().ConversionCompleted(
        conversion_result.var,
        conversion_result.success);
  }
}

void MessageChannel::FromV8ValueComplete(VarConversionResult* result_holder,
                                         const ScopedPPVar& result,
                                         bool success) {
  if (!instance_)
    return;
  result_holder->ConversionCompleted(result, success);
  DrainCompletedPluginMessages();
}

void MessageChannel::DrainCompletedPluginMessages() {
  DCHECK(instance_);
  if (early_message_queue_state_ == QUEUE_MESSAGES)
    return;

  while (!plugin_message_queue_.empty() &&
         plugin_message_queue_.front().conversion_completed()) {
    const VarConversionResult& front = plugin_message_queue_.front();
    if (front.success()) {
      instance_->HandleMessage(front.var());
    } else {
      PpapiGlobals::Get()->LogWithSource(instance()->pp_instance(),
                                         PP_LOGLEVEL_ERROR,
                                         std::string(),
                                         kV8ToVarConversionError);
    }
    plugin_message_queue_.pop_front();
  }
}

void MessageChannel::DrainEarlyMessageQueue() {
  if (!instance_)
    return;
  DCHECK(early_message_queue_state_ == QUEUE_MESSAGES);

  // Take a reference on the PluginInstance. This is because JavaScript code
  // may delete the plugin, which would destroy the PluginInstance and its
  // corresponding MessageChannel.
  scoped_refptr<PepperPluginInstanceImpl> instance_ref(instance_);
  while (!early_message_queue_.empty()) {
    PostMessageToJavaScriptImpl(early_message_queue_.front());
    early_message_queue_.pop_front();
  }
  early_message_queue_state_ = SEND_DIRECTLY;

  DrainCompletedPluginMessages();
}

}  // namespace content
