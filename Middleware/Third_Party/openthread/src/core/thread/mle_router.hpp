/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes definitions for MLE functionality required by the Thread Router and Leader roles.
 */

#ifndef MLE_ROUTER_HPP_
#define MLE_ROUTER_HPP_

#include "openthread-core-config.h"

#include <openthread/thread_ftd.h>

#include "coap/coap_message.hpp"
#include "common/callback.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "common/trickle_timer.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/icmp6.hpp"
#include "net/udp6.hpp"
#include "thread/child.hpp"
#include "thread/child_table.hpp"
#include "thread/mle.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/router.hpp"
#include "thread/router_table.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Mle {

/**
 * @addtogroup core-mle-router
 *
 * @brief
 *   This module includes definitions for MLE functionality required by the Thread Router and Leader roles.
 *
 * @{
 */

#if OPENTHREAD_FTD

/**
 * Implements MLE functionality required by the Thread Router and Leader roles.
 */
class MleRouter : public Mle
{
    friend class Mle;
    friend class ot::Instance;
    friend class ot::TimeTicker;
    friend class Tmf::Agent;

public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit MleRouter(Instance &aInstance);

    /**
     * Indicates whether or not the device is router-eligible.
     *
     * @retval true   If device is router-eligible.
     * @retval false  If device is not router-eligible.
     */
    bool IsRouterEligible(void) const;

    /**
     * Sets whether or not the device is router-eligible.
     *
     * If @p aEligible is false and the device is currently operating as a router, this call will cause the device to
     * detach and attempt to reattach as a child.
     *
     * @param[in]  aEligible  TRUE to configure device router-eligible, FALSE otherwise.
     *
     * @retval kErrorNone         Successfully set the router-eligible configuration.
     * @retval kErrorNotCapable   The device is not capable of becoming a router.
     */
    Error SetRouterEligible(bool aEligible);

    /**
     * Indicates whether a node is the only router on the network.
     *
     * @retval TRUE   It is the only router in the network.
     * @retval FALSE  It is a child or is not a single router in the network.
     */
    bool IsSingleton(void) const;

    /**
     * Generates an Address Solicit request for a Router ID.
     *
     * @param[in]  aStatus  The reason for requesting a Router ID.
     *
     * @retval kErrorNone           Successfully generated an Address Solicit message.
     * @retval kErrorNotCapable     Device is not capable of becoming a router
     * @retval kErrorInvalidState   Thread is not enabled
     */
    Error BecomeRouter(ThreadStatusTlv::Status aStatus);

    /**
     * Becomes a leader and starts a new partition.
     *
     * If the device is already attached, this method can be used to attempt to take over as the leader, creating a new
     * partition. For this to work, the local leader weight must be greater than the weight of the current leader. The
     * @p aCheckWeight can be used to ensure that this check is performed.
     *
     * @param[in] aCheckWeight      Check that the local leader weight is larger than the weight of the current leader.
     *
     * @retval kErrorNone           Successfully become a Leader and started a new partition.
     * @retval kErrorInvalidState   Thread is not enabled.
     * @retval kErrorNotCapable     Device is not capable of becoming a leader (not router eligible), or
     *                              @p aCheckWeight is true and cannot override the current leader due to its local
     *                              leader weight being same or smaller than current leader's weight.
     */
    Error BecomeLeader(bool aCheckWeight);

#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
    /**
     * Gets the device properties which are used to determine the Leader Weight.
     *
     * @returns The current device properties.
     */
    const DeviceProperties &GetDeviceProperties(void) const { return mDeviceProperties; }

    /**
     * Sets the device properties which are then used to determine and set the Leader Weight.
     *
     * @param[in]  aDeviceProperties    The device properties.
     */
    void SetDeviceProperties(const DeviceProperties &aDeviceProperties);
#endif

    /**
     * Returns the Leader Weighting value for this Thread interface.
     *
     * @returns The Leader Weighting value for this Thread interface.
     */
    uint8_t GetLeaderWeight(void) const { return mLeaderWeight; }

    /**
     * Sets the Leader Weighting value for this Thread interface.
     *
     * Directly sets the Leader Weight to the new value replacing its previous value (which may have been
     * determined from a previous call to `SetDeviceProperties()`).
     *
     * @param[in]  aWeight  The Leader Weighting value.
     */
    void SetLeaderWeight(uint8_t aWeight) { mLeaderWeight = aWeight; }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

