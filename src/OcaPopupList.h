/*
   Copyright 2013-2016 Anton Runov

   This file is part of Octaudio.

   Octaudio is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Octaudio is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Octaudio.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OcaPopupList_h
#define OcaPopupList_h

#include <QListWidget>
#include <QStringList>
#include <QItemDelegate>

class OcaPopupList : public QListWidget
{
  Q_OBJECT ;

  public:
    OcaPopupList( QWidget* parent, QStringList list );
    ~OcaPopupList();

  public:
    virtual QSize sizeHint () const;

  public:
    void popup( QPoint pos );

  signals:
    void itemSelected( const QString& item );
    void cancelled();

  protected:
    virtual void keyReleaseEvent ( QKeyEvent * event );
    virtual void keyPressEvent ( QKeyEvent * event );
    virtual bool event ( QEvent * event );

  protected slots:
    void selectItem();

  protected:
    class ItemDelegate : public QItemDelegate
    {
      public:
        ItemDelegate( const OcaPopupList* list );
        virtual ~ItemDelegate();

      public:
        virtual void paint( QPainter*                     painter,
                            const QStyleOptionViewItem&    option,
                            const QModelIndex&              index   ) const;

      public:
        const OcaPopupList* m_list;
    };

  protected:
    void applyPattern();

  protected:
    QString     m_pattern;
    QStringList m_list;
};

#endif // OcaPopupList

