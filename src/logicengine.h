#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

enum class GateType {
    AND,
    OR,
    NOT,
    XOR,
    XNOR,
    NAND
};

struct GateState {
    GateType type;
    bool inputA = false;
    bool inputB = false;
    bool output = false;
};

class LogicEngine {
public:
    static bool evaluateGate(GateType type, bool a, bool b = false) {
        switch (type) {
        case GateType::AND:
            return a && b;
        case GateType::OR:
            return a || b;
        case GateType::NOT:
            return !a;
        case GateType::XOR:
            return a != b;
        case GateType::XNOR:
            return a == b;
        case GateType::NAND:
            return !(a && b);
        default:
            return false;
        }
    }

    static std::string gateName(GateType type) {
        switch (type) {
        case GateType::AND:
            return "AND";
        case GateType::OR:
            return "OR";
        case GateType::NOT:
            return "NOT";
        case GateType::XOR:
            return "XOR";
        case GateType::XNOR:
            return "XNOR";
        case GateType::NAND:
            return "NAND";
        default:
            return "UNKNOWN";
        }
    }
};