    /**
     * Returns the preferred Partition Id when operating in the Leader role for certification testing.
     *
     * @returns The preferred Partition Id value.
     */
    uint32_t GetPreferredLeaderPartitionId(void) const { return mPreferredLeaderPartitionId; }

    /**
     * Sets the preferred Partition Id when operating in the Leader role for certification testing.
     *
     * @param[in]  aPartitionId  The preferred Leader Partition Id.
     */
    void SetPreferredLeaderPartitionId(uint32_t aPartitionId) { mPreferredLeaderPartitionId = aPartitionId; }
#endif

    /**
     * Sets the preferred Router Id. Upon becoming a router/leader the node
     * attempts to use this Router Id. If the preferred Router Id is not set or if it
     * can not be used, a randomly generated router Id is picked.
     * This property can be set when he device role is detached or disabled.
     *
     * @param[in]  aRouterId             The preferred Router Id.
     *
     * @retval kErrorNone          Successfully set the preferred Router Id.
     * @retval kErrorInvalidState  Could not set (role is other than detached and disabled)
     */
    Error SetPreferredRouterId(uint8_t aRouterId);

    /**
     * Gets the Partition Id which the device joined successfully once.
     */
    uint32_t GetPreviousPartitionId(void) const { return mPreviousPartitionId; }

    /**
     * Sets the Partition Id which the device joins successfully.
     *
     * @param[in]  aPartitionId   The Partition Id.
     */
    void SetPreviousPartitionId(uint32_t aPartitionId) { mPreviousPartitionId = aPartitionId; }

    /**
     * Sets the Router Id.
     *
     * @param[in]  aRouterId   The Router Id.
     */
    void SetRouterId(uint8_t aRouterId);

    /**
     * Returns the NETWORK_ID_TIMEOUT value.
     *
     * @returns The NETWORK_ID_TIMEOUT value.
     */
    uint8_t GetNetworkIdTimeout(void) const { return mNetworkIdTimeout; }

    /**
     * Sets the NETWORK_ID_TIMEOUT value.
     *
     * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
     */
    void SetNetworkIdTimeout(uint8_t aTimeout) { mNetworkIdTimeout = aTimeout; }

    /**
     * Returns the ROUTER_SELECTION_JITTER value.
     *
     * @returns The ROUTER_SELECTION_JITTER value in seconds.
     */
    uint8_t GetRouterSelectionJitter(void) const { return mRouterRoleTransition.GetJitter(); }

    /**
     * Sets the ROUTER_SELECTION_JITTER value.
     *
     * @param[in] aRouterJitter  The router selection jitter value (in seconds).
     */
    void SetRouterSelectionJitter(uint8_t aRouterJitter) { mRouterRoleTransition.SetJitter(aRouterJitter); }

    /**
     * Indicates whether or not router role transition (upgrade from REED or downgrade to REED) is pending.
     *
     * @retval TRUE    Router role transition is pending.
     * @retval FALSE   Router role transition is not pending
     */
    bool IsRouterRoleTransitionPending(void) const { return mRouterRoleTransition.IsPending(); }

    /**
     * Returns the current timeout delay in seconds till router role transition (upgrade from REED or downgrade to
     * REED).
     *
     * @returns The timeout in seconds till router role transition, or zero if not pending role transition.
     */
    uint8_t GetRouterRoleTransitionTimeout(void) const { return mRouterRoleTransition.GetTimeout(); }

    /**
     * Returns the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_UPGRADE_THRESHOLD value.
     */
    uint8_t GetRouterUpgradeThreshold(void) const { return mRouterUpgradeThreshold; }

    /**
     * Sets the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @param[in]  aThreshold  The ROUTER_UPGRADE_THRESHOLD value.
     */
    void SetRouterUpgradeThreshold(uint8_t aThreshold) { mRouterUpgradeThreshold = aThreshold; }

    /**
     * Returns the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
     */
    uint8_t GetRouterDowngradeThreshold(void) const { return mRouterDowngradeThreshold; }

    /**
     * Sets the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @param[in]  aThreshold  The ROUTER_DOWNGRADE_THRESHOLD value.
     */
    void SetRouterDowngradeThreshold(uint8_t aThreshold) { mRouterDowngradeThreshold = aThreshold; }

