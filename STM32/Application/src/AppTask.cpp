/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*STM32 includes*/
#include "AppTask.h"
#include "AppEvent.h"
#include "app_common.h"
#include "cmsis_os.h"
#include "CommissionableDataProvider.h"
#include "dbg_trace.h"
#include "FactoryDataProvider.h"
#include "LED_Animation.h"

#if (OTA_SUPPORT == 1)
#include "ota.h"
#endif /* (OTA_SUPPORT == 1) */

#if HIGHWATERMARK
#include "memory_buffer_alloc.h"
#endif

/*Matter includes*/
#include <DeviceInfoProviderImpl.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/server/Dnssd.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <inet/EndPointStateOpenThread.h>
#include <cmath>
#include <platform/CHIPDeviceLayer.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#if CHIP_ENABLE_OPENTHREAD
#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/ThreadStackManager.h>
#endif

using namespace ::chip;
using namespace ::chip::app;
using namespace chip::TLV;
using namespace chip::Credentials;
using namespace chip::DeviceLayer;
using namespace ::chip::Platform;
using namespace ::chip::Credentials;
using namespace ::chip::app::Clusters;
using DeviceLayer::PersistedStorage::KeyValueStoreMgr;

AppTask AppTask::sAppTask;
FactoryDataProvider mFactoryDataProvider;

#define APP_EVENT_QUEUE_SIZE 10
#define NVM_TIMEOUT 1000 // timer to handle PB to save data in nvm or do a factory reset

static QueueHandle_t sAppEventQueue;
const osThreadAttr_t AppTask_attr = {
	.name = APPTASK_NAME,
	.attr_bits = APP_ATTR_BITS,
	.cb_mem = APP_CB_MEM,
	.cb_size = APP_CB_SIZE,
	.stack_mem = APP_STACK_MEM,
	.stack_size = APP_STACK_SIZE,
	.priority = APP_PRIORITY
};

static bool sIsThreadProvisioned = false;
static bool sIsThreadEnabled = false;
static bool sHaveBLEConnections = false;
static bool sFabricNeedSaved = false;
static bool sFailCommissioning = false;
static bool sHaveFabric = false;

extern LED ledCommissioning;

DeviceInfoProviderImpl gExampleDeviceInfoProvider;

CHIP_ERROR AppTask::StartAppTask()
{
	sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
	if (sAppEventQueue == NULL)
	{
		APP_DBG("Failed to allocate app event queue");
		return CHIP_ERROR_NO_MEMORY;
	}

	// Start App task.
	osThreadNew(AppTaskMain, NULL, &AppTask_attr);

	return CHIP_NO_ERROR;
}

void LockOpenThreadTask() { ThreadStackMgr().LockThreadStack(); }

void UnlockOpenThreadTask() { ThreadStackMgr().UnlockThreadStack(); }

CHIP_ERROR AppTask::Init()
{
	CHIP_ERROR err = CHIP_NO_ERROR;

	ThreadStackMgr().InitThreadStack();

#if ( CHIP_DEVICE_CONFIG_ENABLE_SED == 1)
	ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
	ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_FullEndDevice);
#endif

	PlatformMgr().AddEventHandler(MatterEventHandler, 0);

#if CHIP_DEVICE_CONFIG_ENABLE_EXTENDED_DISCOVERY
	chip::app::DnssdServer::Instance().SetExtendedDiscoveryTimeoutSecs(extDiscTimeoutSecs);
