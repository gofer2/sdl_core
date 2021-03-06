/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sdl_rpc_plugin/commands/hmi/on_vr_command_notification.h"

#include "application_manager/message_helper.h"
#include "application_manager/policies/policy_handler.h"
#include "application_manager/state_controller.h"

#include "application_manager/event_engine/event.h"
#include "interfaces/HMI_API.h"
#include "interfaces/MOBILE_API.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

OnVRCommandNotification::OnVRCommandNotification(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    rpc_service::RPCService& rpc_service,
    HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handle)
    : NotificationFromHMI(message,
                          application_manager,
                          rpc_service,
                          hmi_capabilities,
                          policy_handle) {}

OnVRCommandNotification::~OnVRCommandNotification() {}

void OnVRCommandNotification::Run() {
  SDL_LOG_AUTO_TRACE();
  const uint32_t cmd_id =
      (*message_)[strings::msg_params][strings::cmd_id].asUInt();
  uint32_t max_cmd_id = application_manager_.get_settings().max_cmd_id();

  // Check if this is one of standard VR commands (i.e. "Help")
  if (cmd_id > max_cmd_id + 1) {
    SDL_LOG_INFO("Switched App");
    const uint32_t app_id = cmd_id - max_cmd_id;
    ApplicationSharedPtr app = application_manager_.application(app_id);
    if (app) {
      application_manager_.state_controller().SetRegularState(
          app,
          mobile_apis::PredefinedWindows::DEFAULT_WINDOW,
          mobile_apis::HMILevel::HMI_FULL,
          true);
    } else {
      SDL_LOG_ERROR("Unable to find appication " << app_id);
    }
    return;
  }

  // Check if this is "Help"
  if (cmd_id == max_cmd_id + 1) {
    return;
  }
  const uint32_t app_id =
      (*message_)[strings::msg_params][strings::app_id].asUInt();
  ApplicationSharedPtr app = application_manager_.application(app_id);
  if (!app) {
    SDL_LOG_ERROR("NULL pointer");
    return;
  }
  /* check if perform interaction is active
   * if it is active we should sent to HMI DeleteCommand request
   * and PerformInterActionResponse to mobile
   */
  if (0 != app->is_perform_interaction_active()) {
    event_engine::Event event(hmi_apis::FunctionID::VR_OnCommand);
    event.set_smart_object(*message_);
    event.raise(application_manager_.event_dispatcher());
  } else {
    (*message_)[strings::params][strings::function_id] =
        static_cast<int32_t>(mobile_apis::FunctionID::eType::OnCommandID);

    (*message_)[strings::msg_params][strings::trigger_source] =
        static_cast<int32_t>(mobile_apis::TriggerSource::TS_VR);
    SendNotificationToMobile(message_);
  }
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
