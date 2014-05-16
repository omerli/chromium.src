// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_VIEW_MANAGER_VIEW_MANAGER_CONNECTION_H_
#define MOJO_SERVICES_VIEW_MANAGER_VIEW_MANAGER_CONNECTION_H_

#include <set>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#include "mojo/public/cpp/shell/service.h"
#include "mojo/services/public/interfaces/view_manager/view_manager.mojom.h"
#include "mojo/services/view_manager/ids.h"
#include "mojo/services/view_manager/node_delegate.h"
#include "mojo/services/view_manager/view_manager_export.h"

namespace mojo {
namespace view_manager {
namespace service {

class Node;
class RootNodeManager;
class View;

#if defined(OS_WIN)
// Equivalent of NON_EXPORTED_BASE which does not work with the template snafu
// below.
#pragma warning(push)
#pragma warning(disable : 4275)
#endif

// Manages a connection from the client.
class MOJO_VIEW_MANAGER_EXPORT ViewManagerConnection
    : public ServiceConnection<IViewManager, ViewManagerConnection,
                               RootNodeManager>,
      public NodeDelegate {
 public:
  ViewManagerConnection();
  virtual ~ViewManagerConnection();

  TransportConnectionId id() const { return id_; }

  // Invoked when connection is established.
  void Initialize();

  // Returns the Node with the specified id.
  Node* GetNode(const NodeId& id);

  // Returns the View with the specified id.
  View* GetView(const ViewId& id);

  // The following methods are invoked after the corresponding change has been
  // processed. They do the appropriate bookkeeping and update the client as
  // necessary.
  // TODO(sky): convert these to take Node*s.
  void ProcessNodeHierarchyChanged(const NodeId& node_id,
                                   const NodeId& new_parent_id,
                                   const NodeId& old_parent_id,
                                   TransportChangeId server_change_id,
                                   bool originated_change);
  void ProcessNodeViewReplaced(const NodeId& node,
                               const ViewId& new_view_id,
                               const ViewId& old_view_id,
                               bool originated_change);
  void ProcessNodeDeleted(const NodeId& node,
                          TransportChangeId server_change_id,
                          bool originated_change);
  void ProcessViewDeleted(const ViewId& view, bool originated_change);

 private:
  typedef std::map<TransportConnectionSpecificNodeId, Node*> NodeMap;
  typedef std::map<TransportConnectionSpecificViewId, View*> ViewMap;
  typedef base::hash_set<TransportNodeId> NodeIdSet;

  // Deletes a node owned by this connection. Returns true on success. |source|
  // is the connection that originated the change.
  bool DeleteNodeImpl(ViewManagerConnection* source, const NodeId& node_id);

  // Deletes a view owned by this connection. Returns true on success. |source|
  // is the connection that originated the change.
  bool DeleteViewImpl(ViewManagerConnection* source, const ViewId& view_id);

  // Sets the view associated with a node.
  bool SetViewImpl(const NodeId& node_id, const ViewId& view_id);

  // If |node| is known (in |known_nodes_|) does nothing. Otherwise adds |node|
  // to |nodes|, marks |node| as known and recurses.
  void GetUnknownNodesFrom(Node* node, std::vector<Node*>* nodes);

  // Returns true if notification should be sent of a hierarchy change. If true
  // is returned, any nodes that need to be sent to the client are added to
  // |to_send|.
  bool ShouldNotifyOnHierarchyChange(const NodeId& node_id,
                                     const NodeId& new_parent_id,
                                     const NodeId& old_parent_id,
                                     std::vector<Node*>* to_send);

  // Overridden from IViewManager:
  virtual void CreateNode(TransportNodeId transport_node_id,
                          const Callback<void(bool)>& callback) OVERRIDE;
  virtual void DeleteNode(TransportNodeId transport_node_id,
                          const Callback<void(bool)>& callback) OVERRIDE;
  virtual void AddNode(TransportNodeId parent_id,
                       TransportNodeId child_id,
                       TransportChangeId server_change_id,
                       const Callback<void(bool)>& callback) OVERRIDE;
  virtual void RemoveNodeFromParent(
      TransportNodeId node_id,
      TransportChangeId server_change_id,
      const Callback<void(bool)>& callback) OVERRIDE;
  virtual void GetNodeTree(
      TransportNodeId node_id,
      const Callback<void(Array<INode>)>& callback) OVERRIDE;
  virtual void CreateView(TransportViewId transport_view_id,
                          const Callback<void(bool)>& callback) OVERRIDE;
  virtual void DeleteView(TransportViewId transport_view_id,
                          const Callback<void(bool)>& callback) OVERRIDE;
  virtual void SetView(TransportNodeId transport_node_id,
                       TransportViewId transport_view_id,
                       const Callback<void(bool)>& callback) OVERRIDE;
  virtual void SetViewContents(TransportViewId view_id,
                               ScopedSharedBufferHandle buffer,
                               uint32_t buffer_size) OVERRIDE;

  // Overridden from NodeDelegate:
  virtual void OnNodeHierarchyChanged(const NodeId& node,
                                      const NodeId& new_parent,
                                      const NodeId& old_parent) OVERRIDE;
  virtual void OnNodeViewReplaced(const NodeId& node,
                                  const ViewId& new_view_id,
                                  const ViewId& old_view_id) OVERRIDE;

  // Id of this connection as assigned by RootNodeManager. Assigned in
  // Initialize().
  TransportConnectionId id_;

  NodeMap node_map_;

  ViewMap view_map_;

  // The set of nodes that has been communicated to the client.
  NodeIdSet known_nodes_;

  DISALLOW_COPY_AND_ASSIGN(ViewManagerConnection);
};

#if defined(OS_WIN)
#pragma warning(pop)
#endif

}  // namespace service
}  // namespace view_manager
}  // namespace mojo

#endif  // MOJO_SERVICES_VIEW_MANAGER_VIEW_MANAGER_CONNECTION_H_