#endif

	// Init ZCL Data Model
	static CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();
	ReturnErrorOnFailure(mFactoryDataProvider.Init());
	SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
	SetCommissionableDataProvider(&mFactoryDataProvider);
	SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);

	Inet::EndPointStateOpenThread::OpenThreadEndpointInitParam nativeParams;
	nativeParams.lockCb = LockOpenThreadTask;
	nativeParams.unlockCb = UnlockOpenThreadTask;
	nativeParams.openThreadInstancePtr = ThreadStackMgrImpl().OTInstance();
	initParams.endpointNativeParams = static_cast<void*>(&nativeParams);
	Server::GetInstance().Init(initParams);

	gExampleDeviceInfoProvider.SetStorageDelegate(&Server::GetInstance().GetPersistentStorage());
	SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

	err = ThermostatMgr().Init();
	if (err != CHIP_NO_ERROR)
	{
		APP_DBG("TempMgr().Init() failed");
		return err;
	}
	ThermostatMgr().SetCallbacks(ActionCompleted);

	ConfigurationMgr().LogDeviceConfig();

	if (Server::GetInstance().GetFabricTable().FabricCount() == 0)
	{
		PrintOnboardingCodes(RendezvousInformationFlags(RendezvousInformationFlag::kBLE));
		Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
		APP_DBG("BLE advertising started. Waiting for Pairing.");
	}
	else
	{
		sHaveFabric = true;
	}

	err = PlatformMgr().StartEventLoopTask();
	if (err != CHIP_NO_ERROR)
	{
		APP_DBG("PlatformMgr().StartEventLoopTask() failed");
	}

	return err;
}

CHIP_ERROR AppTask::InitMatter()
{
	CHIP_ERROR err = CHIP_NO_ERROR;

	err = MemoryInit();
	if (err != CHIP_NO_ERROR)
	{
		APP_DBG("Platform::MemoryInit() failed");
	}
	else
	{
		APP_DBG("Init CHIP stack");
		err = PlatformMgr().InitChipStack();
		if (err != CHIP_NO_ERROR)
		{
			APP_DBG("PlatformMgr().InitChipStack() failed");
		}
	}
	return err;
}

void AppTask::AppTaskMain(void* pvParameter)
{
	AppEvent event;

	CHIP_ERROR err = sAppTask.Init();
#if HIGHWATERMARK
	UBaseType_t uxHighWaterMark;
	HeapStats_t HeapStatsInfo;
	size_t max_used;
	size_t max_blocks;
#endif // endif HIGHWATERMARK
	if (err != CHIP_NO_ERROR)
	{
		APP_DBG("App task init failed ");
	}

	APP_DBG("App Task started");

	while (true)
	{
		BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, portMAX_DELAY);
		while (eventReceived == pdTRUE)
		{
			sAppTask.DispatchEvent(&event);
			eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
		}

#if HIGHWATERMARK
		uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
		vPortGetHeapStats(&HeapStatsInfo);
		mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);

#endif // endif HIGHWATERMARK
	}
}

void AppTask::PostEvent(const AppEvent* aEvent)
{
	if (sAppEventQueue != NULL)
	{
		if (!xQueueSend(sAppEventQueue, aEvent, 1))
		{
			ChipLogError(NotSpecified, "Failed to post event to app task event queue");
		}
	}
	else
	{
		ChipLogError(NotSpecified, "Event Queue is NULL should never happen");
	}
}

void AppTask::DispatchEvent(AppEvent* aEvent)
{
	if (aEvent->Handler)
	{
		aEvent->Handler(aEvent);
	}
	else
	{
		ChipLogError(NotSpecified, "Event received with no handler. Dropping event.");
	}
}

/**
 * Update cluster status after application level changes
 */
float tempTime = 0.0f;

void AppTask::UpdateClusterState()
{
	float temp = 2000 + 500 * sinf(tempTime);
	tempTime += 0.25f;
	ThermostatMgr().SetCurrentTemp(tempTime);
}

void AppTask::ActionCompleted(ThermostatManager::Action_t aAction)
{
	if (aAction == ThermostatManager::CoolSet)
	{
		APP_DBG("Cooling setpoint completed");
	}
	else if (aAction == ThermostatManager::HeatSet)
	{
		APP_DBG("Heating setpoint completed");
	}
	else if (aAction == ThermostatManager::ModeSet)
	{
		APP_DBG("Mode set completed");
	}
	else if (aAction == ThermostatManager::LocalTemperature)
	{
		APP_DBG("Temperature set completed");
	}

	// UpdateTempGUI();
}

