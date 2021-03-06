/*

 Copyright (c) 2018, Ford Motor Company
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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_SDL_RPC_PLUGIN_INCLUDE_SDL_RPC_PLUGIN_COMMANDS_MOBILE_ON_VEHICLE_DATA_NOTIFICATION_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_SDL_RPC_PLUGIN_INCLUDE_SDL_RPC_PLUGIN_COMMANDS_MOBILE_ON_VEHICLE_DATA_NOTIFICATION_H_

#include "application_manager/application.h"
#include "application_manager/commands/command_notification_impl.h"
#include "utils/macro.h"
#include "vehicle_info_plugin/custom_vehicle_data_manager.h"
#include "vehicle_info_plugin/vehicle_info_command_params.h"

namespace vehicle_info_plugin {
namespace app_mngr = application_manager;

namespace commands {

/**
 * @brief OnVehicleDataNotification class is used to send notification
 * to mobile device that some button was pressed on HMI.
 **/
class OnVehicleDataNotification
    : public app_mngr::commands::CommandNotificationImpl {
 public:
  /**
   * @brief OnVehicleDataNotification class constructor
   *
   * @param message Incoming SmartObject message
   **/
  OnVehicleDataNotification(const app_mngr::commands::MessageSharedPtr& message,
                            const VehicleInfoCommandParams& params);

  /**
   * @brief OnVehicleDataNotification class destructor
   **/
  virtual ~OnVehicleDataNotification();

  /**
   * @brief Execute command
   **/
  virtual void Run();

 private:
  /*
   * @brief Sends vehicle data notification to mobile device
   *
   * @param app Application to receive notification
   */
  void SendVehicleData(app_mngr::ApplicationConstSharedPtr app);

  CustomVehicleDataManager& custom_vehicle_data_manager_;

  DISALLOW_COPY_AND_ASSIGN(OnVehicleDataNotification);
};

}  // namespace commands
}  // namespace vehicle_info_plugin

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_SDL_RPC_PLUGIN_INCLUDE_SDL_RPC_PLUGIN_COMMANDS_MOBILE_ON_VEHICLE_DATA_NOTIFICATION_H_
