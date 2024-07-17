// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllsutils_p.h"
#include "documentsymbolutils_p.h"
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <stack>

QT_BEGIN_NAMESPACE

namespace DocumentSymbolUtils {
using QLspSpecification::SymbolKind;
using namespace QQmlJS::Dom;

struct TypeSymbolRelation
{
    DomType domType;
    SymbolKind symbolKind;
};

constexpr static std::array<TypeSymbolRelation, 9> s_TypeSymbolRelations = { {
        { DomType::Binding, SymbolKind::Variable },
        { DomType::PropertyDefinition, SymbolKind::Property },
        // Although MethodInfo simply relates to Method, SymbolKind requires special handling:
        // When MethodInfo represents a Signal, its SymbolKind is set to Event.
        // This distinction is explicitly managed in the symbolKindOf() helper function.
        // see also QTBUG-128423
        { DomType::MethodInfo, SymbolKind::Method },
        { DomType::Id, SymbolKind::Key },
        { DomType::QmlObject, SymbolKind::Object },
        { DomType::EnumDecl, SymbolKind::Enum },
        { DomType::EnumItem, SymbolKind::EnumMember },
        { DomType::QmlComponent, SymbolKind::Module },
        { DomType::QmlFile, SymbolKind::File },
} };

[[nodiscard]] constexpr static inline SymbolKind symbolKindFor(const DomType &type)
{
    // constexpr std::find_if is only from c++20
    for (const auto &mapping : s_TypeSymbolRelations) {
        if (mapping.domType == type) {
            return mapping.symbolKind;
        }
    }
    return SymbolKind::Null;
}

constexpr static inline bool documentSymbolNotSupportedFor(const DomType &type)
{
    return symbolKindFor(type) == SymbolKind::Null;
}

static bool propertyBoundAtDefinitionLine(const DomItem &propertyDefinition)
{
    Q_ASSERT(propertyDefinition.internalKind() == DomType::PropertyDefinition);
    return FileLocations::treeOf(propertyDefinition)->info().regions[ColonTokenRegion].isValid();
}

static inline bool shouldFilterOut(const DomItem &item)
{
    const auto itemType = item.internalKind();
    if (documentSymbolNotSupportedFor(itemType)) {
        return true;
    }
    if (itemType == DomType::PropertyDefinition && propertyBoundAtDefinitionLine(item)) {
        // without this check there is a "duplication" of symbols.
        // one representing PropertyDefinition another one - Binding
        return true;
    }
    return false;
}

static std::optional<QByteArray> tryGetQmlObjectDetail(const DomItem &qmlObj)
{
    using namespace QQmlJS::Dom;
    Q_ASSERT(qmlObj.internalKind() == DomType::QmlObject);
    bool hasId = !qmlObj.idStr().isEmpty();
    if (hasId) {
        return qmlObj.idStr().toUtf8();
    }
    const bool isRootObject = qmlObj.component().field(Fields::objects).index(0) == qmlObj;
    if (isRootObject) {
        return "root";
    }
    return std::nullopt;
}

static std::optional<QByteArray> tryGetBindingDetail(const DomItem &bItem)
{
    const auto *bindingPtr = bItem.as<Binding>();
    Q_ASSERT(bindingPtr);
    switch (bindingPtr->valueKind()) {
    case BindingValueKind::ScriptExpression: {
        auto exprCode = bindingPtr->scriptExpressionValue()->code();
        if (exprCode.length() > 25) {
            return QStringView(exprCode).first(22).toUtf8().append("...");
        }
        if (exprCode.endsWith(QStringLiteral(";"))) {
            exprCode.chop(1);
        }
        return exprCode.toUtf8();
    }
    default:
        // Value is QmlObject or QList<QmlObject> => no detail
        return std::nullopt;
    }
}

static inline QByteArray getMethodDetail(const DomItem &mItem)
{
    const auto *methodInfoPtr = mItem.as<MethodInfo>();
    Q_ASSERT(methodInfoPtr);
    return methodInfoPtr->signature(mItem).toUtf8();
}

std::optional<QByteArray> tryGetDetailOf(const DomItem &item)
{
    switch (item.internalKind()) {
    case DomType::Id: {
        const auto name = item.name();
        return name.isEmpty() ? std::nullopt : std::make_optional(name.toUtf8());
    }
    case DomType::EnumItem:
        return QByteArray::number(item.as<EnumItem>()->value());
    case DomType::QmlObject:
        return tryGetQmlObjectDetail(item);
    case DomType::MethodInfo:
        return getMethodDetail(item);
    case DomType::Binding:
        return tryGetBindingDetail(item);
    default:
        return std::nullopt;
    }
}

/*! \internal
 * Constructs a \c DocumentSymbol for an \c Item with the provided \c children.
 * Returns \c children if the current \c Item should not be represented via a \c DocumentSymbol.
 */
SymbolsList buildSymbolOrReturnChildren(const DomItem &item, SymbolsList &&children)
{
    if (shouldFilterOut(item)) {
        // nothing to build, just returning children
        return std::move(children);
    }

    QLspSpecification::DocumentSymbol symbol;
    symbol.kind = symbolKindOf(item);
    symbol.name = symbolNameOf(item);
    symbol.detail = tryGetDetailOf(item);
    std::tie(symbol.range, symbol.selectionRange) = symbolRangesOf(item);
    if (!children.empty()) {
        symbol.children.emplace(std::move(children));
    }
    return SymbolsList{ std::move(symbol) };
}

std::pair<QLspSpecification::Range, QLspSpecification::Range> symbolRangesOf(const DomItem &item)
{
    const auto &fLoc = FileLocations::treeOf(item)->info();
    const auto fullRangeSourceloc = fLoc.fullRegion;
    const auto selectionRangeSourceLoc = fLoc.regions[IdentifierRegion].isValid()
            ? fLoc.regions[IdentifierRegion]
            : fullRangeSourceloc;

    auto fItem = item.containingFile();
    Q_ASSERT(fItem);
    const QString &code = fItem.ownerAs<QmlFile>()->code();
    return { QQmlLSUtils::qmlLocationToLspLocation(
                     QQmlLSUtils::Location::from({}, fullRangeSourceloc, code)),
             QQmlLSUtils::qmlLocationToLspLocation(
                     QQmlLSUtils::Location::from({}, selectionRangeSourceLoc, code)) };
}

QByteArray symbolNameOf(const DomItem &item)
{
    if (item.internalKind() == DomType::Id) {
        return "id";
    }
    return (item.name().isEmpty() ? item.internalKindStr() : item.name()).toUtf8();
}

QLspSpecification::SymbolKind symbolKindOf(const DomItem &item)
{
    if (item.internalKind() == DomType::MethodInfo) {
        const auto *methodInfoPtr = item.as<MethodInfo>();
        Q_ASSERT(methodInfoPtr);
        return methodInfoPtr->methodType == MethodInfo::MethodType::Signal
                ? SymbolKind::Event
                : symbolKindFor(DomType::MethodInfo);
    }
    return symbolKindFor(item.internalKind());
}

/*! \internal
 * Design decisions behind this class are the following:
 * 1. It is an implementation detail of the free \c assembleSymbolsForQmlFile function
 * 2. It can only be initialized and used once per \c Item.
 * This is enforced by its \c refToRootItem reference member.
 * 3. It is tested via the public \c assembleSymbolsForQmlFile function.
 */
class DocumentSymbolVisitor
{
public:
    DocumentSymbolVisitor(const DomItem &item, const AssemblingFunction af)
        : m_assemble(af), m_refToRootItem(item) {};

