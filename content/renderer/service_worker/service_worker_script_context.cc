// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_script_context.h"

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "content/child/notifications/notification_data_conversions.h"
#include "content/child/thread_safe_sender.h"
#include "content/child/webmessageportchannel_impl.h"
#include "content/common/message_port_messages.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/public/common/referrer.h"
#include "content/renderer/service_worker/embedded_worker_context_client.h"
#include "ipc/ipc_message.h"
#include "third_party/WebKit/public/platform/WebCrossOriginServiceWorkerClient.h"
#include "third_party/WebKit/public/platform/WebNotificationData.h"
#include "third_party/WebKit/public/platform/WebReferrerPolicy.h"
#include "third_party/WebKit/public/platform/WebServiceWorkerRequest.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebServiceWorkerContextClient.h"
#include "third_party/WebKit/public/web/WebServiceWorkerContextProxy.h"

namespace content {

namespace {

void SendPostMessageToDocumentOnMainThread(
    ThreadSafeSender* sender,
    int routing_id,
    int client_id,
    const base::string16& message,
    scoped_ptr<blink::WebMessagePortChannelArray> channels) {
  sender->Send(new ServiceWorkerHostMsg_PostMessageToDocument(
      routing_id, client_id, message,
      WebMessagePortChannelImpl::ExtractMessagePortIDs(channels.release())));
}

void SendCrossOriginMessageToClientOnMainThread(
    ThreadSafeSender* sender,
    int message_port_id,
    const base::string16& message,
    scoped_ptr<blink::WebMessagePortChannelArray> channels) {
  sender->Send(new MessagePortHostMsg_PostMessage(
      message_port_id, message,
      WebMessagePortChannelImpl::ExtractMessagePortIDs(channels.release())));
}

blink::WebURLRequest::FetchRequestMode GetBlinkFetchRequestMode(
    FetchRequestMode mode) {
  return static_cast<blink::WebURLRequest::FetchRequestMode>(mode);
}

blink::WebURLRequest::FetchCredentialsMode GetBlinkFetchCredentialsMode(
    FetchCredentialsMode credentials_mode) {
  return static_cast<blink::WebURLRequest::FetchCredentialsMode>(
      credentials_mode);
}

blink::WebURLRequest::RequestContext GetBlinkRequestContext(
    RequestContextType request_context_type) {
  return static_cast<blink::WebURLRequest::RequestContext>(
      request_context_type);
}

blink::WebURLRequest::FrameType GetBlinkFrameType(
    RequestContextFrameType frame_type) {
  return static_cast<blink::WebURLRequest::FrameType>(frame_type);
}

}  // namespace

ServiceWorkerScriptContext::ServiceWorkerScriptContext(
    EmbeddedWorkerContextClient* embedded_context,
    blink::WebServiceWorkerContextProxy* proxy)
    : cache_storage_dispatcher_(new ServiceWorkerCacheStorageDispatcher(this)),
      embedded_context_(embedded_context),
      proxy_(proxy) {
}

ServiceWorkerScriptContext::~ServiceWorkerScriptContext() {}

void ServiceWorkerScriptContext::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ServiceWorkerScriptContext, message)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_ActivateEvent, OnActivateEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_FetchEvent, OnFetchEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_InstallEvent, OnInstallEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_SyncEvent, OnSyncEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_NotificationClickEvent,
                        OnNotificationClickEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_PushEvent, OnPushEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_GeofencingEvent, OnGeofencingEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_CrossOriginConnectEvent,
                        OnCrossOriginConnectEvent)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_MessageToWorker, OnPostMessage)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_CrossOriginMessageToWorker,
                        OnCrossOriginMessageToWorker)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_DidGetClientDocuments,
                        OnDidGetClientDocuments)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_FocusClientResponse,
                        OnFocusClientResponse)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_DidSkipWaiting, OnDidSkipWaiting)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  // TODO(gavinp): Would it be preferable to put an AddListener() method to
  // EmbeddedWorkerContextClient?
  if (!handled)
    handled = cache_storage_dispatcher_->OnMessageReceived(message);

  DCHECK(handled);
}

void ServiceWorkerScriptContext::DidHandleActivateEvent(
    int request_id,
    blink::WebServiceWorkerEventResult result) {
  UMA_HISTOGRAM_TIMES(
      "ServiceWorker.ActivateEventExecutionTime",
      base::TimeTicks::Now() - activate_start_timings_[request_id]);
  activate_start_timings_.erase(request_id);

  Send(new ServiceWorkerHostMsg_ActivateEventFinished(
      GetRoutingID(), request_id, result));
}

