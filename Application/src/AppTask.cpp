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
#include "LED.h"

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

// #define STM32ThreadDataSet "STM32DataSet"
#define APP_EVENT_QUEUE_SIZE 10
#define NVM_TIMEOUT 1000 // timer to handle PB to save data in nvm or do a factory reset

static QueueHandle_t sAppEventQueue;
const osThreadAttr_t AppTask_attr = {.name = APPTASK_NAME, .attr_bits = APP_ATTR_BITS, .cb_mem = APP_CB_MEM, .cb_size = APP_CB_SIZE, .stack_mem = APP_STACK_MEM, .stack_size = APP_STACK_SIZE, .priority = APP_PRIORITY};

static bool sIsThreadProvisioned = false;
static bool sIsThreadEnabled = false;
static bool sHaveBLEConnections = false;
static bool sFabricNeedSaved = false;
static bool sFailCommissioning = false;
static bool sHaveFabric = false;

extern TIM_HandleTypeDef htim1;

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

void LockOpenThreadTask(void) { ThreadStackMgr().LockThreadStack(); }

void UnlockOpenThreadTask(void) { ThreadStackMgr().UnlockThreadStack(); }

CHIP_ERROR AppTask::Init()
{
	CHIP_ERROR err = CHIP_NO_ERROR;
	ChipLogProgress(NotSpecified, "Current Software Version: %s", CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING);

	ThreadStackMgr().InitThreadStack();

#if (CHIP_DEVICE_CONFIG_ENABLE_SED == 1)
	ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
	ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
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

	ConfigurationMgr().LogDeviceConfig();

	// Open commissioning after boot if no fabric was available
	if (Server::GetInstance().GetFabricTable().FabricCount() == 0)
	{
		PrintOnboardingCodes(RendezvousInformationFlags(RendezvousInformationFlag::kBLE));
		// Enable BLE advertisements
		Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow();
		APP_DBG("BLE advertising started. Waiting for Pairing.");
	}
	else
	{
		// try to attach to the thread network
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

		LED_SetColorRGB(0, 255, 0, 0);
		LED_Send(&htim1);

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
	if (PlatformMgr().TryLockChipStack())
	{
		ChipLogProgress(NotSpecified, "UpdateClusterState");

		float temp = 2000 + 500 * sinf(tempTime);
		tempTime += 0.25f;

		Protocols::InteractionModel::Status status = Thermostat::Attributes::LocalTemperature::Set(1, static_cast<int16_t>(temp));
		if (status != Protocols::InteractionModel::Status::Success)
		{
			ChipLogError(NotSpecified, "ERR: updating local temperature %x", static_cast<uint8_t>(status));
		}

		int16_t coolingSP = 0, heatingSP = 0;
		Thermostat::Attributes::OccupiedCoolingSetpoint::Get(1, &coolingSP);
		Thermostat::Attributes::OccupiedHeatingSetpoint::Get(1, &heatingSP);

		ChipLogError(NotSpecified, "ERR: cooling %d, heating %d", coolingSP, heatingSP);

		PlatformMgr().UnlockChipStack();
	}
}

void AppTask::UpdateLEDs(void)
{
	if (sIsThreadProvisioned && sIsThreadEnabled)
	{
	}
	else if ((sIsThreadProvisioned == false) || (sIsThreadEnabled == false))
	{
	}
	if (sHaveBLEConnections)
	{
		LED_SetColorRGB(1, 0, 0, 255);
		LED_Send(&htim1);
	}
	else if (sHaveBLEConnections == false)
	{
	}
}

void AppTask::MatterEventHandler(const ChipDeviceEvent* event, intptr_t)
{
	switch (event->Type)
	{
	case DeviceEventType::kServiceProvisioningChange:
		{
			sIsThreadProvisioned = event->ServiceProvisioningChange.IsServiceProvisioned;
			UpdateLEDs();
			break;
		}

	case DeviceEventType::kThreadConnectivityChange:
		{
			sIsThreadEnabled = (event->ThreadConnectivityChange.Result == kConnectivity_Established);
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