    /**
     * Returns the MLE_CHILD_ROUTER_LINKS value.
     *
     * @returns The MLE_CHILD_ROUTER_LINKS value.
     */
    uint8_t GetChildRouterLinks(void) const { return mChildRouterLinks; }

    /**
     * Sets the MLE_CHILD_ROUTER_LINKS value.
     *
     * @param[in]  aChildRouterLinks  The MLE_CHILD_ROUTER_LINKS value.
     *
     * @retval kErrorNone          Successfully set the value.
     * @retval kErrorInvalidState  Thread protocols are enabled.
     */
    Error SetChildRouterLinks(uint8_t aChildRouterLinks);

    /**
     * Returns if the REED is expected to become Router soon.
     *
     * @retval TRUE   If the REED is going to become a Router soon.
     * @retval FALSE  If the REED is not going to become a Router soon.
     */
    bool IsExpectedToBecomeRouterSoon(void) const;

    /**
     * Removes a link to a neighbor.
     *
     * @param[in]  aNeighbor  A reference to the neighbor object.
     */
    void RemoveNeighbor(Neighbor &aNeighbor);

    /**
     * Invalidates a direct link to a neighboring router (due to failed link-layer acks).
     *
     * @param[in]  aRouter  A reference to the router object.
     */
    void RemoveRouterLink(Router &aRouter);

    /**
     * Indicates whether or not the given Thread partition attributes are preferred.
     *
     * @param[in]  aSingletonA   Whether or not the Thread Partition A has a single router.
     * @param[in]  aLeaderDataA  A reference to Thread Partition A's Leader Data.
     * @param[in]  aSingletonB   Whether or not the Thread Partition B has a single router.
     * @param[in]  aLeaderDataB  A reference to Thread Partition B's Leader Data.
     *
     * @retval 1   If partition A is preferred.
     * @retval 0   If partition A and B have equal preference.
     * @retval -1  If partition B is preferred.
     */
    static int ComparePartitions(bool              aSingletonA,
                                 const LeaderData &aLeaderDataA,
                                 bool              aSingletonB,
                                 const LeaderData &aLeaderDataB);

    /**
     * Fills an ConnectivityTlv.
     *
     * @param[out]  aTlv  A reference to the tlv to be filled.
     */
    void FillConnectivityTlv(ConnectivityTlv &aTlv);

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    /**
     * Sets steering data out of band
     *
     * @param[in]  aExtAddress  Value used to set steering data
     *                          All zeros clears steering data
     *                          All 0xFFs sets steering data to 0xFF
     *                          Anything else is used to compute the bloom filter
     */
    void SetSteeringData(const Mac::ExtAddress *aExtAddress);
#endif

    /**
     * Gets the assigned parent priority.
     *
     * @returns The assigned parent priority value, -2 means not assigned.
     */
    int8_t GetAssignParentPriority(void) const { return mParentPriority; }

    /**
     * Sets the parent priority.
     *
     * @param[in]  aParentPriority  The parent priority value.
     *
     * @retval kErrorNone           Successfully set the parent priority.
     * @retval kErrorInvalidArgs    If the parent priority value is not among 1, 0, -1 and -2.
     */
    Error SetAssignParentPriority(int8_t aParentPriority);

    /**
     * Gets the longest MLE Timeout TLV for all active MTD children.
     *
     * @param[out]  aTimeout  A reference to where the information is placed.
     *
     * @retval kErrorNone           Successfully get the max child timeout
     * @retval kErrorInvalidState   Not an active router
     * @retval kErrorNotFound       NO MTD child
     */
    Error GetMaxChildTimeout(uint32_t &aTimeout) const;

    /**
     * Sets the callback that is called when processing an MLE Discovery Request message.
     *
     * @param[in]  aCallback A pointer to a function that is called to deliver MLE Discovery Request data.
     * @param[in]  aContext  A pointer to application-specific context.
     */
    void SetDiscoveryRequestCallback(otThreadDiscoveryRequestCallback aCallback, void *aContext)
    {
        mDiscoveryRequestCallback.Set(aCallback, aContext);
    }

    /**
     * Resets the MLE Advertisement Trickle timer interval.
     */
    void ResetAdvertiseInterval(void);

