/*
 Copyright (c) 2020, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "vehicle_info_plugin/vehicle_info_pending_resumption_handler.h"
#include "application_manager/event_engine/event_observer.h"
#include "application_manager/message_helper.h"
#include "application_manager/resumption/resumption_data_processor.h"
#include "utils/helpers.h"

namespace vehicle_info_plugin {

CREATE_LOGGERPTR_GLOBAL(logger_, "VehicleInfoPendingResumptionHandler")

VehicleInfoPendingResumptionHandler::VehicleInfoPendingResumptionHandler(
    application_manager::ApplicationManager& application_manager)
    : ExtensionPendingResumptionHandler(application_manager) {}

template <class Key, class Value>
std::set<Key> EnumerateKeys(std::map<Key, Value>& container) {
  std::set<std::string> keys;

  std::transform(
      container.begin(),
      container.end(),
      std::inserter(keys, keys.end()),
      [&](const std::pair<std::string, bool>& pair) { return pair.first; });

  void FillResponseWithMissedVD(
      const VehicleInfoPendingResumptionHandler::VehicleDataList& vehicle_data,
      smart_objects::SmartObject* response) {
    DCHECK(response)
    namespace strings = application_manager::strings;
    auto& msg_params = (*response)[strings::msg_params];
    for (const auto& vd : vehicle_data) {
      smart_objects::SmartObject vd_result(smart_objects::SmartType_Map);
      vd_result[strings::result_code] =
          hmi_apis::Common_VehicleDataResultCode::VDRC_SUCCESS;
      msg_params[vd] = vd_result;
    }

    return true;
  }

  VehicleInfoPendingResumptionHandler::VehicleDataList result;
  if (!resumption::IsResponseSuccessful(response)) {
    return result;
  }
}  // namespace vehicle_info_plugin

void VehicleInfoPendingResumptionHandler::ClearPendingRequestsMap() {
  using namespace application_manager;

  for (auto const& it : pending_requests_) {
    const hmi_apis::FunctionID::eType timed_out_pending_request_fid =
        static_cast<hmi_apis::FunctionID::eType>(
            it.second[strings::params][strings::function_id].asInt());
    unsubscribe_from_event(timed_out_pending_request_fid);
  }

  pending_requests_.clear();
}

void VehicleInfoPendingResumptionHandler::ClearPendingResumptionRequests() {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace application_manager;

  ClearPendingRequestsMap();
  if (!freezed_resumptions_.empty()) {
    ResumptionAwaitingHandling freezed_resumption =
        freezed_resumptions_.front();
    freezed_resumptions_.pop();

    std::set<std::string> subscriptions =
        freezed_resumption.ext.Subscriptions();

    auto request = CreateSubscribeRequestToHMI(subscriptions);
    const uint32_t cid =
        (*request)[strings::params][strings::correlation_id].asUInt();
    const hmi_apis::FunctionID::eType fid =
        static_cast<hmi_apis::FunctionID::eType>(
            (*request)[strings::params][strings::function_id].asInt());
    auto resumption_req = MakeResumptionRequest(cid, fid, *request);
    auto subscriber = freezed_resumption.subscriber;
    subscriber(freezed_resumption.app_id, resumption_req);
    LOG4CXX_DEBUG(logger_,
                  "Subscribing for event with function id: "
                      << fid << " correlation id: " << cid);
    subscribe_on_event(fid, cid);
    pending_requests_[cid] = *request;
    LOG4CXX_DEBUG(logger_,
                  "Sending request with fid: " << fid << " and cid: " << cid);
    application_manager_.GetRPCService().ManageHMICommand(request);
  }
}

void VehicleInfoPendingResumptionHandler::on_event(
    const application_manager::event_engine::Event& event) {
  using namespace application_manager;
  LOG4CXX_AUTO_TRACE(logger_);

  const smart_objects::SmartObject& response = event.smart_object();
  const uint32_t corr_id = event.smart_object_correlation_id();

  smart_objects::SmartObject pending_request;
  if (pending_requests_.find(corr_id) == pending_requests_.end()) {
    LOG4CXX_DEBUG(logger_, "corr id" << corr_id << " NOT found");
    return;
  }
  pending_request = pending_requests_[corr_id];
  pending_requests_.erase(corr_id);

  LOG4CXX_DEBUG(logger_,
                "Received event with function id: "
                    << event.id() << " and correlation id: " << corr_id);

  if (freezed_resumptions_.empty()) {
    LOG4CXX_DEBUG(logger_, "freezed resumptions is empty");
    return;
  }

  std::map<std::string, bool> subscription_results =
      ExtractSubscribeResults(pending_request, response);

  LOG4CXX_DEBUG(logger_,
                "pending_requests_.size()" << pending_requests_.size());

  std::set<std::string> successful_subscriptions =
      EnumerateKeys(subscription_results);

  const auto vs_count_in_response =
      response_message[application_manager::strings::msg_params].length();
  if (resumption::IsResponseSuccessful(response_message) &&
      vs_count_in_response == 0) {
    FillResponseWithMissedVD(current_pending.requested_vehicle_data_,
                             &response_message);
  }

  std::set<std::string> subscriptions = freezed_resumption.ext.Subscriptions();

  if (!IsResumptionResultSuccessful(subscription_results)) {
    LOG4CXX_DEBUG(logger_, "Resumption of subscriptions is NOT successful");
  } else {
    LOG4CXX_DEBUG(logger_, "Resumption of subscriptions is successful");
    RemoveSucessfulSubscriptions(subscriptions, successful_subscriptions);
  }

  auto request = CreateSubscribeRequestToHMI(subscriptions);
  const uint32_t cid =
      (*request)[strings::params][strings::correlation_id].asUInt();
  const hmi_apis::FunctionID::eType fid =
      static_cast<hmi_apis::FunctionID::eType>(
          (*request)[strings::params][strings::function_id].asInt());
  auto resumption_req = MakeResumptionRequest(cid, fid, *request);
  subscribe_on_event(fid, cid);
  subscriber(freezed_resumption.app_id, resumption_req);
  LOG4CXX_DEBUG(logger_,
                "Subscribing for event with function id: "
                    << fid << " correlation id: " << cid);
  pending_requests_[cid] = *request;
  LOG4CXX_DEBUG(logger_,
                "Sending request with fid: " << fid << " and cid: " << cid);
  application_manager_.GetRPCService().ManageHMICommand(request);
}

std::map<std::string, bool>
VehicleInfoPendingResumptionHandler::ExtractSubscribeResults(
    const smart_objects::SmartObject& response,
    const smart_objects::SmartObject& request) const {
  using namespace application_manager;
  const hmi_apis::Common_Result::eType result_code =
      static_cast<hmi_apis::Common_Result::eType>(
          response[strings::params][application_manager::hmi_response::code]
              .asInt());
  bool succesfull_response = (result_code == hmi_apis::Common_Result::SUCCESS ||
                              result_code == hmi_apis::Common_Result::WARNINGS);
  const auto response_keys =
      response[application_manager::strings::msg_params].enumerate();
  const auto request_keys =
      request[application_manager::strings::msg_params].enumerate();

  auto response_params = response[strings::msg_params];

  std::map<std::string, bool> subscription_results;

  if (!succesfull_response) {
    for (auto key : request_keys) {
      subscription_results[key] = false;
    }
  }

  if (succesfull_response) {
    for (auto key : request_keys) {
      if (!helpers::in_range(response_keys, key)) {
        subscription_results[key] = true;
      } else {
        const auto kSuccess =
            hmi_apis::Common_VehicleDataResultCode::VDRC_SUCCESS;
        const auto vd_result_code =
            response_params[key][application_manager::strings::result_code]
                .asInt();
        subscription_results[key] = vd_result_code == kSuccess;
      }
    }
  }
  return subscription_results;
}

void VehicleInfoPendingResumptionHandler::HandleResumptionSubscriptionRequest(
    application_manager::AppExtension& extension,
    resumption::Subscriber& subscriber,
    application_manager::Application& app) {
  LOG4CXX_AUTO_TRACE(logger_);

  VehicleInfoAppExtension& ext =
      dynamic_cast<VehicleInfoAppExtension&>(extension);

  std::set<std::string> subscriptions = ext.Subscriptions();

  smart_objects::SmartObjectSPtr request =
      CreateSubscribeRequestToHMI(subscriptions);

  smart_objects::SmartObject& request_ref = *request;
  const auto function_id = static_cast<hmi_apis::FunctionID::eType>(
      request_ref[application_manager::strings::params]
                 [application_manager::strings::function_id]
                     .asInt());
  const uint32_t corr_id =
      request_ref[application_manager::strings::params]
                 [application_manager::strings::correlation_id]
                     .asUInt();

  auto resumption_request =
      MakeResumptionRequest(corr_id, function_id, *request);

  if (pending_requests_.empty()) {
    LOG4CXX_DEBUG(logger_,
                  "There are no pending requests for app_id: " << app.app_id());
    pending_requests_[corr_id] = request_ref;
    subscribe_on_event(function_id, corr_id);
    subscriber(app.app_id(), resumption_request);
    LOG4CXX_DEBUG(logger_,
                  "Sending request with function id: "
                      << function_id << " and correlation_id: " << corr_id);
    application_manager_.GetRPCService().ManageHMICommand(request);
    return;
  } else {
    LOG4CXX_DEBUG(logger_,
                  "There are pending requests for app_id: " << app.app_id());
    ResumptionAwaitingHandling frozen_res(app.app_id(), ext, subscriber);
    freezed_resumptions_.push(frozen_res);
  }
}

smart_objects::SmartObjectSPtr
VehicleInfoPendingResumptionHandler::CreateSubscribeRequestToHMI(
    const std::set<std::string>& subscriptions) {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace application_manager;
  smart_objects::SmartObject msg_params =
      smart_objects::SmartObject(smart_objects::SmartType_Map);

  for (const auto& ivi_data : subscriptions) {
    msg_params[ivi_data] = true;
  }

  smart_objects::SmartObjectSPtr request =
      application_manager::MessageHelper::CreateModuleInfoSO(
          hmi_apis::FunctionID::VehicleInfo_SubscribeVehicleData,
          application_manager_);
  (*request)[strings::msg_params] = msg_params;

  return request;
}
}  // namespace vehicle_info_plugin