void ServiceWorkerScriptContext::DidHandleInstallEvent(
    int request_id,
    blink::WebServiceWorkerEventResult result) {
  UMA_HISTOGRAM_TIMES(
      "ServiceWorker.InstallEventExecutionTime",
      base::TimeTicks::Now() - install_start_timings_[request_id]);
  install_start_timings_.erase(request_id);

  Send(new ServiceWorkerHostMsg_InstallEventFinished(
      GetRoutingID(), request_id, result));
}

void ServiceWorkerScriptContext::DidHandleFetchEvent(
    int request_id,
    ServiceWorkerFetchEventResult result,
    const ServiceWorkerResponse& response) {
  UMA_HISTOGRAM_TIMES(
      "ServiceWorker.FetchEventExecutionTime",
      base::TimeTicks::Now() - fetch_start_timings_[request_id]);
  fetch_start_timings_.erase(request_id);

  Send(new ServiceWorkerHostMsg_FetchEventFinished(
      GetRoutingID(), request_id, result, response));
}

void ServiceWorkerScriptContext::DidHandleNotificationClickEvent(
    int request_id,
    blink::WebServiceWorkerEventResult result) {
  UMA_HISTOGRAM_TIMES(
      "ServiceWorker.NotificationClickEventExecutionTime",
      base::TimeTicks::Now() - notification_click_start_timings_[request_id]);
  notification_click_start_timings_.erase(request_id);

  Send(new ServiceWorkerHostMsg_NotificationClickEventFinished(
      GetRoutingID(), request_id));
}

void ServiceWorkerScriptContext::DidHandlePushEvent(
    int request_id,
    blink::WebServiceWorkerEventResult result) {
  if (result == blink::WebServiceWorkerEventResultCompleted) {
    UMA_HISTOGRAM_TIMES(
        "ServiceWorker.PushEventExecutionTime",
        base::TimeTicks::Now() - push_start_timings_[request_id]);
  }
  push_start_timings_.erase(request_id);

  Send(new ServiceWorkerHostMsg_PushEventFinished(
      GetRoutingID(), request_id, result));
}

void ServiceWorkerScriptContext::DidHandleSyncEvent(int request_id) {
  Send(new ServiceWorkerHostMsg_SyncEventFinished(
      GetRoutingID(), request_id));
}

void ServiceWorkerScriptContext::DidHandleCrossOriginConnectEvent(
    int request_id,
    bool accept_connection) {
  Send(new ServiceWorkerHostMsg_CrossOriginConnectEventFinished(
      GetRoutingID(), request_id, accept_connection));
}

void ServiceWorkerScriptContext::GetClientDocuments(
    blink::WebServiceWorkerClientsCallbacks* callbacks) {
  DCHECK(callbacks);
  int request_id = pending_clients_callbacks_.Add(callbacks);
  Send(new ServiceWorkerHostMsg_GetClientDocuments(
      GetRoutingID(), request_id));
}

void ServiceWorkerScriptContext::PostMessageToDocument(
    int client_id,
    const base::string16& message,
    scoped_ptr<blink::WebMessagePortChannelArray> channels) {
  // This may send channels for MessagePorts, and all internal book-keeping
  // messages for MessagePort (e.g. QueueMessages) are sent from main thread
  // (with thread hopping), so we need to do the same thread hopping here not
  // to overtake those messages.
  embedded_context_->main_thread_proxy()->PostTask(
      FROM_HERE,
      base::Bind(&SendPostMessageToDocumentOnMainThread,
                 make_scoped_refptr(embedded_context_->thread_safe_sender()),
                 GetRoutingID(), client_id, message, base::Passed(&channels)));
}

void ServiceWorkerScriptContext::PostCrossOriginMessageToClient(
    const blink::WebCrossOriginServiceWorkerClient& client,
    const base::string16& message,
    scoped_ptr<blink::WebMessagePortChannelArray> channels) {
  // This may send channels for MessagePorts, and all internal book-keeping
  // messages for MessagePort (e.g. QueueMessages) are sent from main thread
  // (with thread hopping), so we need to do the same thread hopping here not
  // to overtake those messages.
  embedded_context_->main_thread_proxy()->PostTask(
      FROM_HERE,
      base::Bind(&SendCrossOriginMessageToClientOnMainThread,
                 make_scoped_refptr(embedded_context_->thread_safe_sender()),
                 client.clientID, message, base::Passed(&channels)));
}

