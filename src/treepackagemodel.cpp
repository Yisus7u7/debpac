#include "../include/treepackagemodel.h"
#include "../include/folder.h"
#include "../include/realfile.h"
#include "../include/filesignatureinfo.hpp"
#include <QIcon>
#include <QFileInfo>

TreePackageModel::TreePackageModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    // default tree of a debian package
    tree = new Folder("package_name");
    tree->add(new Folder("DEBIAN")).add(new RealFile("control"));
    tree->add(new Folder("usr")).add(new Folder("bin"));
}

TreePackageModel::~TreePackageModel()
{
    delete tree;
}

QModelIndex TreePackageModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex ret;
    if (hasIndex(row, column, parent)){
        AbstractFile *parentItem;
        if (parent.isValid()){
            parentItem = static_cast<AbstractFile*>(parent.internalPointer());
        } else {
            parentItem = tree; // root
        }
        if(Folder* f = dynamic_cast<Folder*>(parentItem)) {
           ret = createIndex(row, column, f->child(row));
        }
    }
    return ret;
}

QModelIndex TreePackageModel::parent(const QModelIndex &index) const
{
    QModelIndex ret;
    if (index.isValid()){
        AbstractFile *child = static_cast<AbstractFile*>(index.internalPointer());
        AbstractFile *parent = child->getParent();
        if (parent && parent != child){
            ret = createIndex(1, 0, parent);
        }
    }
    return ret;
}

QVariant TreePackageModel::data(const QModelIndex &index, int role) const
{
    QVariant ret;
    if (index.isValid()){
        switch (role) {
        case Qt::DisplayRole:
            ret = displayRole(index);
            break;
        case Qt::DecorationRole:
            ret = decorationRole(index);
            break;
        default:
            break;
        }
    }
    return ret;
}

QVariant TreePackageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    QVariant ret;
    if (orientation == Qt::Horizontal){
        switch (role) {
        case Qt::DisplayRole:
            ret = QString("Package name: %1").arg(tree->getPath().c_str());
            break;
        default:
            break;
        }
    }
    return ret;
}

int TreePackageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int ret = 0;
    AbstractFile *parentItem = static_cast<AbstractFile*>(parent.internalPointer());
    if (parent.isValid()){
        if(Folder* f = dynamic_cast<Folder*>(parentItem)) {
            ret = f->count(false);
        } else {
            ret = 0;
        }
    } else {
        // root
        ret = tree->count(false);
    }
    return ret;
}

int TreePackageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

void TreePackageModel::addFileSignatureInfo(FileSignatureInfo *fsi)
{
    QModelIndex parentIndex = index(0);
    QString folder;
    switch (fsi->getCategory()) {
    case FileSignatureInfo::BINARY:
        folder = "usr/bin";
        break;
    case FileSignatureInfo::AUDIO:
        folder = "usr/share/"+QString(tree->getPath().c_str())+"/sounds";
        break;
    case FileSignatureInfo::IMAGE:
        if (QPixmap(fsi->getPath().c_str()).width() == QPixmap(fsi->getPath().c_str()).height()){
            folder = "usr/share/icons/hicolor/"+QString("%1x%1").arg(QPixmap(fsi->getPath().c_str()).width())+"/apps";
        } else {
            folder = "usr/share/"+QString(tree->getPath().c_str())+"/images";
        }
        break;
    case FileSignatureInfo::PACKAGE:
    case FileSignatureInfo::ARCHIVE:
        folder = "usr/share/"+QString(tree->getPath().c_str());
        break;
    default:
        break;
    }
    if (!folder.isEmpty()){
        QStringList sl = folder.split("/");
        Folder *f = tree;
        for (QString s : sl){
            if (Folder *current = f->getChild<Folder*>(s.toStdString())){
                f = current;
            } else {
                int at = f->count(false);
                beginInsertRows(parentIndex, at, at);
                Folder *nf = new Folder(s.toStdString());
                f->add(nf);
                f = nf;
                endInsertRows();
            }
            parentIndex = index(0, 0, parentIndex);
        }
        if (f){
            int at = f->count(false);
            beginInsertRows(parentIndex, at, at);
            f->add(new RealFile(QFileInfo(fsi->getPath().c_str()).completeBaseName().toStdString().c_str(), fsi));
            endInsertRows();
        }
    }
}

QVariant TreePackageModel::displayRole(const QModelIndex &index) const
{
    QString ret = QString(static_cast<AbstractFile*>(index.internalPointer())->getPath().c_str());
    return ret;
}

QVariant TreePackageModel::decorationRole(const QModelIndex &index) const
{
    QIcon ret;
    AbstractFile *af = static_cast<AbstractFile*>(index.internalPointer());
    if(dynamic_cast<Folder*>(af)) {
        ret = QIcon("://icon/folder.png");
    } else {
        ret = dynamic_cast<RealFile*>(af)->getFileSignatureInfo().getIcon();
    }
    return ret;
}