    /**
     * Updates the MLE Advertisement Trickle timer max interval (if timer is running).
     *
     * This is called when there is change in router table.
     */
    void UpdateAdvertiseInterval(void);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Generates an MLE Time Synchronization message.
     *
     * @retval kErrorNone     Successfully sent an MLE Time Synchronization message.
     * @retval kErrorNoBufs   Insufficient buffers to generate the MLE Time Synchronization message.
     */
    Error SendTimeSync(void);
#endif

    /**
     * Gets the maximum number of IP addresses that each MTD child may register with this device as parent.
     *
     * @returns The maximum number of IP addresses that each MTD child may register with this device as parent.
     */
    uint8_t GetMaxChildIpAddresses(void) const;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

    /**
     * Sets/restores the maximum number of IP addresses that each MTD child may register with this
     * device as parent.
     *
     * @param[in]  aMaxIpAddresses  The maximum number of IP addresses that each MTD child may register with this
     *                              device as parent. 0 to clear the setting and restore the default.
     *
     * @retval kErrorNone           Successfully set/cleared the number.
     * @retval kErrorInvalidArgs    If exceeds the allowed maximum number.
     */
    Error SetMaxChildIpAddresses(uint8_t aMaxIpAddresses);

    /**
     * Sets whether the device was commissioned using CCM.
     *
     * @param[in]  aEnabled  TRUE if the device was commissioned using CCM, FALSE otherwise.
     */
    void SetCcmEnabled(bool aEnabled) { mCcmEnabled = aEnabled; }

    /**
     * Sets whether the Security Policy TLV version-threshold for routing (VR field) is enabled.
     *
     * @param[in]  aEnabled  TRUE to enable Security Policy TLV version-threshold for routing, FALSE otherwise.
     */
    void SetThreadVersionCheckEnabled(bool aEnabled) { mThreadVersionCheckEnabled = aEnabled; }

    /**
     * Gets the current Interval Max value used by Advertisement trickle timer.
     *
     * @returns The Interval Max of Advertisement trickle timer in milliseconds.
     */
    uint32_t GetAdvertisementTrickleIntervalMax(void) const { return mAdvertiseTrickleTimer.GetIntervalMax(); }

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

private:
    //------------------------------------------------------------------------------------------------------------------
    // Constants

    // Advertisement trickle timer constants - all times are in milliseconds.
    static constexpr uint32_t kAdvIntervalMin                = 1000;  // I_MIN
    static constexpr uint32_t kAdvIntervalNeighborMultiplier = 4000;  // Multiplier for I_MAX per router neighbor
    static constexpr uint32_t kAdvIntervalMaxLowerBound      = 12000; // Lower bound for I_MAX
    static constexpr uint32_t kAdvIntervalMaxUpperBound      = 32000; // Upper bound for I_MAX
    static constexpr uint32_t kReedAdvIntervalMin            = 570000;
    static constexpr uint32_t kReedAdvIntervalMax            = 630000;
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    static constexpr uint32_t kAdvIntervalMaxLogRoutes = 5000;
#endif

    static constexpr uint32_t kMaxNeighborAge                = 300000; // Max neighbor age (in msec)
    static constexpr uint32_t kMaxLeaderToRouterTimeout      = 90000;  // (in msec)
    static constexpr uint8_t  kMinDowngradeNeighbors         = 4; //7
    static constexpr uint8_t  kNetworkIdTimeout              = 255; //120 // (in sec)
    static constexpr uint8_t  kRouterSelectionJitter         = 255; //120 // (in sec)
    static constexpr uint8_t  kRouterDowngradeThreshold      = 24;
    static constexpr uint8_t  kRouterUpgradeThreshold        = 16;
    static constexpr uint16_t kDiscoveryMaxJitter            = 250; // Max jitter delay Discovery Responses (in msec).
    static constexpr uint16_t kUnsolicitedDataResponseJitter = 500; // Max delay for unsol Data Response (in msec).
    static constexpr uint8_t  kLeaderDowngradeExtraDelay     = 10;  // Extra delay to downgrade leader (in sec).
    static constexpr uint8_t  kDefaultLeaderWeight           = 64;
    static constexpr uint8_t  kAlternateRloc16Timeout        = 8; // Time to use alternate RLOC16 (in sec).

    // Threshold to accept a router upgrade request with reason
    // `kBorderRouterRequest` (number of BRs acting as router in
    // Network Data).
    static constexpr uint8_t kRouterUpgradeBorderRouterRequestThreshold = 2;

