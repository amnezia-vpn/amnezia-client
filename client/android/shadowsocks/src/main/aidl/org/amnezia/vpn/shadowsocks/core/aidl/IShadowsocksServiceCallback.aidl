package org.amnezia.vpn.shadowsocks.core.aidl;

import org.amnezia.vpn.shadowsocks.core.aidl.TrafficStats;

//"oneway" unexpected. xinlake
interface IShadowsocksServiceCallback {
  oneway void stateChanged(int state, String profileName, String msg);
  oneway void trafficUpdated(long profileId, in TrafficStats stats);
  // Traffic data has persisted to database, listener should refetch their data from database
  oneway void trafficPersisted(long profileId);
}

//oneway interface IShadowsocksServiceCallback {
//  void stateChanged(int state, String profileName, String msg);
//  void trafficUpdated(long profileId, in TrafficStats stats);
//  // Traffic data has persisted to database, listener should refetch their data from database
//  void trafficPersisted(long profileId);
//}
