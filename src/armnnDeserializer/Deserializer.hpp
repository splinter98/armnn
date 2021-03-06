//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include "armnn/INetwork.hpp"
#include "armnnDeserializer/IDeserializer.hpp"
#include <ArmnnSchema_generated.h>

namespace armnnDeserializer
{
class Deserializer : public IDeserializer
{
public:
    // Shorthands for deserializer types
    using ConstTensorRawPtr = const armnnSerializer::ConstTensor *;
    using GraphPtr = const armnnSerializer::SerializedGraph *;
    using TensorRawPtr = const armnnSerializer::TensorInfo *;
    using PoolingDescriptor = const armnnSerializer::Pooling2dDescriptor *;
    using TensorRawPtrVector = std::vector<TensorRawPtr>;
    using LayerRawPtr = const armnnSerializer::LayerBase *;
    using LayerBaseRawPtr = const armnnSerializer::LayerBase *;
    using LayerBaseRawPtrVector = std::vector<LayerBaseRawPtr>;

public:

    /// Create an input network from binary file contents
    armnn::INetworkPtr CreateNetworkFromBinary(const std::vector<uint8_t>& binaryContent) override;

    /// Create an input network from a binary input stream
    armnn::INetworkPtr CreateNetworkFromBinary(std::istream& binaryContent) override;

    /// Retrieve binding info (layer id and tensor info) for the network input identified by the given layer name
    BindingPointInfo GetNetworkInputBindingInfo(unsigned int layerId, const std::string& name) const override;

    /// Retrieve binding info (layer id and tensor info) for the network output identified by the given layer name
    BindingPointInfo GetNetworkOutputBindingInfo(unsigned int layerId, const std::string& name) const override;

    Deserializer();
    ~Deserializer() {}

public:
    // testable helpers
    static GraphPtr LoadGraphFromBinary(const uint8_t* binaryContent, size_t len);
    static TensorRawPtrVector GetInputs(const GraphPtr& graph, unsigned int layerIndex);
    static TensorRawPtrVector GetOutputs(const GraphPtr& graph, unsigned int layerIndex);
    static LayerBaseRawPtrVector GetGraphInputs(const GraphPtr& graphPtr);
    static LayerBaseRawPtrVector GetGraphOutputs(const GraphPtr& graphPtr);
    static LayerBaseRawPtr GetBaseLayer(const GraphPtr& graphPtr, unsigned int layerIndex);
    static int32_t GetBindingLayerInfo(const GraphPtr& graphPtr, unsigned int layerIndex);
    static std::string GetLayerName(const GraphPtr& graph, unsigned int index);
    armnn::Pooling2dDescriptor GetPoolingDescriptor(PoolingDescriptor pooling2dDescriptor,
                                                    unsigned int layerIndex);
    static armnn::TensorInfo OutputShapeOfReshape(const armnn::TensorInfo & inputTensorInfo,
                                                  const std::vector<uint32_t> & targetDimsIn);

private:
    // No copying allowed until it is wanted and properly implemented
    Deserializer(const Deserializer&) = delete;
    Deserializer& operator=(const Deserializer&) = delete;

    /// Create the network from an already loaded flatbuffers graph
    armnn::INetworkPtr CreateNetworkFromGraph(GraphPtr graph);

    // signature for the parser functions
    using LayerParsingFunction = void(Deserializer::*)(GraphPtr graph, unsigned int layerIndex);

    void ParseUnsupportedLayer(GraphPtr graph, unsigned int layerIndex);
    void ParseActivation(GraphPtr graph, unsigned int layerIndex);
    void ParseAdd(GraphPtr graph, unsigned int layerIndex);
    void ParseConstant(GraphPtr graph, unsigned int layerIndex);
    void ParseConvolution2d(GraphPtr graph, unsigned int layerIndex);
    void ParseDepthwiseConvolution2d(GraphPtr graph, unsigned int layerIndex);
    void ParseFullyConnected(GraphPtr graph, unsigned int layerIndex);
    void ParseMultiplication(GraphPtr graph, unsigned int layerIndex);
    void ParsePermute(GraphPtr graph, unsigned int layerIndex);
    void ParsePooling2d(GraphPtr graph, unsigned int layerIndex);
    void ParseReshape(GraphPtr graph, unsigned int layerIndex);
    void ParseSoftmax(GraphPtr graph, unsigned int layerIndex);
    void ParseSpaceToBatchNd(GraphPtr graph, unsigned int layerIndex);

    void RegisterOutputSlotOfConnection(uint32_t connectionIndex, armnn::IOutputSlot* slot);
    void RegisterInputSlotOfConnection(uint32_t connectionIndex, armnn::IInputSlot* slot);
    void RegisterInputSlots(GraphPtr graph, uint32_t layerIndex,
                            armnn::IConnectableLayer* layer);
    void RegisterOutputSlots(GraphPtr graph, uint32_t layerIndex,
                             armnn::IConnectableLayer* layer);
    void ResetParser();

    void SetupInputLayers(GraphPtr graphPtr);
    void SetupOutputLayers(GraphPtr graphPtr);

    /// The network we're building. Gets cleared after it is passed to the user
    armnn::INetworkPtr                    m_Network;
    std::vector<LayerParsingFunction>     m_ParserFunctions;

    using NameToBindingInfo = std::pair<std::string, BindingPointInfo >;
    std::vector<NameToBindingInfo>    m_InputBindings;
    std::vector<NameToBindingInfo>    m_OutputBindings;

    /// A mapping of an output slot to each of the input slots it should be connected to
    /// The outputSlot is from the layer that creates this tensor as one of its outputs
    /// The inputSlots are from the layers that use this tensor as one of their inputs
    struct Slots
    {
        armnn::IOutputSlot* outputSlot;
        std::vector<armnn::IInputSlot*> inputSlots;

        Slots() : outputSlot(nullptr) { }
    };
    typedef std::vector<Slots> Connection;
    std::vector<Connection>   m_GraphConnections;
};

} //namespace armnnDeserializer
