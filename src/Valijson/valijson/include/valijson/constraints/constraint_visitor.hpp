#pragma once

namespace valijson {
namespace constraints {

class AllOfConstraint;
class AnyOfConstraint;
class ConditionalConstraint;
class ConstConstraint;
class ContainsConstraint;
class DependenciesConstraint;
class EnumConstraint;
class LinearItemsConstraint;
class MaxItemsConstraint;
class MaximumConstraint;
class MaxLengthConstraint;
class MaxPropertiesConstraint;
class MinItemsConstraint;
class MinimumConstraint;
class MinLengthConstraint;
class MinPropertiesConstraint;
class MultipleOfDoubleConstraint;
class MultipleOfIntConstraint;
class NotConstraint;
class OneOfConstraint;
class PatternConstraint;
class PolyConstraint;
class PropertiesConstraint;
class PropertyNamesConstraint;
class RequiredConstraint;
class SingularItemsConstraint;
class TypeConstraint;
class UniqueItemsConstraint;

/// Interface to allow usage of the visitor pattern with Constraints
class ConstraintVisitor
{
protected:
    virtual ~ConstraintVisitor() {}

    // Shorten type names for derived classes outside of this namespace
    typedef constraints::AllOfConstraint AllOfConstraint;
    typedef constraints::AnyOfConstraint AnyOfConstraint;
    typedef constraints::ConditionalConstraint ConditionalConstraint;
    typedef constraints::ConstConstraint ConstConstraint;
    typedef constraints::ContainsConstraint ContainsConstraint;
    typedef constraints::DependenciesConstraint DependenciesConstraint;
    typedef constraints::EnumConstraint EnumConstraint;
    typedef constraints::LinearItemsConstraint LinearItemsConstraint;
    typedef constraints::MaximumConstraint MaximumConstraint;
    typedef constraints::MaxItemsConstraint MaxItemsConstraint;
    typedef constraints::MaxLengthConstraint MaxLengthConstraint;
    typedef constraints::MaxPropertiesConstraint MaxPropertiesConstraint;
    typedef constraints::MinimumConstraint MinimumConstraint;
    typedef constraints::MinItemsConstraint MinItemsConstraint;
    typedef constraints::MinLengthConstraint MinLengthConstraint;
    typedef constraints::MinPropertiesConstraint MinPropertiesConstraint;
    typedef constraints::MultipleOfDoubleConstraint MultipleOfDoubleConstraint;
    typedef constraints::MultipleOfIntConstraint MultipleOfIntConstraint;
    typedef constraints::NotConstraint NotConstraint;
    typedef constraints::OneOfConstraint OneOfConstraint;
    typedef constraints::PatternConstraint PatternConstraint;
    typedef constraints::PolyConstraint PolyConstraint;
    typedef constraints::PropertiesConstraint PropertiesConstraint;
    typedef constraints::PropertyNamesConstraint PropertyNamesConstraint;
    typedef constraints::RequiredConstraint RequiredConstraint;
    typedef constraints::SingularItemsConstraint SingularItemsConstraint;
    typedef constraints::TypeConstraint TypeConstraint;
    typedef constraints::UniqueItemsConstraint UniqueItemsConstraint;

public:

    virtual bool visit(const AllOfConstraint &) = 0;
    virtual bool visit(const AnyOfConstraint &) = 0;
    virtual bool visit(const ConditionalConstraint &) = 0;
    virtual bool visit(const ConstConstraint &) = 0;
    virtual bool visit(const ContainsConstraint &) = 0;
    virtual bool visit(const DependenciesConstraint &) = 0;
    virtual bool visit(const EnumConstraint &) = 0;
    virtual bool visit(const LinearItemsConstraint &) = 0;
    virtual bool visit(const MaximumConstraint &) = 0;
    virtual bool visit(const MaxItemsConstraint &) = 0;
    virtual bool visit(const MaxLengthConstraint &) = 0;
    virtual bool visit(const MaxPropertiesConstraint &) = 0;
    virtual bool visit(const MinimumConstraint &) = 0;
    virtual bool visit(const MinItemsConstraint &) = 0;
    virtual bool visit(const MinLengthConstraint &) = 0;
    virtual bool visit(const MinPropertiesConstraint &) = 0;
    virtual bool visit(const MultipleOfDoubleConstraint &) = 0;
    virtual bool visit(const MultipleOfIntConstraint &) = 0;
    virtual bool visit(const NotConstraint &) = 0;
    virtual bool visit(const OneOfConstraint &) = 0;
    virtual bool visit(const PatternConstraint &) = 0;
    virtual bool visit(const PolyConstraint &) = 0;
    virtual bool visit(const PropertiesConstraint &) = 0;
    virtual bool visit(const PropertyNamesConstraint &) = 0;
    virtual bool visit(const RequiredConstraint &) = 0;
    virtual bool visit(const SingularItemsConstraint &) = 0;
    virtual bool visit(const TypeConstraint &) = 0;
    virtual bool visit(const UniqueItemsConstraint &) = 0;
};

}  // namespace constraints
}  // namespace valijson
