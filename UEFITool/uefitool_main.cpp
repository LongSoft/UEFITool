/* uefitool_main.cpp

  Copyright (c) 2018, LongSoft. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#include <QApplication>
#include <QString>
#include "uefitool.h"

class UEFIToolApplication : public QApplication
{
    UEFITool* tool;

public:
    UEFIToolApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        setOrganizationName("LongSoft");
        setOrganizationDomain("longsoft.org");
        setApplicationName("UEFITool NE");
        
        tool = new UEFITool();
    }
    
    virtual ~UEFIToolApplication() {
        delete tool;
    }
    
    virtual bool event(QEvent *event)
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            tool->openImageFile(openEvent->file());
        }

        return QApplication::event(event);
    }
    
    int startup()
    {
        tool->setProgramPath(arguments().at(0));
        if (arguments().length() > 1)
            tool->openImageFile(arguments().at(1));
        tool->show();
        
        return exec();
    }
};

int main(int argc, char *argv[])
{
    UEFIToolApplication a(argc, argv);
    return a.startup();
}
