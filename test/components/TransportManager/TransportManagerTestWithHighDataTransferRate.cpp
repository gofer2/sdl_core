/*
    Tests huge amount of data tranfer through Transport Manager. Currently there are 10000 iterations, on each iteration
    data randomly sent from TM client to device or from device to TM client. Size of sent data is up to 1 MiB.
    Average amount of transfered data is 2.4 GiB
 */
#define __STDC_FORMAT_MACROS

#include <algorithm>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "TransportManager/ITransportManagerDataListener.hpp"
#include "TransportManager/ITransportManagerDeviceListener.hpp"
#include "../../../src/components/TransportManager/src/CTransportManager.hpp"
#include "../../../src/components/TransportManager/src/IDeviceAdapter.hpp"

namespace test
{
    namespace components
    {
        namespace TransportManager
        {
            namespace HighDataTransferRate
            {
                class MockDataListener: public NsSmartDeviceLink::NsTransportManager::ITransportManagerDataListener
                {
                public:
                    MockDataListener(pthread_mutex_t & Mutex);

                    MOCK_METHOD3(onFrameReceived, void(NsSmartDeviceLink::NsTransportManager::tConnectionHandle ConnectionHandle, const uint8_t * Data, size_t DataSize));
                    MOCK_METHOD3(onFrameSendCompleted, void(NsSmartDeviceLink::NsTransportManager::tConnectionHandle ConnectionHandle, int FrameSequenceNumber, NsSmartDeviceLink::NsTransportManager::ESendStatus SendStatus));

                    void unlockMutexAfterFrameReceived(NsSmartDeviceLink::NsTransportManager::tConnectionHandle, const uint8_t *, size_t);
                    void unlockMutexAfterFrameSendCompleted(NsSmartDeviceLink::NsTransportManager::tConnectionHandle, int, NsSmartDeviceLink::NsTransportManager::ESendStatus);

                private:
                    pthread_mutex_t & mMutex;
                };

                class MockDeviceListener: public NsSmartDeviceLink::NsTransportManager::ITransportManagerDeviceListener
                {
                public:
                    MockDeviceListener(pthread_mutex_t & Mutex);

                    MOCK_METHOD1(onDeviceListUpdated, void(const NsSmartDeviceLink::NsTransportManager::tDeviceList & DeviceList));
                    MOCK_METHOD2(onApplicationConnected, void(const NsSmartDeviceLink::NsTransportManager::SDeviceInfo & ConnectedDevice, const NsSmartDeviceLink::NsTransportManager::tConnectionHandle Connection));
                    MOCK_METHOD2(onApplicationDisconnected, void(const NsSmartDeviceLink::NsTransportManager::SDeviceInfo & DisconnectedDevice, const NsSmartDeviceLink::NsTransportManager::tConnectionHandle Connection));

                    void unlockMutexAfterDeviceListUpdate(const NsSmartDeviceLink::NsTransportManager::tDeviceList &);
                    void unlockMutexAfterApplicationConnectedDisconnected(const NsSmartDeviceLink::NsTransportManager::SDeviceInfo &, const NsSmartDeviceLink::NsTransportManager::tConnectionHandle);

                private:
                    pthread_mutex_t & mMutex;
                };

                class MockDeviceAdapter: public NsSmartDeviceLink::NsTransportManager::IDeviceAdapter
                {
                public:
                    MockDeviceAdapter(pthread_mutex_t & Mutex);
                    virtual NsSmartDeviceLink::NsTransportManager::EDeviceType getDeviceType(void) const;

                    MOCK_METHOD1(connectDevice, void (const NsSmartDeviceLink::NsTransportManager::tDeviceHandle DeviceHandle));
                    MOCK_METHOD1(disconnectDevice, void (const NsSmartDeviceLink::NsTransportManager::tDeviceHandle DeviceHandle));
                    MOCK_METHOD0(run, void());
                    MOCK_METHOD0(scanForNewDevices, void());
                    MOCK_METHOD4(sendFrame, void(NsSmartDeviceLink::NsTransportManager::tConnectionHandle ConnectionHandle, const uint8_t * Data, size_t DataSize, int UserData));

                    void unlockMutexAfterSendFrame(NsSmartDeviceLink::NsTransportManager::tConnectionHandle, const uint8_t *, size_t, int UserData);

                    NsSmartDeviceLink::NsTransportManager::SDeviceInfo mMockDeviceInfo;
                    NsSmartDeviceLink::NsTransportManager::tConnectionHandle mMockConnectionHandle;

                private:
                    pthread_mutex_t & mMutex;
                };

