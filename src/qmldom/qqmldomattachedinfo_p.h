// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOMATTACHEDINFO_P_H
#define QMLDOMATTACHEDINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmldom_global.h"
#include "qqmldomitem_p.h"

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT AttachedInfo : public OwningItem  {
    Q_GADGET
public:
    constexpr static DomType kindValue = DomType::AttachedInfo;
    using Ptr = std::shared_ptr<AttachedInfo>;

    DomType kind() const override { return kindValue; }
    // mainly used for debugging, for example dumping qmlFile
    Path canonicalPath(const DomItem &self) const override { return self.m_ownerPath; }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const override;

    AttachedInfo::Ptr makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<AttachedInfo>(doCopy(self));
    }

    Ptr parent() const { return m_parent.lock(); }
    Path path() const { return m_path; }
    void setPath(const Path &p) { m_path = p; }

    AttachedInfo(const Ptr &parent = nullptr, const Path &p = Path())
        : m_path(p), m_parent(parent)
    {}

    AttachedInfo(const AttachedInfo &o) = default;

    static Ptr ensure(const Ptr &self, const Path &path);
    static Ptr find(const Ptr &self, const Path &p);

    virtual AttachedInfo::Ptr instantiate(
            const AttachedInfo::Ptr &parent, const Path &p = Path()) const = 0;
    virtual DomItem infoItem(const DomItem &self) const = 0;
    QMap<Path, Ptr> subItems() const {
        return m_subItems;
    }
protected:
    Path m_path;
    std::weak_ptr<AttachedInfo> m_parent;
    QMap<Path, Ptr> m_subItems;
};

template<typename Info>
class QMLDOM_EXPORT AttachedInfoT final : public AttachedInfo
{
public:
    constexpr static DomType kindValue = DomType::AttachedInfo;
    using Ptr = std::shared_ptr<AttachedInfoT>;
    using InfoType = Info;

    AttachedInfoT(const Ptr &parent = nullptr, const Path &p = Path()) : AttachedInfo(parent, p) {}
    AttachedInfoT(const AttachedInfoT &o):
        AttachedInfo(o),
        m_info(o.m_info)
    {
        auto end = o.m_subItems.end();
        auto i = o.m_subItems.begin();
        while (i != end) {
            m_subItems.insert(i.key(), Ptr(
                                  new AttachedInfoT(*std::static_pointer_cast<AttachedInfoT>(i.value()).get())));
        }
    }

   static Ptr createTree(const Path &p = Path()) {
        return Ptr(new AttachedInfoT(nullptr, p));
    }

    static Ptr ensure(const Ptr &self, const Path &path)
    {
        return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::ensure(self, path));
    }

    static Ptr find(const Ptr &self, const Path &p)
    {
        return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::find(self, p));
    }

    static bool visitTree(const Ptr &base, function_ref<bool(const Path &, const Ptr &)> visitor,
                          const Path &basePath = Path())
    {
        if (base) {
            Path pNow = basePath.path(base->path());
            if (visitor(pNow, base)) {
                auto it = base->m_subItems.cbegin();
                auto end = base->m_subItems.cend();
                while (it != end) {
                    if (!visitTree(std::static_pointer_cast<AttachedInfoT>(it.value()), visitor, pNow))
                        return false;
                    ++it;
                }
            } else {
                return false;
            }
        }
        return true;
    }

    AttachedInfo::Ptr instantiate(
            const AttachedInfo::Ptr &parent, const Path &p = Path()) const override
    {
        return Ptr(new AttachedInfoT(std::static_pointer_cast<AttachedInfoT>(parent), p));
    }

    DomItem infoItem(const DomItem &self) const override { return self.wrapField(Fields::infoItem, m_info); }
    Ptr parent() const { return std::static_pointer_cast<AttachedInfoT>(AttachedInfo::parent()); }

    Info &info() { return m_info; }

    QString canonicalPathForTesting() const
    {
        QString result;
        for (auto *it = this; it; it = it->parent().get()) {
            result.prepend(it->path().toString());
        }
        return result;
    }

protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return Ptr(new AttachedInfoT(*this));
    }

private:
    Info m_info;
};

class QMLDOM_EXPORT FileLocations {
public:
    using Tree = std::shared_ptr<AttachedInfoT<FileLocations>>;
    constexpr static DomType kindValue = DomType::FileLocations;
    DomType kind() const {  return kindValue; }
    // mainly used for debugging, for example dumping qmlFile
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const;

    static Tree createTree(const Path &basePath);
    static Tree ensure(const Tree &base, const Path &basePath);
    static Tree find(const Tree &self, const Path &p)
    {
        return AttachedInfoT<FileLocations>::find(self, p);
    }

    // returns the path looked up and the found tree when looking for the info attached to item
    static FileLocations::Tree treeOf(const DomItem &);

    static void updateFullLocation(const Tree &fLoc, SourceLocation loc);
    static void addRegion(const Tree &fLoc, FileLocationRegion region, SourceLocation loc);
    static QQmlJS::SourceLocation region(const Tree &fLoc, FileLocationRegion region);

public:
    SourceLocation fullRegion;
    QMap<FileLocationRegion, SourceLocation> regions;
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMATTACHEDINFO_P_H