void AppTask::UpdateLEDs()
{
	ledCommissioning.SetEffect(LED::Effect::Solid);

	if (sIsThreadProvisioned && sIsThreadEnabled)
	{
		ledCommissioning.SetColor(0, 100, 0);
	}
	else
	{
		ledCommissioning.SetColor(0, 0, 0);
	}

	if (sHaveBLEConnections)
	{
		ledCommissioning.SetColor(0, 0, 100);
	}
	else
	{
		ledCommissioning.SetColor(0, 0, 0);
	}

	if (sHaveFabric)
	{
		ledCommissioning.SetColor(0, 100, 0);
	}

	if (sFailCommissioning)
	{
		ledCommissioning.SetColor(100, 0, 0);
	}
}

void AppTask::DelayNvmHandler(void)
{
	AppEvent event;
	event.Type = AppEvent::kEventType_Timer;
	event.Handler = UpdateNvmEventHandler;
	sAppTask.mFunction = kFunction_SaveNvm;
	sAppTask.PostEvent(&event);
}

void AppTask::UpdateNvmEventHandler(AppEvent* aEvent)
{
	uint8_t err = 0;

	if (sAppTask.mFunction == kFunction_SaveNvm)
	{
		err = NM_Dump();
		if (err == 0)
		{
			APP_DBG("SAVE NVM");
		}
		else
		{
			APP_DBG("Failed to SAVE NVM");
		}
	}
	else if (sAppTask.mFunction == kFunction_FactoryReset)
	{
		APP_DBG("FACTORY RESET");
		NM_ResetFactory();
	}
}

void AppTask::MatterEventHandler(const ChipDeviceEvent* event, intptr_t)
{
	switch (event->Type)
	{
		case DeviceEventType::kCHIPoBLEAdvertisingChange:
			{
				if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped)
				{
					ledCommissioning.SetColor(0, 0, 0);
				}
				break;
			}
		case DeviceEventType::kServiceProvisioningChange:
			{
				sIsThreadProvisioned = event->ServiceProvisioningChange.IsServiceProvisioned;
				UpdateLEDs();
				break;
			}

		case DeviceEventType::kThreadConnectivityChange:
			{
				sIsThreadEnabled = event->ThreadConnectivityChange.Result == kConnectivity_Established;
				UpdateLEDs();
				break;
			}

		case DeviceEventType::kCHIPoBLEConnectionEstablished:
			{
				sHaveBLEConnections = true;
				APP_DBG("kCHIPoBLEConnectionEstablished");

				UpdateLEDs();

				break;
			}

		case DeviceEventType::kCHIPoBLEConnectionClosed:
			{
				sHaveBLEConnections = false;
				APP_DBG("kCHIPoBLEConnectionClosed");
				if (sFabricNeedSaved)
				{
					APP_DBG("Start timer to save nvm after commissioning finish");
					DelayNvmHandler();
					sFabricNeedSaved = false;
				}
				UpdateLEDs();
				break;
			}
		case DeviceEventType::kCommissioningComplete:
			{
				sFabricNeedSaved = true;
				sHaveFabric = true;

				// check if ble is on, since before save in nvm we need to stop m0, Better to write in nvm when m0 is less busy
				if (sHaveBLEConnections == false)
				{
					APP_DBG("Start timer to save nvm after commissioning finish");
					DelayNvmHandler();
					sFabricNeedSaved = false; // put to false to avoid save in nvm 2 times
				}
				UpdateLEDs();
				break;
			}
		case DeviceEventType::kFailSafeTimerExpired:
			{
				sFailCommissioning = true;
				UpdateLEDs();
				break;
			}

		case DeviceEventType::kDnssdInitialized:
#if (OTA_SUPPORT == 1)
			InitializeOTARequestor();
#endif
			break;

		default:
			break;
	}
}