                class TestTransportManager: public NsSmartDeviceLink::NsTransportManager::CTransportManager
                {
                public:
                    TestTransportManager(test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter * DeviceAdapter);
                    ~TestTransportManager(void);
                protected:
                    virtual void initializeDeviceAdapters(void);

                private:
                    MockDeviceAdapter * mMockDeviceAdapter;
                };

                std::string formatDataSize(const uint64_t DataSize);
            }
        }
    }
}

test::components::TransportManager::HighDataTransferRate::MockDataListener::MockDataListener(pthread_mutex_t & Mutex):
mMutex(Mutex)
{
}

void test::components::TransportManager::HighDataTransferRate::MockDataListener::unlockMutexAfterFrameReceived(NsSmartDeviceLink::NsTransportManager::tConnectionHandle, const uint8_t *, size_t)
{
    pthread_mutex_unlock(&mMutex);
}

void test::components::TransportManager::HighDataTransferRate::MockDataListener::unlockMutexAfterFrameSendCompleted(NsSmartDeviceLink::NsTransportManager::tConnectionHandle, int, NsSmartDeviceLink::NsTransportManager::ESendStatus)
{
    pthread_mutex_unlock(&mMutex);
}

test::components::TransportManager::HighDataTransferRate::MockDeviceListener::MockDeviceListener(pthread_mutex_t & Mutex):
mMutex(Mutex)
{
}

void test::components::TransportManager::HighDataTransferRate::MockDeviceListener::unlockMutexAfterDeviceListUpdate(const NsSmartDeviceLink::NsTransportManager::tDeviceList &)
{
    pthread_mutex_unlock(&mMutex);
}

void test::components::TransportManager::HighDataTransferRate::MockDeviceListener::unlockMutexAfterApplicationConnectedDisconnected(const NsSmartDeviceLink::NsTransportManager::SDeviceInfo &, const NsSmartDeviceLink::NsTransportManager::tConnectionHandle)
{
    pthread_mutex_unlock(&mMutex);
}

test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter::MockDeviceAdapter(pthread_mutex_t & Mutex):
mMockDeviceInfo(),
mMockConnectionHandle(NsSmartDeviceLink::NsTransportManager::InvalidConnectionHandle),
mMutex(Mutex)
{
    mMockDeviceInfo.mDeviceHandle = NsSmartDeviceLink::NsTransportManager::InvalidDeviceHandle;
    mMockDeviceInfo.mDeviceType = NsSmartDeviceLink::NsTransportManager::DeviceBluetooth;
}

NsSmartDeviceLink::NsTransportManager::EDeviceType test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter::getDeviceType(void) const
{
    return mMockDeviceInfo.mDeviceType;
}

void test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter::unlockMutexAfterSendFrame(NsSmartDeviceLink::NsTransportManager::tConnectionHandle, const uint8_t *, size_t, int UserData)
{
    pthread_mutex_unlock(&mMutex);
}

test::components::TransportManager::HighDataTransferRate::TestTransportManager::TestTransportManager(test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter * DeviceAdapter):
CTransportManager(),
mMockDeviceAdapter(DeviceAdapter)
{
}

test::components::TransportManager::HighDataTransferRate::TestTransportManager::~TestTransportManager(void)
{
}

void test::components::TransportManager::HighDataTransferRate::TestTransportManager::initializeDeviceAdapters(void)
{
    addDeviceAdapter(mMockDeviceAdapter);
}

MATCHER_P2(BuffersSame, buffer, size, "Matches two buffers contents")
{
    bool result = false;

    if (size == std::tr1::get<1>(arg))
    {
        result = (0 == memcmp(buffer, std::tr1::get<0>(arg), size));
    }

    return result;
}

#define TRY_LOCK_AND_FAIL_ON_TIMEOUT(mutex)                     \
    {                                                           \
        timespec timeoutTime;                                   \
                                                                \
        clock_gettime(CLOCK_REALTIME, &timeoutTime);            \
        timeoutTime.tv_sec += 1;                                \
                                                                \
        if (0 != pthread_mutex_timedlock(&mutex, &timeoutTime)) \
        {                                                       \
            FAIL() << "Mutex lock timeout";                     \
        }                                                       \
    }

