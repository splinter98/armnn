//
// Copyright © 2017 Arm Ltd. All rights reserved.
// See LICENSE file in the project root for full license information.
//

#if (defined(__aarch64__)) || (defined(__x86_64__)) // disable test failing on FireFly/Armv7

#include "arm_compute/runtime/CL/CLScheduler.h"
#include "backends/ClContextControl.hpp"
#include "backends/ClWorkloadFactory.hpp"
#include "backends/CpuTensorHandle.hpp"
#include <boost/format.hpp>
#include <iostream>
#include "OpenClTimer.hpp"
#include "backends/test/TensorCopyUtils.hpp"
#include "TensorHelpers.hpp"
#include <boost/test/unit_test.hpp>
#include "backends/WorkloadFactory.hpp"
#include "backends/test/WorkloadTestUtils.hpp"

using namespace armnn;

struct OpenClFixture
{
    // Initialising ClContextControl to ensure OpenCL is loaded correctly for each test case.
    // NOTE: Profiling needs to be enabled in ClContextControl to be able to obtain execution
    // times from OpenClTimer.
    OpenClFixture() : m_ClContextControl(nullptr, true) {}
    ~OpenClFixture() {}

    ClContextControl m_ClContextControl;
};

BOOST_FIXTURE_TEST_SUITE(OpenClTimerBatchNorm, OpenClFixture)
using FactoryType = ClWorkloadFactory;

BOOST_AUTO_TEST_CASE(OpenClTimerBatchNorm)
{
    ClWorkloadFactory  workloadFactory;

    const unsigned int width    = 2;
    const unsigned int height   = 3;
    const unsigned int channels = 2;
    const unsigned int num      = 1;
    int32_t qOffset = 0;
    float qScale = 0.f;

    TensorInfo inputTensorInfo({num, channels, height, width}, GetDataType<float>());
    TensorInfo outputTensorInfo({num, channels, height, width}, GetDataType<float>());
    TensorInfo tensorInfo({channels}, GetDataType<float>());

    // Set quantization parameters if the requested type is a quantized type.
    if(IsQuantizedType<float>())
    {
         inputTensorInfo.SetQuantizationScale(qScale);
         inputTensorInfo.SetQuantizationOffset(qOffset);
         outputTensorInfo.SetQuantizationScale(qScale);
         outputTensorInfo.SetQuantizationOffset(qOffset);
         tensorInfo.SetQuantizationScale(qScale);
         tensorInfo.SetQuantizationOffset(qOffset);
    }

    auto input = MakeTensor<float, 4>(inputTensorInfo,
    QuantizedVector<float>(qScale, qOffset,
    {
        1.f, 4.f,
        4.f, 2.f,
        1.f, 6.f,

        1.f, 1.f,
        4.f, 1.f,
        -2.f, 4.f
    }));
    // these values are per-channel of the input
    auto mean     = MakeTensor<float, 1>(tensorInfo, QuantizedVector<float>(qScale, qOffset, {3, -2}));
    auto variance = MakeTensor<float, 1>(tensorInfo, QuantizedVector<float>(qScale, qOffset, {4, 9}));
    auto beta     = MakeTensor<float, 1>(tensorInfo, QuantizedVector<float>(qScale, qOffset, {3, 2}));
    auto gamma    = MakeTensor<float, 1>(tensorInfo, QuantizedVector<float>(qScale, qOffset, {2, 1}));

    std::unique_ptr<ITensorHandle> inputHandle = workloadFactory.CreateTensorHandle(inputTensorInfo);
    std::unique_ptr<ITensorHandle> outputHandle = workloadFactory.CreateTensorHandle(outputTensorInfo);

    BatchNormalizationQueueDescriptor data;
    WorkloadInfo info;
    ScopedCpuTensorHandle meanTensor(tensorInfo);
    ScopedCpuTensorHandle varianceTensor(tensorInfo);
    ScopedCpuTensorHandle betaTensor(tensorInfo);
    ScopedCpuTensorHandle gammaTensor(tensorInfo);

    AllocateAndCopyDataToITensorHandle(&meanTensor, &mean[0]);
    AllocateAndCopyDataToITensorHandle(&varianceTensor, &variance[0]);
    AllocateAndCopyDataToITensorHandle(&betaTensor, &beta[0]);
    AllocateAndCopyDataToITensorHandle(&gammaTensor, &gamma[0]);

    AddInputToWorkload(data, info, inputTensorInfo, inputHandle.get());
    AddOutputToWorkload(data, info, outputTensorInfo, outputHandle.get());
    data.m_Mean             = &meanTensor;
    data.m_Variance         = &varianceTensor;
    data.m_Beta             = &betaTensor;
    data.m_Gamma            = &gammaTensor;
    data.m_Parameters.m_Eps = 0.0f;

    // for each channel:
    // substract mean, divide by standard deviation (with an epsilon to avoid div by 0)
    // multiply by gamma and add beta
    std::unique_ptr<IWorkload> workload = workloadFactory.CreateBatchNormalization(data, info);

    inputHandle->Allocate();
    outputHandle->Allocate();

    CopyDataToITensorHandle(inputHandle.get(), &input[0][0][0][0]);

    OpenClTimer openClTimer;

    BOOST_CHECK_EQUAL(openClTimer.GetName(), "OpenClKernelTimer");

    //Start the timer
    openClTimer.Start();

    //Execute the workload
    workload->Execute();

    //Stop the timer
    openClTimer.Stop();

    BOOST_CHECK_EQUAL(openClTimer.GetMeasurements().size(), 1);

    BOOST_CHECK_EQUAL(openClTimer.GetMeasurements().front().m_Name,
                      "OpenClKernelTimer/0: batchnormalization_layer_nchw GWS[1,3,2]");

    BOOST_CHECK(openClTimer.GetMeasurements().front().m_Value > 0);

}

BOOST_AUTO_TEST_SUITE_END()

#endif //aarch64 or x86_64