    static constexpr uint8_t kLinkRequestMinMargin    = OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN;
    static constexpr uint8_t kPartitionMergeMinMargin = OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN;
    static constexpr uint8_t kChildRouterLinks        = OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS;
    static constexpr uint8_t kMaxChildIpAddresses     = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD;

    static constexpr uint8_t kMinCriticalChildrenCount = 6;

    static constexpr uint16_t kChildSupervisionDefaultIntervalForOlderVersion =
        OPENTHREAD_CONFIG_CHILD_SUPERVISION_OLDER_VERSION_CHILD_DEFAULT_INTERVAL;

    static constexpr int8_t kParentPriorityHigh        = 1;
    static constexpr int8_t kParentPriorityMedium      = 0;
    static constexpr int8_t kParentPriorityLow         = -1;
    static constexpr int8_t kParentPriorityUnspecified = -2;

    //------------------------------------------------------------------------------------------------------------------
    // Nested types

    class RouterRoleTransition
    {
    public:
        RouterRoleTransition(void);

        bool    IsPending(void) const { return (mTimeout != 0); }
        void    StartTimeout(void);
        void    StopTimeout(void) { mTimeout = 0; }
        void    IncreaseTimeout(uint8_t aIncrement) { mTimeout += aIncrement; }
        uint8_t GetTimeout(void) const { return mTimeout; }
        bool    HandleTimeTick(void);
        uint8_t GetJitter(void) const { return mJitter; }
        void    SetJitter(uint8_t aJitter) { mJitter = aJitter; }

    private:
        uint8_t mTimeout;
        uint8_t mJitter;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class RouterRoleRestorer : public InstanceLocator
    {
        // Attempts to restore the router or leader role after an MLE
        // restart(e.g., after a device reboot) by sending multicast
        // Link Requests.

    public:
        RouterRoleRestorer(Instance &aInstance);

        bool IsActive(void) const { return mAttempts > 0; }
        void Start(DeviceRole aPreviousRole);
        void Stop(void) { mAttempts = 0; }
        void HandleTimer(void);

        void               GenerateRandomChallenge(void) { mChallenge.GenerateRandom(); }
        const TxChallenge &GetChallenge(void) const { return mChallenge; }

    private:
        void SendMulticastLinkRequest(void);

        uint8_t     mAttempts;
        TxChallenge mChallenge;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Methods

    void     SetAlternateRloc16(uint16_t aRloc16);
    void     ClearAlternateRloc16(void);
    void     HandleDetachStart(void);
    void     HandleChildStart(AttachMode aMode);
    void     HandleSecurityPolicyChanged(void);
    void     HandleLinkRequest(RxInfo &aRxInfo);
    void     HandleLinkAccept(RxInfo &aRxInfo);
    void     HandleLinkAcceptAndRequest(RxInfo &aRxInfo);
    void     HandleLinkAcceptVariant(RxInfo &aRxInfo, MessageType aMessageType);
    Error    HandleAdvertisementOnFtd(RxInfo &aRxInfo, uint16_t aSourceAddress, const LeaderData &aLeaderData);
    void     HandleParentRequest(RxInfo &aRxInfo);
    void     HandleChildIdRequest(RxInfo &aRxInfo);
    void     HandleChildUpdateRequestOnParent(RxInfo &aRxInfo);
    void     HandleChildUpdateResponseOnParent(RxInfo &aRxInfo);
    void     HandleDataRequest(RxInfo &aRxInfo);
    void     HandleNetworkDataUpdateRouter(void);
    void     HandleDiscoveryRequest(RxInfo &aRxInfo);
    Error    ProcessRouteTlv(const RouteTlv &aRouteTlv, RxInfo &aRxInfo);
    Error    ReadAndProcessRouteTlvOnFtdChild(RxInfo &aRxInfo, uint8_t aParentId);
    void     StopAdvertiseTrickleTimer(void);
    uint32_t DetermineAdvertiseIntervalMax(void) const;
    Error    SendAddressSolicit(ThreadStatusTlv::Status aStatus);
    void     SendAddressSolicitResponse(const Coap::Message    &aRequest,
                                        ThreadStatusTlv::Status aResponseStatus,
                                        const Router           *aRouter,
                                        const Ip6::MessageInfo &aMessageInfo);
    void     SendAddressRelease(void);
    void     SendAdvertisement(void);
    void     SendLinkRequest(Router *aRouter);
    Error    SendLinkAccept(const LinkAcceptInfo &aInfo);
    void     SendParentResponse(const ParentResponseInfo &aInfo);
    Error    SendChildIdResponse(Child &aChild);
    Error    SendChildUpdateRequestToChild(Child &aChild);
    void     SendChildUpdateResponseToChild(Child                  *aChild,
                                            const Ip6::MessageInfo &aMessageInfo,
                                            const TlvList          &aTlvList,
                                            const RxChallenge      &aChallenge);
    void     SendMulticastDataResponse(void);
    void     SendDataResponse(const Ip6::Address &aDestination,
                              const TlvList      &aTlvList,
                              const Message      *aRequestMessage = nullptr);
    Error    SendDiscoveryResponse(const Ip6::Address &aDestination, const DiscoveryResponseInfo &aInfo);
    void     SetStateRouter(uint16_t aRloc16);
    void     SetStateLeader(uint16_t aRloc16, LeaderStartMode aStartMode);
    void     SetStateRouterOrLeader(DeviceRole aRole, uint16_t aRloc16, LeaderStartMode aStartMode);
    void     StopLeader(void);
    void     SynchronizeChildNetworkData(void);
    Error    ProcessAddressRegistrationTlv(RxInfo &aRxInfo, Child &aChild);
    bool     HasNeighborWithGoodLinkQuality(void) const;
    void     HandlePartitionChange(void);
    void     SetChildStateToValid(Child &aChild);
    bool     HasChildren(void);
    void     RemoveChildren(void);
    bool     ShouldDowngrade(uint8_t aNeighborId, const RouteTlv &aRouteTlv) const;
    bool     NeighborHasComparableConnectivity(const RouteTlv &aRouteTlv, uint8_t aNeighborId) const;
    void     HandleAdvertiseTrickleTimer(void);
    void     HandleAddressSolicitResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);
    void     HandleTimeTick(void);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    void SignalDuaAddressEvent(const Child &aChild, const Ip6::Address &aOldDua) const;
#endif

