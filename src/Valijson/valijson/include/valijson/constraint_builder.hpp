#pragma once

namespace valijson {

namespace adapters {
    class Adapter;
}

namespace constraints {
    struct Constraint;
}

class ConstraintBuilder
{
public:
    virtual ~ConstraintBuilder() {}

    virtual constraints::Constraint * make(const adapters::Adapter &) const = 0;
};

}  // namespace valijson
