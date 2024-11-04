// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqmldom_fwd_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomelements_p.h"
#include "qqmldompath_p.h"

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using namespace Qt::StringLiterals;


/*!
\internal
\class QQmlJS::Dom::FileLocations
\brief Represents and maintains a mapping between elements and their location in a file

The location information is attached to the element it refers to via AttachedInfo
There are static methods to simplify the handling of the tree of AttachedInfo.

Attributes:
\list
\li fullRegion: A location guaranteed to include this element, its comments, and all its sub elements
\li regions: a map with locations of regions of this element, the empty string is the default region
    of this element
\endlist

\sa QQmlJs::Dom::AttachedInfo
*/
bool FileLocations::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    cont = cont && self.dvValueLazyField(visitor, Fields::fullRegion, [this]() {
        return sourceLocationToQCborValue(fullRegion);
    });
    cont = cont && self.dvItemField(visitor, Fields::regions, [this, &self]() -> DomItem {
        const Path pathFromOwner = self.pathFromOwner().field(Fields::regions);
        auto map = Map::fromFileRegionMap(pathFromOwner, regions);
        return self.subMapItem(map);
    });
    return cont;
}

FileLocations::Tree FileLocations::createTree(const Path &basePath){
    return AttachedInfoT<FileLocations>::createTree(basePath);
}

FileLocations::Tree FileLocations::ensure(const FileLocations::Tree &base, const Path &basePath)
{
    return AttachedInfoT<FileLocations>::ensure(base, basePath);
}

/*!
   \internal
   Returns the tree corresponding to a DomItem.
 */
FileLocations::Tree FileLocations::treeOf(const DomItem &item)
{
    Path p;
    DomItem fLoc = item.field(Fields::fileLocationsTree);
    if (!fLoc) {
        // owner or container.owner should be a file, so this works, but we could simply use the
        // canonical path, and PathType::Canonical instead...
        DomItem o = item.owner();
        p = item.pathFromOwner();
        fLoc = o.field(Fields::fileLocationsTree);
        while (!fLoc && o) {
            DomItem c = o.container();
            p = c.pathFromOwner().path(o.canonicalPath().last()).path(p);
            o = c.owner();
            fLoc = o.field(Fields::fileLocationsTree);
        }
    }
    if (AttachedInfo::Ptr fLocPtr = fLoc.ownerAs<AttachedInfo>())
        if (AttachedInfo::Ptr foundTree = AttachedInfo::find(fLocPtr, p))
            return std::static_pointer_cast<AttachedInfoT<FileLocations>>(foundTree);
    return {};
}

void FileLocations::updateFullLocation(const FileLocations::Tree &fLoc, SourceLocation loc)
{
    Q_ASSERT(fLoc);
    if (loc != SourceLocation()) {
        FileLocations::Tree p = fLoc;
        while (p) {
            SourceLocation &l = p->info().fullRegion;
            if (loc.begin() < l.begin() || loc.end() > l.end()) {
                l = combine(l, loc);
                p->info().regions[MainRegion] = l;
            } else {
                break;
            }
            p = p->parent();
        }
    }
}

// Adding a new region to file location regions might break down qmlformat because
// comments might be linked to new region undesirably. We might need to add an
// exception to AstRangesVisitor::shouldSkipRegion when confronted those cases.
void FileLocations::addRegion(const FileLocations::Tree &fLoc, FileLocationRegion region,
                              SourceLocation loc)
{
    Q_ASSERT(fLoc);
    fLoc->info().regions[region] = loc;
    updateFullLocation(fLoc, loc);
}

SourceLocation FileLocations::region(const FileLocations::Tree &fLoc, FileLocationRegion region)
{
    Q_ASSERT(fLoc);
    const auto &regions = fLoc->info().regions;
    if (auto it = regions.constFind(region); it != regions.constEnd() && it->isValid()) {
        return *it;
    }

    if (region == MainRegion)
        return fLoc->info().fullRegion;

    return SourceLocation{};
}

/*!
\internal
\class QQmlJS::Dom::AttachedInfo
\brief Attached info creates a tree to attach extra info to DomItems

Normally one uses the template AttachedInfoT<SpecificInfoToAttach>

static methods
Attributes:
\list
\li parent: parent AttachedInfo in tree (might be empty)
\li subItems: subItems of the tree (path -> AttachedInfo)
\li infoItem: the attached information
\endlist

\sa QQmlJs::Dom::AttachedInfo
*/

bool AttachedInfo::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = true;
    if (Ptr p = parent())
        cont = cont && self.dvItemField(visitor, Fields::parent, [&self, p]() {
            return self.copy(p, self.m_ownerPath.dropTail(2), p.get());
        });
    cont = cont
            && self.dvValueLazyField(visitor, Fields::path, [this]() { return path().toString(); });
    cont = cont && self.dvItemField(visitor, Fields::subItems, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::subItems),
                [this](const DomItem &map, const QString &key) {
                    Path p = Path::fromString(key);
                    return map.copy(m_subItems.value(p), map.canonicalPath().key(key));
                },
                [this](const DomItem &) {
                    QSet<QString> res;
                    for (const auto &p : m_subItems.keys())
                        res.insert(p.toString());
                    return res;
                },
                QLatin1String("AttachedInfo")));
    });
    cont = cont && self.dvItemField(visitor, Fields::infoItem, [&self, this]() {
        return infoItem(self);
    });
    return cont;
}

/*!
  \brief
  Returns that the AttachedInfo corresponding to the given path, creating it if it does not exists.

  The path might be either a relative path or a canonical path, as specified by the PathType
*/
AttachedInfo::Ptr AttachedInfo::ensure(const AttachedInfo::Ptr &self, const Path &path)
{
    Q_ASSERT(self);
    Path relative = path;
    Ptr res = self;
    for (const auto &p : std::as_const(relative)) {
        if (AttachedInfo::Ptr subEl = res->m_subItems.value(p)) {
            res = subEl;
        } else {
            AttachedInfo::Ptr newEl = res->instantiate(res, p);
            res->m_subItems.insert(p, newEl);
            res = newEl;
        }
    }
    return res;
}

AttachedInfo::Ptr AttachedInfo::find(const AttachedInfo::Ptr &self, const Path &p)
{
    Path rest = p;
    AttachedInfo::Ptr res = self;
    while (rest) {
        if (!res)
            break;
        res = res->m_subItems.value(rest.head());
        rest = rest.dropFront();
    }
    return res;
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#include "moc_qqmldomattachedinfo_p.cpp"