void ServiceWorkerScriptContext::FocusClient(
    int client_id, blink::WebServiceWorkerClientFocusCallback* callback) {
  DCHECK(callback);
  int request_id = pending_focus_client_callbacks_.Add(callback);
  Send(new ServiceWorkerHostMsg_FocusClient(
      GetRoutingID(), request_id, client_id));
}

void ServiceWorkerScriptContext::SkipWaiting(
    blink::WebServiceWorkerSkipWaitingCallbacks* callbacks) {
  DCHECK(callbacks);
  int request_id = pending_skip_waiting_callbacks_.Add(callbacks);
  Send(new ServiceWorkerHostMsg_SkipWaiting(GetRoutingID(), request_id));
}

void ServiceWorkerScriptContext::Send(IPC::Message* message) {
  embedded_context_->Send(message);
}

int ServiceWorkerScriptContext::GetRoutingID() const {
  return embedded_context_->embedded_worker_id();
}

void ServiceWorkerScriptContext::OnActivateEvent(int request_id) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnActivateEvent");
  activate_start_timings_[request_id] = base::TimeTicks::Now();
  proxy_->dispatchActivateEvent(request_id);
}

void ServiceWorkerScriptContext::OnInstallEvent(int request_id,
                                                int active_version_id) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnInstallEvent");
  install_start_timings_[request_id] = base::TimeTicks::Now();
  proxy_->dispatchInstallEvent(request_id);
}

void ServiceWorkerScriptContext::OnFetchEvent(
    int request_id,
    const ServiceWorkerFetchRequest& request) {
  blink::WebServiceWorkerRequest webRequest;
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnFetchEvent");
  webRequest.setURL(blink::WebURL(request.url));
  webRequest.setMethod(blink::WebString::fromUTF8(request.method));
  for (ServiceWorkerHeaderMap::const_iterator it = request.headers.begin();
       it != request.headers.end();
       ++it) {
    webRequest.setHeader(blink::WebString::fromUTF8(it->first),
                         blink::WebString::fromUTF8(it->second));
  }
  if (!request.blob_uuid.empty()) {
    webRequest.setBlob(blink::WebString::fromUTF8(request.blob_uuid),
                       request.blob_size);
  }
  webRequest.setReferrer(
      blink::WebString::fromUTF8(request.referrer.url.spec()),
      request.referrer.policy);
  webRequest.setMode(GetBlinkFetchRequestMode(request.mode));
  webRequest.setCredentialsMode(
      GetBlinkFetchCredentialsMode(request.credentials_mode));
  webRequest.setRequestContext(
      GetBlinkRequestContext(request.request_context_type));
  webRequest.setFrameType(GetBlinkFrameType(request.frame_type));
  webRequest.setIsReload(request.is_reload);
  fetch_start_timings_[request_id] = base::TimeTicks::Now();
  proxy_->dispatchFetchEvent(request_id, webRequest);
}

void ServiceWorkerScriptContext::OnSyncEvent(int request_id) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnSyncEvent");
  proxy_->dispatchSyncEvent(request_id);
}

void ServiceWorkerScriptContext::OnNotificationClickEvent(
    int request_id,
    const std::string& notification_id,
    const PlatformNotificationData& notification_data) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnNotificationClickEvent");
  notification_click_start_timings_[request_id] = base::TimeTicks::Now();
  proxy_->dispatchNotificationClickEvent(
      request_id,
      blink::WebString::fromUTF8(notification_id),
      ToWebNotificationData(notification_data));
}

void ServiceWorkerScriptContext::OnPushEvent(int request_id,
                                             const std::string& data) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnPushEvent");
  push_start_timings_[request_id] = base::TimeTicks::Now();
  proxy_->dispatchPushEvent(request_id, blink::WebString::fromUTF8(data));
}

void ServiceWorkerScriptContext::OnGeofencingEvent(
    int request_id,
    blink::WebGeofencingEventType event_type,
    const std::string& region_id,
    const blink::WebCircularGeofencingRegion& region) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnGeofencingEvent");
  proxy_->dispatchGeofencingEvent(
      request_id, event_type, blink::WebString::fromUTF8(region_id), region);
  Send(new ServiceWorkerHostMsg_GeofencingEventFinished(GetRoutingID(),
                                                        request_id));
}

void ServiceWorkerScriptContext::OnCrossOriginConnectEvent(
    int request_id,
    const CrossOriginServiceWorkerClient& client) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnCrossOriginConnectEvent");
  blink::WebCrossOriginServiceWorkerClient web_client;
  web_client.origin = client.origin;
  web_client.targetURL = client.target_url;
  web_client.clientID = client.message_port_id;
  proxy_->dispatchCrossOriginConnectEvent(request_id, web_client);
}

