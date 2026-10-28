#include "pch.h"
#include "MaterialGraph.h"
using namespace GameEngine;

const Pin* MaterialGraph::FindPin(int pinId) const {
    for (auto& node : nodes) {
        for (const Pin& pin : node->GetInputs()) {
            if (pin.id == pinId) { return &pin; }
        }
        for (const Pin& pin : node->GetOutputs()) {
            if (pin.id == pinId) { return &pin; }
        }
    }
    return nullptr;
}

IMaterialNode* MaterialGraph::FindNode(int nodeId) const {
    for (auto& node : nodes) {
        if (node->GetId() == nodeId) { 
            return node.get(); 
        }
    }
    return nullptr;
}

bool MaterialGraph::IsPinLinked(int pinId) const {
    for (auto& l : links) { if (l.startPinId == pinId || l.endPinId == pinId) return true; }
    return false;
}
