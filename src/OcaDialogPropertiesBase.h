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

#ifndef OcaDialogPropertiesBase_h
#define OcaDialogPropertiesBase_h

#include <QWidget>

class OcaObject;
class OcaObjectListener;
class QLineEdit;

class OcaDialogPropertiesBase : public QWidget
{
  Q_OBJECT ;

  public:
    OcaDialogPropertiesBase();
    ~OcaDialogPropertiesBase();

  protected:
    virtual OcaObject* getObject() const = 0;

  protected:
    void updateNames();
    void createListener( OcaObject* obj, uint mask );

  protected slots:
    virtual void onUpdateRequired( uint flags ) = 0;
    void onNameChanged();
    void onDisplayNameChanged();
    void onOk();

  protected:
    virtual void changeEvent( QEvent* event );
    virtual void keyReleaseEvent( QKeyEvent* event );

  protected:
    OcaObjectListener*    m_listener;
    bool                  m_ok;
    QLineEdit*            m_editName;
    QLineEdit*            m_editDisplayName;
    bool                  m_disableAutoClose;
};

#endif // OcaDialogPropertiesBase_h