std::string test::components::TransportManager::HighDataTransferRate::formatDataSize(const uint64_t DataSize)
{
    const uint64_t cKiB = 1024u;
    const uint64_t cMiB = cKiB * 1024u;
    const uint64_t cGiB = cMiB * 1024u;
    const uint64_t cTiB = cGiB * 1024u;

    std::stringstream s;

    s << std::fixed << std::setprecision(2);

    if (DataSize < cKiB)
    {
        s << DataSize << " B";
    }
    else if (DataSize < cMiB)
    {
        s << static_cast<double>(DataSize) / cKiB << " KiB";
    }
    else if (DataSize < cGiB)
    {
        s << static_cast<double>(DataSize) / cMiB << " MiB";
    }
    else if (DataSize < cTiB)
    {
        s << static_cast<double>(DataSize) / cGiB << " GiB";
    }
    else
    {
        s << static_cast<double>(DataSize) / cTiB << " TiB";
    }

    return s.str();
}

TEST(TransportManager, HighDataTransferRate)
{
    pthread_mutex_t callbacksMutex;

    pthread_mutex_init(&callbacksMutex, 0);

    test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter * mockDeviceAdapter = new test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter(callbacksMutex);
    test::components::TransportManager::HighDataTransferRate::MockDataListener mockDataListener(callbacksMutex);
    test::components::TransportManager::HighDataTransferRate::MockDeviceListener mockDeviceListener(callbacksMutex);
    test::components::TransportManager::HighDataTransferRate::TestTransportManager transportManager(mockDeviceAdapter);

    EXPECT_CALL(*mockDeviceAdapter, run()).Times(1);

    transportManager.run();

    transportManager.addDeviceListener(&mockDeviceListener);
    transportManager.addDataListener(&mockDataListener);

    NsSmartDeviceLink::NsTransportManager::tDeviceList deviceList;

    mockDeviceAdapter->mMockDeviceInfo.mDeviceHandle = 1;
    mockDeviceAdapter->mMockDeviceInfo.mUniqueDeviceId = std::string("id");
    mockDeviceAdapter->mMockDeviceInfo.mUserFriendlyName = std::string("Name");
    deviceList.push_back(mockDeviceAdapter->mMockDeviceInfo);

    EXPECT_CALL(mockDeviceListener, onDeviceListUpdated(::testing::_)).Times(0);
    EXPECT_CALL(mockDeviceListener, onApplicationConnected(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockDeviceListener, onApplicationDisconnected(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockDataListener, onFrameReceived(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockDataListener, onFrameSendCompleted(::testing::_, ::testing::_, ::testing::_)).Times(0);

    NsSmartDeviceLink::NsTransportManager::tInternalDeviceList internalDeviceList;
    internalDeviceList.push_back(NsSmartDeviceLink::NsTransportManager::SInternalDeviceInfo(mockDeviceAdapter->mMockDeviceInfo.mDeviceHandle, mockDeviceAdapter->mMockDeviceInfo.mUserFriendlyName, mockDeviceAdapter->mMockDeviceInfo.mUniqueDeviceId));

    TRY_LOCK_AND_FAIL_ON_TIMEOUT(callbacksMutex);

    EXPECT_CALL(mockDeviceListener, onDeviceListUpdated(::testing::ContainerEq(deviceList)))
        .Times(1)
        .WillOnce(::testing::Invoke(&mockDeviceListener, &test::components::TransportManager::HighDataTransferRate::MockDeviceListener::unlockMutexAfterDeviceListUpdate))
        .RetiresOnSaturation();

    transportManager.onDeviceListUpdated(mockDeviceAdapter, internalDeviceList);

    mockDeviceAdapter->mMockConnectionHandle = 1;

    TRY_LOCK_AND_FAIL_ON_TIMEOUT(callbacksMutex);

    EXPECT_CALL(mockDeviceListener, onApplicationConnected(mockDeviceAdapter->mMockDeviceInfo, mockDeviceAdapter->mMockConnectionHandle))
        .Times(1)
        .WillOnce(::testing::Invoke(&mockDeviceListener, &test::components::TransportManager::HighDataTransferRate::MockDeviceListener::unlockMutexAfterApplicationConnectedDisconnected))
        .RetiresOnSaturation();

    transportManager.onApplicationConnected(mockDeviceAdapter, mockDeviceAdapter->mMockDeviceInfo, mockDeviceAdapter->mMockConnectionHandle);

    static const uint32_t cMaxFramePayloadSize = 1048576u;

    uint8_t * frameData = new uint8_t[cMaxFramePayloadSize + 8u];
    frameData[0] = 0x11;
    frameData[1] = 0x07;
    frameData[2] = 0x00;
    frameData[3] = 0x01;

    for (uint32_t b = 8u; b < cMaxFramePayloadSize + 8u; b += sizeof(int))
    {
        *((int*)&frameData[b]) = rand();
    }

    uint64_t bytesSent = 0u, bytesReceived = 0u;

    timespec dataTransferStartTime;
    clock_gettime(CLOCK_REALTIME, &dataTransferStartTime);

    for (int i = 0; i < 10000; ++i)
    {
        TRY_LOCK_AND_FAIL_ON_TIMEOUT(callbacksMutex);

        uint32_t framePayloadSize = static_cast<uint32_t>(rand()) % cMaxFramePayloadSize;
        uint32_t frameSize = framePayloadSize + 8u;
        *((unsigned int*)(&frameData[4])) = htonl(framePayloadSize);

        if (0 == rand() % 2)
        {
            EXPECT_CALL(mockDataListener, onFrameReceived(mockDeviceAdapter->mMockConnectionHandle, ::testing::_, ::testing::_))
                .With(::testing::Args<1, 2>(BuffersSame(frameData, frameSize)))
                .Times(1)
                .WillOnce(::testing::Invoke(&mockDataListener, &test::components::TransportManager::HighDataTransferRate::MockDataListener::unlockMutexAfterFrameReceived))
                .RetiresOnSaturation();

            transportManager.onFrameReceived(mockDeviceAdapter, mockDeviceAdapter->mMockConnectionHandle, frameData, frameSize);

            bytesReceived += frameSize;
        }
        else
        {
            int userData = 1234;
            EXPECT_CALL(*mockDeviceAdapter, sendFrame(mockDeviceAdapter->mMockConnectionHandle, ::testing::_, ::testing::_, userData))
                .With(::testing::Args<1, 2>(BuffersSame(frameData, frameSize)))
                .Times(1)
                .WillOnce(::testing::Invoke(mockDeviceAdapter, &test::components::TransportManager::HighDataTransferRate::MockDeviceAdapter::unlockMutexAfterSendFrame))
                .RetiresOnSaturation();

            transportManager.sendFrame(mockDeviceAdapter->mMockConnectionHandle, frameData, frameSize, userData);

            TRY_LOCK_AND_FAIL_ON_TIMEOUT(callbacksMutex);

            EXPECT_CALL(mockDataListener, onFrameSendCompleted(mockDeviceAdapter->mMockConnectionHandle, userData, NsSmartDeviceLink::NsTransportManager::SendStatusOK))
                .Times(1)
                .WillOnce(::testing::Invoke(&mockDataListener, &test::components::TransportManager::HighDataTransferRate::MockDataListener::unlockMutexAfterFrameSendCompleted))
                .RetiresOnSaturation();

            transportManager.onFrameSendCompleted(mockDeviceAdapter, mockDeviceAdapter->mMockConnectionHandle, userData, NsSmartDeviceLink::NsTransportManager::SendStatusOK);

            bytesSent += frameSize;
        }
    }

    timespec dataTransferEndTime;
    clock_gettime(CLOCK_REALTIME, &dataTransferEndTime);

    double dataTransferTime = static_cast<double>(dataTransferEndTime.tv_sec - dataTransferStartTime.tv_sec) + static_cast<double>(dataTransferEndTime.tv_nsec - dataTransferStartTime.tv_nsec) * 1e-9;

    TRY_LOCK_AND_FAIL_ON_TIMEOUT(callbacksMutex);

    EXPECT_CALL(mockDeviceListener, onApplicationDisconnected(mockDeviceAdapter->mMockDeviceInfo, mockDeviceAdapter->mMockConnectionHandle))
        .Times(1)
        .WillOnce(::testing::Invoke(&mockDeviceListener, &test::components::TransportManager::HighDataTransferRate::MockDeviceListener::unlockMutexAfterApplicationConnectedDisconnected))
        .RetiresOnSaturation();

    transportManager.onApplicationDisconnected(mockDeviceAdapter, mockDeviceAdapter->mMockDeviceInfo, mockDeviceAdapter->mMockConnectionHandle);

    sleep(1);

    delete [] frameData;

    pthread_mutex_destroy(&callbacksMutex);

    printf("Sent %s (%s/s), received %s (%s/s)\n",
           test::components::TransportManager::HighDataTransferRate::formatDataSize(bytesSent).c_str(),
           test::components::TransportManager::HighDataTransferRate::formatDataSize(bytesSent / dataTransferTime).c_str(),
           test::components::TransportManager::HighDataTransferRate::formatDataSize(bytesReceived).c_str(),
           test::components::TransportManager::HighDataTransferRate::formatDataSize(bytesReceived / dataTransferTime).c_str());
}

int main(int argc, char **argv)
{
    PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("log4cplus_disabled.properties"));

    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
