//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//
#pragma once

namespace armnn
{
namespace
{

// Make a workload of the specified WorkloadType.
template<typename WorkloadType>
struct MakeWorkloadForType
{
    template<typename QueueDescriptorType, typename... Args>
    static std::unique_ptr<WorkloadType> Func(const QueueDescriptorType& descriptor,
                                              const WorkloadInfo& info,
                                              Args&&... args)
    {
        return std::make_unique<WorkloadType>(descriptor, info, std::forward<Args>(args)...);
    }
};

// Specialization for void workload type used for unsupported workloads.
template<>
struct MakeWorkloadForType<NullWorkload>
{
    template<typename QueueDescriptorType, typename... Args>
    static std::unique_ptr<NullWorkload> Func(const QueueDescriptorType& descriptor,
                                              const WorkloadInfo& info,
                                              Args&&... args)
    {
        return nullptr;
    }
};

// Makes a workload for one the specified types based on the data type requirements of the tensorinfo.
// Specify type void as the WorkloadType for unsupported DataType/WorkloadType combos.
template <typename Float16Workload, typename Float32Workload, typename Uint8Workload, typename Int32Workload,
          typename BooleanWorkload, typename QueueDescriptorType, typename... Args>
std::unique_ptr<IWorkload> MakeWorkloadHelper(const QueueDescriptorType& descriptor,
                                              const WorkloadInfo& info,
                                              Args&&... args)
{
    const DataType dataType = !info.m_InputTensorInfos.empty() ?
        info.m_InputTensorInfos[0].GetDataType()
        : info.m_OutputTensorInfos[0].GetDataType();

    BOOST_ASSERT(info.m_InputTensorInfos.empty()  ||
                 info.m_OutputTensorInfos.empty() ||
                 ((info.m_InputTensorInfos[0].GetDataType() == info.m_OutputTensorInfos[0].GetDataType()) ||
                   info.m_OutputTensorInfos[0].GetDataType() == armnn::DataType::Boolean));

    switch (dataType)
    {
        case DataType::Float16:
            return MakeWorkloadForType<Float16Workload>::Func(descriptor, info, std::forward<Args>(args)...);
        case DataType::Float32:
            return MakeWorkloadForType<Float32Workload>::Func(descriptor, info, std::forward<Args>(args)...);
        case DataType::QuantisedAsymm8:
            return MakeWorkloadForType<Uint8Workload>::Func(descriptor, info, std::forward<Args>(args)...);
        case DataType::Signed32:
            return MakeWorkloadForType<Int32Workload>::Func(descriptor, info, std::forward<Args>(args)...);
        case DataType::Boolean:
            return MakeWorkloadForType<BooleanWorkload>::Func(descriptor, info, std::forward<Args>(args)...);
        default:
            BOOST_ASSERT_MSG(false, "Unknown DataType.");
            return nullptr;
    }
}

// Makes a workload for one the specified types based on the data type requirements of the tensorinfo.
// Calling this method is the equivalent of calling the five typed MakeWorkload method with <FloatWorkload,
// FloatWorkload, Uint8Workload, NullWorkload, NullWorkload>.
// Specify type void as the WorkloadType for unsupported DataType/WorkloadType combos.
template <typename FloatWorkload, typename Uint8Workload, typename QueueDescriptorType, typename... Args>
std::unique_ptr<IWorkload> MakeWorkloadHelper(const QueueDescriptorType& descriptor,
                                              const WorkloadInfo& info,
                                              Args&&... args)
{
    return MakeWorkloadHelper<FloatWorkload, FloatWorkload, Uint8Workload, NullWorkload, NullWorkload>(
        descriptor,
        info,
        std::forward<Args>(args)...);
}

} //namespace
} //namespace armnn
