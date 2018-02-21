/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.hadoop.hdfs.server.blockmanagement;

import org.apache.hadoop.classification.InterfaceAudience;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.StorageType;
import org.apache.hadoop.net.NetworkTopology;
import org.apache.hadoop.net.Node;

import java.util.*;

/**
 * The class is responsible for choosing the desired number of targets
 * for placing block replicas.
 * The strategy is that it tries its best to make a single ring.
 */
@InterfaceAudience.Private
public class BlockPlacementPolicyRDCN extends BlockPlacementPolicyDefault {

  @Override
  protected DatanodeStorageInfo chooseRandom(int numOfReplicas,
					     String scope,
					     Set<Node> excludedNodes,
					     long blocksize,
					     int maxNodesPerRack,
					     List<DatanodeStorageInfo> results,
					     boolean avoidStaleNodes,
					     EnumMap<StorageType, Integer> storageTypes)
    throws NotEnoughReplicasException {
      int numOfRacks = clusterMap.getNumOfRacks();
      if (scope.contains("~/rack")) {
	  int i = Integer.parseInt(scope.split("~/rack")[1]);
	  scope = String.format("/rack%02d", (i % numOfRacks) + 1);
      }
      return super.chooseRandom(numOfReplicas, scope, excludedNodes, blocksize,
				maxNodesPerRack, results, avoidStaleNodes,
				storageTypes);
  }
}