    static const FieldFilter &fieldsFilter();

    [[nodiscard]] SymbolsList assembleSymbols();

private:
    [[nodiscard]] SymbolsList popAndAssembleSymbolsFor(const DomItem &item);

    void appendToTop(const SymbolsList &symbols);

private:
    const AssemblingFunction m_assemble;
    const DomItem &m_refToRootItem;
    std::stack<SymbolsList> m_stackOfChildrenSymbols;
};

const FieldFilter &DocumentSymbolVisitor::fieldsFilter()
{
    // TODO(QTBUG-128118) add only fields to be visited and not the ones
    // to be removed.
    static const FieldFilter ff{
        {}, // to add
        {
                // to remove
                { QString(), QString::fromUtf16(Fields::code) },
                { QString(), QString::fromUtf16(Fields::postCode) },
                { QString(), QString::fromUtf16(Fields::preCode) },
                { QString(), QString::fromUtf16(Fields::importScope) },
                { QString(), QString::fromUtf16(Fields::fileLocationsTree) },
                { QString(), QString::fromUtf16(Fields::astComments) },
                { QString(), QString::fromUtf16(Fields::comments) },
                { QString(), QString::fromUtf16(Fields::exports) },
                { QString(), QString::fromUtf16(Fields::propertyInfos) },
                { QLatin1String("AttachedInfo"), QString::fromUtf16(Fields::parent) },
                //^^^ FieldFilter::default
                { QString(), QString::fromUtf16(Fields::errors) },
                { QString(), QString::fromUtf16(Fields::imports) },
                { QString(), QString::fromUtf16(Fields::prototypes) },
                { QString(), QString::fromUtf16(Fields::annotations) },
                { QString(), QString::fromUtf16(Fields::attachedType) },
                { QString(), QString::fromUtf16(Fields::canonicalFilePath) },
                { QString(), QString::fromUtf16(Fields::isValid) },
                { QString(), QString::fromUtf16(Fields::isSingleton) },
                { QString(), QString::fromUtf16(Fields::isCreatable) },
                { QString(), QString::fromUtf16(Fields::isComposite) },
                { QString(), QString::fromUtf16(Fields::attachedTypeName) },
                { QString(), QString::fromUtf16(Fields::pragmas) },
                { QString(), QString::fromUtf16(Fields::defaultPropertyName) },
                { QString(), QString::fromUtf16(Fields::name) },
                { QString(), QString::fromUtf16(Fields::nameIdentifiers) },
                { QString(), QString::fromUtf16(Fields::prototypes) },
                { QString(), QString::fromUtf16(Fields::nextScope) },
                { QString(), QString::fromUtf16(Fields::parameters) },
                { QString(), QString::fromUtf16(Fields::methodType) },
                { QString(), QString::fromUtf16(Fields::type) },
                { QString(), QString::fromUtf16(Fields::isConstructor) },
                { QString(), QString::fromUtf16(Fields::returnType) },
                { QString(), QString::fromUtf16(Fields::body) },
                { QString(), QString::fromUtf16(Fields::access) },
                { QString(), QString::fromUtf16(Fields::typeName) },
                { QString(), QString::fromUtf16(Fields::isReadonly) },
                { QString(), QString::fromUtf16(Fields::isList) },
                { QString(), QString::fromUtf16(Fields::bindingIdentifiers) },
                { QString(), QString::fromUtf16(Fields::bindingType) },
                { QString(), QString::fromUtf16(Fields::isSignalHandler) },
                // prop def?
                { QString(), QString::fromUtf16(Fields::isPointer) },
                { QString(), QString::fromUtf16(Fields::isFinal) },
                { QString(), QString::fromUtf16(Fields::isAlias) },
                { QString(), QString::fromUtf16(Fields::isDefaultMember) },
                { QString(), QString::fromUtf16(Fields::isRequired) },
                { QString(), QString::fromUtf16(Fields::read) },
                { QString(), QString::fromUtf16(Fields::write) },
                { QString(), QString::fromUtf16(Fields::bindable) },
                { QString(), QString::fromUtf16(Fields::notify) },
                { QString(), QString::fromUtf16(Fields::type) },
                // scriptExpr
                { QString(), QString::fromUtf16(Fields::scriptElement) },
                { QString(), QString::fromUtf16(Fields::localOffset) },
                { QString(), QString::fromUtf16(Fields::astRelocatableDump) },
                { QString(), QString::fromUtf16(Fields::expressionType) },
                // components
                { QString(), QString::fromUtf16(Fields::subComponents) },
                // TODO we actually need these, but should later be re-arranged?
                //{ QString(), QString::fromUtf16(Fields::ids) },
                // however idStr of the object should be ignored though?
                { QString(), QString::fromUtf16(Fields::idStr) },

                // id
                { QString(), QString::fromUtf16(Fields::referredObject) },
                // enum item
                { QLatin1String("EnumItem"), QString::fromUtf16(Fields::value) },
        }
    };
    return ff;
}

SymbolsList DocumentSymbolVisitor::assembleSymbols()
{
    using namespace QQmlJS::Dom;
    auto openingVisitor = [this](const Path &, const DomItem &, bool) -> bool {
        m_stackOfChildrenSymbols.emplace();
        return true;
    };
    auto closingVisitor = [this](const Path &, const DomItem &item, bool) -> bool {
        // it's closing Visitor, openingVisitor must have pushed something
        Q_ASSERT(!m_stackOfChildrenSymbols.empty());
        if (m_stackOfChildrenSymbols.size() == 1) {
            // reached children of root, nothing to do
            return false;
        }
        auto symbols = popAndAssembleSymbolsFor(item);
        appendToTop(symbols);
        return true;
    };
    m_refToRootItem.visitTree(Path(), emptyChildrenVisitor, VisitOption::Default, openingVisitor,
                              closingVisitor, fieldsFilter());
    return popAndAssembleSymbolsFor(m_refToRootItem);
}

SymbolsList DocumentSymbolVisitor::popAndAssembleSymbolsFor(const DomItem &item)
{
    Q_ASSERT(!m_stackOfChildrenSymbols.empty());
    auto atEnd = qScopeGuard([this]() { m_stackOfChildrenSymbols.pop(); });
    return m_assemble(item, std::move(m_stackOfChildrenSymbols.top()));
}

void DocumentSymbolVisitor::appendToTop(const SymbolsList &symbols)
{
    Q_ASSERT(!m_stackOfChildrenSymbols.empty());
    m_stackOfChildrenSymbols.top().append(symbols);
}

SymbolsList assembleSymbolsForQmlFile(const DomItem &item, const AssemblingFunction af)
{
    Q_ASSERT(item.internalKind() == DomType::QmlFile);
    DocumentSymbolVisitor visitor(item, af);
    return visitor.assembleSymbols();
}
} // namespace DocumentSymbolUtils

QT_END_NAMESPACE