    static bool IsMessageMleSubType(const Message &aMessage);
    static bool IsMessageChildUpdateRequest(const Message &aMessage);
    static void HandleAdvertiseTrickleTimer(TrickleTimer &aTimer);
    static void HandleAddressSolicitResponse(void                *aContext,
                                             otMessage           *aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             Error                aResult);

    //------------------------------------------------------------------------------------------------------------------
    // Variables

    bool mRouterEligible : 1;
    bool mAddressSolicitPending : 1;
    bool mAddressSolicitRejected : 1;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    bool mCcmEnabled : 1;
    bool mThreadVersionCheckEnabled : 1;
#endif

    uint8_t mRouterId;
    uint8_t mPreviousRouterId;
    uint8_t mNetworkIdTimeout;
    uint8_t mRouterUpgradeThreshold;
    uint8_t mRouterDowngradeThreshold;
    uint8_t mLeaderWeight;
    uint8_t mPreviousPartitionRouterIdSequence;
    uint8_t mPreviousPartitionIdTimeout;
    uint8_t mChildRouterLinks;
    uint8_t mAlternateRloc16Timeout;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t mMaxChildIpAddresses;
#endif
    int8_t   mParentPriority;
    uint16_t mNextChildId;
    uint32_t mPreviousPartitionIdRouter;
    uint32_t mPreviousPartitionId;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint32_t mPreferredLeaderPartitionId;
#endif

    TrickleTimer               mAdvertiseTrickleTimer;
    ChildTable                 mChildTable;
    RouterTable                mRouterTable;
    RouterRoleRestorer         mRouterRoleRestorer;
    RouterRoleTransition       mRouterRoleTransition;
    Ip6::Netif::UnicastAddress mLeaderAloc;
#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
    DeviceProperties mDeviceProperties;
#endif
#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    MeshCoP::SteeringData mSteeringData;
#endif
    Callback<otThreadDiscoveryRequestCallback> mDiscoveryRequestCallback;
};

DeclareTmfHandler(MleRouter, kUriAddressSolicit);
DeclareTmfHandler(MleRouter, kUriAddressRelease);

#endif // OPENTHREAD_FTD

#if OPENTHREAD_MTD

typedef Mle MleRouter;

#endif

} // namespace Mle

/**
 * @}
 */

} // namespace ot

#endif // MLE_ROUTER_HPP_