void ServiceWorkerScriptContext::OnPostMessage(
    const base::string16& message,
    const std::vector<int>& sent_message_port_ids,
    const std::vector<int>& new_routing_ids) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnPostEvent");
  std::vector<WebMessagePortChannelImpl*> ports;
  if (!sent_message_port_ids.empty()) {
    base::MessageLoopProxy* loop_proxy = embedded_context_->main_thread_proxy();
    ports.resize(sent_message_port_ids.size());
    for (size_t i = 0; i < sent_message_port_ids.size(); ++i) {
      ports[i] = new WebMessagePortChannelImpl(
          new_routing_ids[i], sent_message_port_ids[i], loop_proxy);
    }
  }

  // dispatchMessageEvent is expected to execute onmessage function
  // synchronously.
  base::TimeTicks before = base::TimeTicks::Now();
  proxy_->dispatchMessageEvent(message, ports);
  UMA_HISTOGRAM_TIMES(
      "ServiceWorker.MessageEventExecutionTime",
      base::TimeTicks::Now() - before);
}

void ServiceWorkerScriptContext::OnCrossOriginMessageToWorker(
    const CrossOriginServiceWorkerClient& client,
    const base::string16& message,
    const std::vector<int>& sent_message_port_ids,
    const std::vector<int>& new_routing_ids) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnCrossOriginMessageToWorker");
  std::vector<WebMessagePortChannelImpl*> ports;
  if (!sent_message_port_ids.empty()) {
    base::MessageLoopProxy* loop_proxy = embedded_context_->main_thread_proxy();
    ports.resize(sent_message_port_ids.size());
    for (size_t i = 0; i < sent_message_port_ids.size(); ++i) {
      ports[i] = new WebMessagePortChannelImpl(
          new_routing_ids[i], sent_message_port_ids[i], loop_proxy);
    }
  }

  blink::WebCrossOriginServiceWorkerClient web_client;
  web_client.origin = client.origin;
  web_client.targetURL = client.target_url;
  web_client.clientID = client.message_port_id;
  proxy_->dispatchCrossOriginMessageEvent(web_client, message, ports);
}

void ServiceWorkerScriptContext::OnDidGetClientDocuments(
    int request_id, const std::vector<ServiceWorkerClientInfo>& clients) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnDidGetClientDocuments");
  blink::WebServiceWorkerClientsCallbacks* callbacks =
      pending_clients_callbacks_.Lookup(request_id);
  if (!callbacks) {
    NOTREACHED() << "Got stray response: " << request_id;
    return;
  }
  scoped_ptr<blink::WebServiceWorkerClientsInfo> info(
      new blink::WebServiceWorkerClientsInfo);
  blink::WebVector<blink::WebServiceWorkerClientInfo> convertedClients(
      clients.size());
  for (size_t i = 0; i < clients.size(); ++i) {
    convertedClients[i].clientID = clients[i].client_id;
    convertedClients[i].visibilityState =
        blink::WebString::fromUTF8(clients[i].visibility_state);
    convertedClients[i].isFocused = clients[i].is_focused;
    convertedClients[i].url = clients[i].url;
    convertedClients[i].frameType =
        static_cast<blink::WebURLRequest::FrameType>(clients[i].frame_type);
  }
  info->clients.swap(convertedClients);
  callbacks->onSuccess(info.release());
  pending_clients_callbacks_.Remove(request_id);
}

void ServiceWorkerScriptContext::OnFocusClientResponse(int request_id,
                                                       bool result) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnFocusClientResponse");
  blink::WebServiceWorkerClientFocusCallback* callback =
      pending_focus_client_callbacks_.Lookup(request_id);
  if (!callback) {
    NOTREACHED() << "Got stray response: " << request_id;
    return;
  }
  callback->onSuccess(&result);
  pending_focus_client_callbacks_.Remove(request_id);
}

void ServiceWorkerScriptContext::OnDidSkipWaiting(int request_id) {
  TRACE_EVENT0("ServiceWorker",
               "ServiceWorkerScriptContext::OnDidSkipWaiting");
  blink::WebServiceWorkerSkipWaitingCallbacks* callbacks =
      pending_skip_waiting_callbacks_.Lookup(request_id);
  if (!callbacks) {
    NOTREACHED() << "Got stray response: " << request_id;
    return;
  }
  callbacks->onSuccess();
  pending_skip_waiting_callbacks_.Remove(request_id);
}

}  // namespace content
